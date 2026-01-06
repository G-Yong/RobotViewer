#include "opcuaconnector.h"

#include <QtConcurrentRun>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <QCoreApplication>

QVariant convertUAVariantToQVariant(const UA_Variant &data);

OPCUAConnector::OPCUAConnector(QObject *parent)
    : BaseConnector{parent}
{
    // 周期性调用，处理opcua的各种事件
    auto functor = [=](){
        qDebug() << "opcua operation thread:" << QThread::currentThread();

        QElapsedTimer readValTimer;
        readValTimer.start();

        while(mExited == false)
        {
            bool disconnected = false;
            // 已打算释放的话，就不要继续处理了
            if(mClient != nullptr && state() == 1 && mTryRelease == false)
            {
                // 一定要放在这里，不然一直阻塞
                QMutexLocker locker(&mClientMutex);

                // 设置为0好像也是可以的 https://github.com/open62541/open62541/issues/3817
                if(mClient)
                {
                    // https://blog.csdn.net/whahu1989/article/details/102905028
                    // UA_Client_run_iterate()的第二个参数1000表示1000ms，
                    // 意思是发送完Publish请求后，Server要在1000ms内回复，不是回复被监测变量值，
                    // 而是回复已收到该Publish请求。也可以设置为0，这样就不用等Server的回复了，属于异步写法。
                    // 设置非0值会导致线程堵塞相应的时间
                    auto ret = UA_Client_run_iterate(mClient, 0); // 这个函数需要周期性调用，否则无法实现监听

                    UA_StatusCode statusCode;

                    UA_Client_getState(mClient, NULL, NULL, &statusCode);
                    // qDebug() << "status code:" << QString::number(statusCode, 16);
                    if(UA_StatusCode_isGood(statusCode) == false)
                    {
                        disconnected = true;
                        mTryRelease  = true;
                        qDebug() << "opcaua disconnected";
                    }
                }

                if(mClient == nullptr) continue;

#ifdef USE_OPCUA_SUBSCRIPTION
#else
                if(readValTimer.elapsed() >= 100) // 100ms读一次就好，不要太快
                {
                    readValTimer.restart();

                    // QElapsedTimer timer1;
                    // timer1.start();

                    customSubsProcess(); // 用周期性读取来替代opcua的订阅

                    // qDebug() << "readAllSubscribValue interval:" << timer1.elapsed() << mMonVarMap.count() << this;
                }
#endif
            }

            if(disconnected == true)
            {
                QMetaObject::invokeMethod(this, "release", Qt::QueuedConnection);
                // release();
            }

            // QThread::usleep(1);
            QThread::msleep(1);
        }

        qInfo() << "UA_Client_run_iterate finished";
    };
    // mFunture = QtConcurrent::run(functor);
    MyThread::run(functor);
}

OPCUAConnector::~OPCUAConnector()
{
    qDebug() << "~OPCUAConnector()";
    mExited = true;
    // mFunture.waitForFinished();

    // 稍微延时一下，等待子线程退出
    QThread::msleep(20);

    // this->release();

    qDebug() << "release completed";
}

void customLogger(void *context,
                         UA_LogLevel level,
                         UA_LogCategory category,
                         const char *msg,
                         va_list args);

void subscriptionInactivity(UA_Client *client,
                            UA_UInt32 subscriptionId,
                            void *subContext);

int OPCUAConnector::init(QStringList paramList)
{
    if(paramList.length() != 2)
    {
        return -1;
    }

    if(mClient != nullptr)
    {
        release();
    }

    mHostInfo = paramList.first();
    mPrefix =  paramList.at(1);

    QMutexLocker locker(&mClientMutex);

    mClient = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(mClient);
    UA_ClientConfig_setDefault(config); // 进行了一些默认设置

    // config->timeout = 1000;
    // config->secureChannelLifeTime = 10 * 24 * 60 * 60 * 1000; // 没必要设置太久，因为到时会renew
    config->stateCallback = onStateChanged;
    // config->logger.log = customLogger; // 自定义日志处理函数
    config->clientContext = this;
    config->subscriptionInactivityCallback = subscriptionInactivity; // 订阅失效时的回调函数


    // qDebug() << "default timeout:"
    //          << config->timeout
    //          << config->secureChannelLifeTime
    //          << config->requestedSessionTimeout
    //          << config->connectivityCheckInterval;

    // qDebug() << "try connect:" << mHostInfo;
    // 释放时，要调用 UA_Client_disconnect(client)
    UA_StatusCode status = UA_Client_connect(mClient, mHostInfo.toStdString().c_str());
    // UA_StatusCode status = UA_Client_connectUsername(client, "opc.tcp://127.0.0.1:4840", "1", "1");;
    if(status != UA_STATUSCODE_GOOD)
    {
        qDebug() << "-----------client connect not finished" << QString::number(status, 16).toUpper();

        UA_Client_delete(mClient);
        mClient = nullptr;

        setState(0);
        return -1;
    }
    else
    {
        QThread::msleep(10);

        // 获取服务器的OperationLimits-MaxNodesPerRead
        {
            UA_NodeId nodeId = UA_NODEID_NUMERIC(0,
                                                 UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERREAD);
            UA_Variant var;
            UA_Variant_init(&var);
            UA_StatusCode ret = UA_Client_readValueAttribute(mClient,
                                                             nodeId,
                                                             &var);
            if(UA_StatusCode_isGood(ret))
            {
                QVariant value = convertUAVariantToQVariant(var);
                mMaxNodesPerRead = value.toInt();
            }

            UA_Variant_clear(&var);
            // qDebug() << "++++++++++++mMaxNodesPerRead:"
            //          << UA_StatusCode_name(ret)
            //          << mMaxNodesPerRead;
        }

        // 获取服务器的OperationLimits-MaxNodesPerWrite
        {
            UA_NodeId nodeId = UA_NODEID_NUMERIC(0,
                                                 UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXNODESPERWRITE);
            UA_Variant var;
            UA_Variant_init(&var);
            UA_StatusCode ret = UA_Client_readValueAttribute(mClient,
                                                             nodeId,
                                                             &var);
            if(UA_StatusCode_isGood(ret))
            {
                QVariant value = convertUAVariantToQVariant(var);
                mMaxNodesPerWrite = value.toInt();
            }

            UA_Variant_clear(&var);

            qDebug() << "++++++++++++mMaxNodesPerWrite:"
                     << UA_StatusCode_name(ret)
                     << mMaxNodesPerWrite;
        }

        // MaxMonitoredItemsPerCall
        {
            UA_NodeId nodeId = UA_NODEID_NUMERIC(0,
                                                 UA_NS0ID_SERVER_SERVERCAPABILITIES_OPERATIONLIMITS_MAXMONITOREDITEMSPERCALL);
            UA_Variant var;
            UA_Variant_init(&var);
            UA_StatusCode ret = UA_Client_readValueAttribute(mClient,
                                                             nodeId,
                                                             &var);
            if(UA_StatusCode_isGood(ret))
            {
                QVariant value = convertUAVariantToQVariant(var);
                mMaxMonitoredItemsPerCall = value.toInt();
            }

            UA_Variant_clear(&var);
            qDebug() << "++++++++++++mMaxMonitoredItemsPerCall:"
                     << UA_StatusCode_name(ret)
                     << mMaxMonitoredItemsPerCall;
        }

        // mClientConnected = true;
        setState(1);
        qDebug() << "++++++++++++client connect finished";
    }

    mTryRelease = false;

    return 0;
}

int OPCUAConnector::setNamespaceIndex(int idx)
{
    mDefaultNsIdx = idx;

    return 0;
}

int OPCUAConnector::release()
{
    if(state() != 1 || mClient == nullptr)
    {
        return 0;
    }

    // 这个大括号是用来控制locker的生命周期的
    {
        QMutexLocker locker(&mClientMutex);

        // // 释放
        // Delete the subscription
        // if(UA_Client_Subscriptions_deleteSingle(client, subId) == UA_STATUSCODE_GOOD)
        //     printf("Subscription removed\n");

        clearMonitor();

        UA_StatusCode ret = UA_Client_disconnect(mClient); // 断开连接并释放会话资源
        qInfo() << "UA_Client_disconnect" << UA_StatusCode_name(ret) << ret;
        UA_Client_delete(mClient); // 释放对象
        mClient = nullptr;

        setState(0);
    }

    // 由于线程锁的影响，释放完后，在线程中运行的周期函数还没执行
    // 因此这里需要稍微延时一会，让其执行完毕
    QThread::msleep(10);

    // 已经释放完毕
    mTryRelease = false;

    return 0;
}

int OPCUAConnector::readValue(QString path, QVariant &value, QString *errorString)
{
    QMutexLocker locker(&mClientMutex);

    if(mClient == nullptr)
    {
        return -1000;
    }

    QString nodeString = mPrefix + "." + path;
    UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(mDefaultNsIdx, nodeString.toStdString().data());

    UA_Variant var;
    UA_Variant_init(&var);
    UA_StatusCode ret = UA_Client_readValueAttribute(mClient,
                                                     nodeId,
                                                     &var);
    if(UA_StatusCode_isGood(ret))
    {
        value = convertUAVariantToQVariant(var);
    }
    else
    {
        qDebug() << "read value fail:"
                 << nodeString
                 << QString("0x%1").arg(QString::number(ret, 16));
    }

    // 凡是通过UA_NODEID_STRING_ALLOC，都要通过调用此函数清理
    UA_NodeId_clear(&nodeId);

    // 不清理的话会内存溢出
    // 释放 Variant 中的 data/arrayDimensions
    // 因为这个数据是UA_Client_readValueAttribute给allocate出来的
    // 交给我们后，我们需要对其进行管理
    UA_Variant_clear(&var);

    return ret;
}

QMap<BaseConnector::Type, const UA_DataType *> mQATypeMap ={
    {BaseConnector::Type::BOOLEAN, &UA_TYPES[UA_TYPES_BOOLEAN]},
    {BaseConnector::Type::SBYTE  , &UA_TYPES[UA_TYPES_SBYTE  ]},
    {BaseConnector::Type::BYTE   , &UA_TYPES[UA_TYPES_BYTE   ]},
    {BaseConnector::Type::INT16  , &UA_TYPES[UA_TYPES_INT16  ]},
    {BaseConnector::Type::UINT16 , &UA_TYPES[UA_TYPES_UINT16 ]},
    {BaseConnector::Type::INT32  , &UA_TYPES[UA_TYPES_INT32  ]},
    {BaseConnector::Type::UINT32 , &UA_TYPES[UA_TYPES_UINT32 ]},
    {BaseConnector::Type::INT64  , &UA_TYPES[UA_TYPES_INT64  ]},
    {BaseConnector::Type::UINT64 , &UA_TYPES[UA_TYPES_UINT64 ]},
    {BaseConnector::Type::FLOAT  , &UA_TYPES[UA_TYPES_FLOAT  ]},
    {BaseConnector::Type::DOUBLE , &UA_TYPES[UA_TYPES_DOUBLE ]},
    {BaseConnector::Type::STRING , &UA_TYPES[UA_TYPES_STRING ]},
    };

int OPCUAConnector::writeValue(QString path,
                               QVariant value,
                               Type type,
                               QString *errorString)
{
    QMutexLocker locker(&mClientMutex);

    if(mClient == nullptr)
    {
        return -1000;
    }

    // 经过QVaraint的智能转换一下，免得内存空间对不上
    QVariant tmpVar = toValidValue(value, type);

    QString nodeString = mPrefix + "." + path;
    UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(mDefaultNsIdx, nodeString.toStdString().data());

    UA_Variant var;
    UA_Variant_setScalar(&var,
                         tmpVar.data(),
                         mQATypeMap[type]);

    // 变量生存期的问题，不能直接放在后面大括号里面
    std::string str = tmpVar.toString().toStdString();
    UA_String uaStr;
    uaStr.length = str.length();
    uaStr.data   = (quint8*)str.data();
    if(type == Type::STRING)
    {
        var.data = &uaStr;
    }

    UA_StatusCode ret = UA_Client_writeValueAttribute(mClient,
                                                      nodeId,
                                                      &var);

    // UA_Variant_clear(&var);
    UA_NodeId_clear(&nodeId);
    if(UA_StatusCode_isGood(ret))
    {
        convertUAVariantToQVariant(var);

        // 写太快会导致opcua写失败,稍稍延时
        QThread::msleep(1);

        return 0;
    }
    else
    {
        qDebug() << "write value fail:"
                 << nodeString
                 << QString("0x%1").arg(QString::number(ret, 16));
    }

    return -1;
}

int OPCUAConnector::readValueList(QList<DataUnit> &dataList, QString *errorString)
{
    QMutexLocker locker(&mClientMutex);

    return __readValueList(dataList, errorString);
}

// 此函数存在内存泄漏风险，后续要跟踪一下
int OPCUAConnector::writeValueList(QList<DataUnit> dataList, QString *errorString)
{
    QMutexLocker locker(&mClientMutex);

    if(mClient == nullptr)
    {
        return -1000;
    }

    if(dataList.length() <= 0)
    {
        return -1;
    }

    // 根据每次能够发送的数据多少，分批发送
    QList<QList<DataUnit>> batchWriteList;
    int curIdx = 0;
    do{
        batchWriteList << dataList.mid(curIdx, mMaxNodesPerWrite);
        curIdx += mMaxNodesPerRead;
    }while(curIdx < dataList.length());

    // qDebug() << "batch write:" << batchWriteList.length() << mMaxNodesPerWrite;

    foreach (auto dataList, batchWriteList) {
        // 定义写的节点列表
        int valCount = dataList.length();
        // UA_WriteValue* wValArray = (UA_WriteValue*)UA_Array_new(valCount, &UA_TYPES[UA_TYPES_WRITEVALUE]);
        UA_WriteValue* wValArray = new UA_WriteValue[valCount];

        // 得把数据收集起来，最后统一释放
        QList<UA_NodeId> nodeIdList;
        QList<UA_String> uaStringList;
        for(int i = 0; i < valCount; i++)
        {
            UA_WriteValue_init(&wValArray[i]);
            wValArray[i].attributeId = UA_ATTRIBUTEID_VALUE;

            // 一定要用引用才能访问到原始数据
            DataUnit &dataItem = dataList[i];
            dataItem.value = toValidValue(dataItem.value, dataItem.type);

            QString nodeString = mPrefix + "." + dataItem.path;
            UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(mDefaultNsIdx, nodeString.toStdString().data());

            // qDebug() << nodeString;

            // 收集数据
            nodeIdList << nodeId;

            wValArray[i].nodeId = nodeIdList.last();

            UA_Variant var;
            UA_Variant_init(&var);
            UA_Variant_setScalar(&var,
                                 (void*)dataItem.value.data(),
                                 mQATypeMap[dataItem.type]);
            if(dataItem.type == STRING)
            {
                std::string str = dataItem.value.toString().toStdString();
                UA_String uaStr = UA_STRING_ALLOC(str.c_str());
                uaStringList << uaStr;
                UA_Variant_setScalar(&var, &uaStringList.last(), &UA_TYPES[UA_TYPES_STRING]);
            }

            wValArray[i].value.value = var;
            wValArray[i].value.hasValue = true;

            // 执行这个会崩溃
            // UA_Variant_clear(&var);
        }

        // for(int i = 0; i < valCount; i++)
        // {
        //     qDebug() << "data:" <<*(double*) wValArray[i].value.value.data;
        // }

        // 初始化写请求
        UA_WriteRequest wReq;
        UA_WriteRequest_init(&wReq);
        wReq.nodesToWrite = &wValArray[0];
        wReq.nodesToWriteSize = valCount;

        // 发送写请求
        UA_WriteResponse wResp = UA_Client_Service_write(mClient, wReq);
        auto retval = wResp.responseHeader.serviceResult;
        if (retval == UA_STATUSCODE_GOOD) {
            if (wResp.resultsSize == valCount) {
                for (int i = 0; i < valCount; ++i) {
                    if (wResp.results[i] == UA_STATUSCODE_GOOD) {
                        // qDebug("Node %d written successfully\n", i + 1);
                    } else {
                        qCritical() << QString("Node %1 write failed with status code %2")
                        .arg(i + 1)
                            .arg(QString::number(wResp.results[i], 16));
                    }
                }
            } else {
                qCritical("Unexpected number of results\n");
                return -2;
            }
        } else {
            qCritical() << QString("Write request failed with status code %1")
            .arg(QString::number(retval, 16));
            return -3;
        }

        // // 也无法执行
        // // 清理请求
        // UA_WriteRequest_clear(&wReq);

        // 清理响应
        UA_WriteResponse_clear(&wResp);

        // 清理
        for (int i = 0; i < nodeIdList.length(); ++i) {
            // // 执行了这个主对象的清理，就不需要其子成员清理了， 比如id，string
            // // 执行这个函数会报错
            // UA_WriteValue_clear(&wValArray[i]);

            // 这个也无法执行
            // UA_Variant_clear(&(wValArray[i].value.value));

            UA_NodeId_clear(&nodeIdList[i]);
        }
        for (int i = 0; i < uaStringList.length(); ++i) {
            UA_String_clear(&uaStringList[i]);
        }

        // UA_Array_delete(wValArray, valCount, &UA_TYPES[UA_TYPES_WRITEVALUE]);
        delete[] wValArray;
    }

    return 0;
}

int OPCUAConnector::monitorValues(QStringList pathList, QString *errorString)
{
    QMutexLocker locker(&mClientMutex);

    if(mClient == nullptr)
    {
        return -1;
    }
    if(state() != 1)
    {
        return -1;
    }

    int ret = 0;

#ifdef USE_OPCUA_SUBSCRIPTION
    QStringList failList;

    if(0)
    {
        // 逐个订阅
        for(int i = 0; i < pathList.length(); i++)
        {
            QString varName = pathList.at(i);

            // 不重复订阅
            if(monitoredValueList().contains(varName))
            {
                continue;
            }

            QString name = mPrefix + "." + varName;
            ret = subscribeVariant(mDefaultNsIdx, name, varName);
            if(ret != 0)
            {
                qDebug() << "订阅失败：" << varName;
                failList << varName;
            }
        }
    }
    else
    {
        // 批量订阅
        // mMaxMonitoredItemsPerCall

        // 去掉已经订阅的
        QList<QString> tmpStrList;
        for(int i = 0; i < pathList.length(); i++)
        {
            QString varName = pathList.at(i);
            if(monitoredValueList().contains(varName))
            {
                continue;
            }

            tmpStrList << varName;
        }

        QList<QMap<QString, QString>> subMapList;
        int curIdx = 0;
        do{
            QMap<QString, QString> varMap;
            QStringList tmpList = tmpStrList.mid(curIdx, mMaxMonitoredItemsPerCall);
            foreach (QString varName, tmpList) {
                QString name = mPrefix + "." + varName;
                varMap.insert(name, varName);
            }
            subMapList << varMap;

            curIdx += mMaxMonitoredItemsPerCall;
        }while(curIdx < tmpStrList.length());

        for(int i = 0; i < subMapList.length(); i++)
        {
            subscribeVariantList(mDefaultNsIdx, subMapList.at(i), failList);

            // // 这里稍作延时，留时间给回调函数做处理，
            // // 防止出现：Could not process a notification with clienthandle。。。。
            // QThread::msleep(100);
        }
    }

    if(failList.length() > 0)
    {
        qDebug() << "订阅失败：" << failList;

        ret = -2;
        if(errorString != nullptr)
        {
            foreach (QString str, failList) {
                *errorString += str + ",";
            }
        }
    }
#else
    foreach (QString varPath, pathList)
    {
        if(mMonVarMap.contains(varPath) == false)
        {
            mMonVarMap.insert(varPath, QVariant());
        }
    }
#endif

    return ret;
}

QStringList OPCUAConnector::monitoredValueList()
{
#ifdef USE_OPCUA_SUBSCRIPTION
    return mMonIdxMap.values();
#else
    return mMonVarMap.keys();
#endif
}

void OPCUAConnector::clearMonitor()
{
    if(mSubId != -1)
    {
        if(UA_Client_Subscriptions_deleteSingle(mClient, mSubId) == UA_STATUSCODE_GOOD)
        {
            qInfo("Subscription removed\n");
        }
        else
        {
            qCritical("Subscription remove fail\n");
        }
    }
    mMonIdxMap.clear();
    mSubId = -1;

    mMonVarMap.clear();
}

// 状态变化回调函数，可以通过这个监测客户端是否断开连接
// 此回调函数的线程，貌似就是UA_Client_run_iterate所在的线程？起始应该不是，恐怕是在哪个线程调用了UA_Client相关的函数，就在那个线程；
void OPCUAConnector::onStateChanged(UA_Client *client,
                                    UA_SecureChannelState channelState,
                                    UA_SessionState sessionState,
                                    UA_StatusCode connectStatus)
{
    UA_ClientConfig *config = UA_Client_getConfig(client);
    // qDebug() << "client context:" << config->clientContext;

    OPCUAConnector *com = (OPCUAConnector*)config->clientContext;


    // qInfo() << "--> status code:" << UA_StatusCode_name(connectStatus) << channelState << sessionState;

    // if(UA_StatusCode_name(connectStatus) == "BadDisconnect")
    // {

    // }

    // opcua状态从连接被动切换到掉线（网线被拔掉、PLC关机、或者其他原因）
    if(channelState == UA_SECURECHANNELSTATE_CLOSED) // 连接已断开
    {
        qDebug() << "callback thread:" << QThread::currentThread();
        qCritical() << "连接已断开--------"
                 << QDateTime::currentDateTime()
                 << com->state()
                 << sessionState
                 << com;

        // release中有UA_Client_delete，而这个函数是在client内部执行的，这样子操作相当于自杀。不能这样操作。释放要放在此回调函数之外才行
        // 貌似不是自杀的问题，是顺序的问题
        // if(sessionState == UA_SESSIONSTATE_CLOSED) // 不用加此判断
        {
            qCritical() << "x--> status code:" << UA_StatusCode_name(connectStatus) << channelState << sessionState;
            QMetaObject::invokeMethod(com, "release", Qt::QueuedConnection);
        }
    }

    if(connectStatus == UA_STATUSCODE_GOOD)
    {
        // channelState先走完流程，然后才轮到sessionState走流程；
        // 而只有sessionState走完流程，才可以进行读写、订阅操作
        if(sessionState == UA_SESSIONSTATE_ACTIVATED)
        {
            // 这里可以写重连完成、或者连接完成后的初始化操作
            qDebug() << "opcua connect complete";
        }
    }

}

void subscriptionInactivity(UA_Client *client,
                            UA_UInt32 subscriptionId,
                            void *subContext)
{
    // 重新订阅？

    qCritical() << "---------------------->opcua subscriptionInactivity";
}


// 创建一个函数模板
template<typename T>
QVariant UAValueToQVariant(const UA_Variant &data) {
    return *(T*)data.data;
}

// 创建一个映射
std::map<UA_DataTypeKind, QVariant(*)(const UA_Variant &data)> UAQTConvertor = {
    {UA_DATATYPEKIND_BOOLEAN,  UAValueToQVariant<bool>},
    {UA_DATATYPEKIND_SBYTE  ,  UAValueToQVariant<qint8>},
    {UA_DATATYPEKIND_BYTE   ,  UAValueToQVariant<quint8>},
    {UA_DATATYPEKIND_INT16  ,  UAValueToQVariant<qint16>},
    {UA_DATATYPEKIND_UINT16 ,  UAValueToQVariant<quint16>},
    {UA_DATATYPEKIND_INT32  ,  UAValueToQVariant<qint32>},
    {UA_DATATYPEKIND_UINT32 ,  UAValueToQVariant<quint32>},
    {UA_DATATYPEKIND_INT64  ,  UAValueToQVariant<qint64>},
    {UA_DATATYPEKIND_UINT64 ,  UAValueToQVariant<quint64>},
    {UA_DATATYPEKIND_FLOAT  ,  UAValueToQVariant<float>},
    {UA_DATATYPEKIND_DOUBLE ,  UAValueToQVariant<double>},
    // {UA_DATATYPEKIND_STRING ,  UAValueToQVariant<char*>}, // 需要特殊处理
    };

QVariant convertUAVariantToQVariant(const UA_Variant &data) {
    UA_DataTypeKind typeKind = (UA_DataTypeKind)data.type->typeKind;
    auto it = UAQTConvertor.find(typeKind);
    if (it != UAQTConvertor.end()) {
        // 如果找到了对应的转换函数，则调用它
        return it->second(data);
    }
    else
    {
        if(typeKind == UA_DATATYPEKIND_STRING)
        {
            QString str;
            UA_String *uaStr = (UA_String *)data.data;
            if(uaStr->length != 0)
            {
                str = QString(QByteArray((char*)uaStr->data, uaStr->length));
            }
            return str;
        }
        else
        {
            // 如果没有找到对应的转换函数，可以返回一个默认值或抛出异常
            qDebug() << "no type convertor:" << data.type->typeName;
            return QVariant();
        }
    }
}
// 监视的变量发生变化
void OPCUAConnector::handler_paramsChanged(UA_Client *client,
                                           UA_UInt32 subId,
                                           void *subContext,
                                           UA_UInt32 monId,
                                           void *monContext,
                                           UA_DataValue *value)
{
    if(monContext != NULL && value != NULL)
    {
        if(value->hasValue == false)
        {
            return;
        }
        OPCUAConnector *obj = (OPCUAConnector*)monContext;

        QString monName = obj->mMonIdxMap[monId];

        // switch (value->value.type->typeKind) {
        // case UA_DATATYPEKIND_BOOLEAN:

        //     break;
        // default:
        //     break;
        // }
        // QString str;
        // UA_String *uaStr = (UA_String *)value->value.data;
        // if(uaStr->length != 0)
        // {
        //     str = QString(QByteArray((char*)uaStr->data, uaStr->length));
        // }

        emit obj->valueChanged(monName, convertUAVariantToQVariant(value->value));
    }
}

// https://blog.csdn.net/xhydongda/article/details/131847493
// https://www.cnblogs.com/davisdabing/p/17841124.html
// 最短采样间隔：设置 OPC UA 服务器记录 CPU 变量值并与以前值相比较检查是否发生变更的时间间隔。
// 最短发布间隔：变量值发生改变时,服务器将新值向客户端发送消息的时间间隔。
int OPCUAConnector::subscribeVariant(int nsIdx, QString name, QString desc)
{
    // 要弄清楚 subscribe 和 monitor 的区别
    // 建立一次Subscription后，可以在这个Subscription上，进行多个变量的monitor
    // 每次的Subscription，都有一些参数可以设定。由于monitor是建立在subscribe之上的，因此会继承subscribe的一些属性
    // 可以简单理解为分组。分组管理的话，方便批量控制。
    if(mSubId == -1)
    {
        // PublishingInterval：数据发布的间隔时间。
        // LifetimeCount：服务器在判定订阅不活跃前，允许丢失的最大心跳包数。
        // MaxKeepAliveCount：服务器发送心跳包的最大次数（若在此期间无数据变化，发送空通知）
        // 关键规则：LifetimeCount > MaxKeepAliveCount（通常设为 3-5 倍）
        /* Create a subscription */
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        request.requestedPublishingInterval = 1; // ---配合后面，好像服务器限制，最小好像是100ms
        // request.requestedMaxKeepAliveCount  = 10; // 10
        // request.requestedLifetimeCount = request.requestedMaxKeepAliveCount * 5; // 最小只能设置 MaxKeepAliveCount * 3
        /* uaexpert的配方*/
        // request.requestedLifetimeCount = 2400;
        // request.requestedMaxKeepAliveCount = 10;
        // request.priority = 0;
        // request.maxNotificationsPerPublish = 0;
        UA_CreateSubscriptionResponse response = UA_Client_Subscriptions_create(mClient, request,
                                                                                NULL, NULL, NULL);
        qDebug() << "revised sub data:"
                 << response.revisedPublishingInterval // 实际的发布周期 ms
                 << response.revisedLifetimeCount      // 创建订阅后，必须要在多少个周期内发起monitor，否则关闭此订阅
                 << response.revisedMaxKeepAliveCount  // 最多服务器跳过多少次无数据的发布
            ;

        mSubId = response.subscriptionId;
        if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        {
            qDebug("Create subscription succeeded, id %u\n", mSubId);
        }
        else
        {
            qDebug() << "Create subscription fail:" << mSubId;
            mSubId = -1;
            return -1;
        }
    }

    // 这个 requestedParameters.samplingInterval 要和前面的 request.requestedPublishingInterval 一起配合使用才有效果
    auto nodeId = UA_NODEID_STRING_ALLOC(nsIdx, name.toStdString().data());
    UA_MonitoredItemCreateRequest monRequest =
        UA_MonitoredItemCreateRequest_default(nodeId);
    monRequest.requestedParameters.samplingInterval = 10; // --配合前面， 好像服务器限制，最小好像是100ms
    monRequest.requestedParameters.queueSize = 1; // 10
    // monRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;
    UA_MonitoredItemCreateResult monResponse =
        UA_Client_MonitoredItems_createDataChange(mClient, mSubId,
                                                  UA_TIMESTAMPSTORETURN_BOTH,
                                                  monRequest, (void*)this, handler_paramsChanged, NULL);
    qDebug() << "subs res code:"
             << nsIdx
             << name
             << QString::number(monResponse.statusCode, 16);
    qDebug() << "revised mon data:" << monResponse.revisedSamplingInterval << monResponse.revisedQueueSize;

    UA_NodeId_clear(&nodeId);

    if(monResponse.statusCode == UA_STATUSCODE_GOOD)
    {
        // qDebug("Monitoring %s, id %u\n", name.toStdString().c_str(), monResponse.monitoredItemId);

        mMonIdxMap.insert(monResponse.monitoredItemId, desc);
    }
    else
    {
        qDebug() << "订阅失败 Monitoring fail:" << name;
        return -2;
    }

    return 0;
}

// https://blog.csdn.net/m0_47722349/article/details/123323317
int OPCUAConnector::subscribeVariantList(int nsIdx, QMap<QString, QString> varMap, QStringList &failList)
{
    int ret = 0;
    if(mSubId == -1)
    {
        UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
        request.requestedPublishingInterval = 1; // ---配合后面，好像服务器限制，最小好像是100ms

        UA_CreateSubscriptionResponse response =
            UA_Client_Subscriptions_create(mClient, request, NULL, NULL, NULL);

        mSubId = response.subscriptionId;
        if(response.responseHeader.serviceResult == UA_STATUSCODE_GOOD)
        {
            qDebug("Create subscription succeeded, id %u\n", mSubId);
        }
        else
        {
            qDebug() << "Create subscription fail:" << mSubId;
            mSubId = -1;
            return -1;
        }
    }

    std::vector<UA_MonitoredItemCreateRequest> reqList;
    QStringList nameList = varMap.keys();
    for (int i = 0; i < nameList.length(); ++i) {
        QString name = nameList.at(i);
        auto nodeId = UA_NODEID_STRING_ALLOC(nsIdx, name.toStdString().data());
        UA_MonitoredItemCreateRequest monRequest = UA_MonitoredItemCreateRequest_default(nodeId);
        monRequest.requestedParameters.samplingInterval = 10;
        monRequest.requestedParameters.queueSize = 2;
        // monRequest.monitoringMode = UA_MONITORINGMODE_REPORTING;

        reqList.push_back(monRequest);
    }

    UA_CreateMonitoredItemsRequest monRequest;
    UA_CreateMonitoredItemsRequest_init(&monRequest);
    monRequest.subscriptionId = mSubId;
    monRequest.timestampsToReturn = UA_TIMESTAMPSTORETURN_BOTH;
    monRequest.itemsToCreate = (UA_MonitoredItemCreateRequest*)reqList.data();
    monRequest.itemsToCreateSize = nameList.length();

    std::vector<void*> contextList;
    std::vector<UA_Client_DataChangeNotificationCallback> callbackList;
    std::vector<UA_Client_DeleteMonitoredItemCallback> deleteCallbackList;
    for (int i = 0; i < nameList.length(); ++i) {
        contextList.push_back((void*)this);
        callbackList.push_back(handler_paramsChanged);
        deleteCallbackList.push_back(NULL);
    }

    UA_CreateMonitoredItemsResponse monResponse =
        UA_Client_MonitoredItems_createDataChanges(mClient,
                                                   monRequest,
                                                   contextList.data(),
                                                   callbackList.data(),
                                                   deleteCallbackList.data());

    for(int i = 0; i < monResponse.resultsSize; i++)
    {
        QString name = nameList.at(i);
        UA_MonitoredItemCreateResult *result = &monResponse.results[i];

        if(result->statusCode == UA_STATUSCODE_GOOD)
        {
            // qDebug("Monitoring success, %s, id %u",
            //        name.toStdString().data(),
            //        result->monitoredItemId);
            mMonIdxMap.insert(result->monitoredItemId, varMap[name]);
        }
        else
        {
            qCritical() << "--->monitor fail:" << name;
            failList << name;
            ret = -1000;
            continue;
        }
    }

    qDebug() << "monitor finished:" << ret << nameList.length();

    UA_CreateMonitoredItemsResponse_clear(&monResponse);

    return ret;
}

int OPCUAConnector::customSubsProcess()
{
    int ret = 0;

    QStringList varPathList = mMonVarMap.keys();
    if(varPathList.length() == 0)
    {
        return -1;
    }

    // 初始化数据
    QList<DataUnit> dataUnitList;
    for (int i = 0; i < varPathList.length(); ++i) {
        DataUnit unit;
        unit.path = varPathList.at(i);

        dataUnitList << unit;
    }

    // 读取数据
    ret = __readValueList(dataUnitList);
    if(ret != 0)
    {
        return -1000;
    }

    // 比较数据
    for (int i = 0; i < dataUnitList.length(); ++i) {
        DataUnit &unit = dataUnitList[i];
        if(mMonVarMap[unit.path] != unit.value)
        {
            mMonVarMap[unit.path] = unit.value;
            emit valueChanged(unit.path, unit.value);
        }
    }

    return ret;
}

int OPCUAConnector::__readValueList(QList<DataUnit> &dataList, QString *errorString)
{
    if(mClient == nullptr)
    {
        return -1000;
    }

    // 缓存读取到的数据
    QMap<QString, QVariant> tmpVarMap;

    // 统计需要读取的数据
    QStringList varPathList;
    for (int i = 0; i < dataList.length(); ++i) {
        DataUnit unit = dataList.at(i);
        varPathList << unit.path;
    }

    if(varPathList.length() == 0)
    {
        return -1;
    }

    // qDebug() << varPathList << varPathList.length();

    // 根据MaxNodesPerRead，来分批读取
    QList<QStringList> batchReadList;
    int curIdx = 0;
    do{
        batchReadList << varPathList.mid(curIdx, mMaxNodesPerRead);
        curIdx += mMaxNodesPerRead;
    }while(curIdx < varPathList.length());

    if(batchReadList.length() > 0)
    {
        for(int batchIdx = 0; batchIdx < batchReadList.length(); batchIdx++)
        {
            QStringList tmpList = batchReadList.at(batchIdx);

            UA_ReadRequest request;
            UA_ReadRequest_init(&request);

            // 定义要读取的节点列表
            int idsCount = tmpList.length();
            // UA_ReadValueId *ids = new UA_ReadValueId[idsCount];
            UA_ReadValueId* ids = (UA_ReadValueId*)UA_Array_new(idsCount, &UA_TYPES[UA_TYPES_READVALUEID]);

            for(int i = 0; i < idsCount; i++)
            {
                UA_ReadValueId_init(&ids[i]);

                QString nodeString = mPrefix + "." + tmpList.at(i);
                UA_NodeId nodeId = UA_NODEID_STRING_ALLOC(mDefaultNsIdx, nodeString.toStdString().data());

                ids[i].nodeId = nodeId;
                ids[i].attributeId = UA_ATTRIBUTEID_VALUE;
            }

            request.nodesToRead     = ids;
            request.nodesToReadSize = idsCount;

            // 执行批量读取
            UA_ReadResponse response = UA_Client_Service_read(mClient, request);
            if (response.responseHeader.serviceResult == UA_STATUSCODE_GOOD) {
                for (size_t i = 0; i < response.resultsSize; i++) {
                    // 处理每个节点的值
                    UA_Variant uaVar = response.results[i].value;
                    QString varPath = tmpList.at(i);
                    // qDebug() << varPath;
                    if(UA_Variant_isEmpty(&uaVar) == false)
                    {
                        QVariant var = convertUAVariantToQVariant(uaVar);
                        // qDebug() << varPath << var;

                        tmpVarMap[varPath] = var;
                    }
                    else
                    {
                        QString info = QString("read controller value fail:") + "\r\n" +
                                       varPath + "\r\n" +
                                       QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
                        qCritical() << info;
                        emit errorOccured(info);
                        return -1001;
                    }
                }
            }
            else
            {
                qCritical() << "multi read error:"
                            << QString::number(response.responseHeader.serviceResult, 16).toUpper()
                            << tmpList << batchReadList.length();

                return -1002;
            }
            UA_ReadResponse_clear(&response);

            // qDebug() << "multi read values:" << response.responseHeader.serviceResult;

            for(int i = 0; i < idsCount; i++)
            {
                UA_ReadValueId_clear(&ids[i]);
            }
            // delete[] ids;
            UA_Array_delete(ids, idsCount, &UA_TYPES[UA_TYPES_READVALUEID]);
        }
    }

    // 将数据填充回到list
    for (int i = 0; i < dataList.length(); ++i) {
        DataUnit unit = dataList.at(i);
        dataList[i].value = tmpVarMap[unit.path];
    }

    return 0;
}

// https://blog.csdn.net/love_xiaoqiner/article/details/129864786
// 自定义日志处理函数（空实现，不输出任何内容）
void customLogger(void *context,
                         UA_LogLevel level,
                         UA_LogCategory category,
                         const char *msg,
                         va_list args) {
    // 可在此添加条件判断，例如屏蔽低于某个级别的日志
    // if (level < UA_LOGLEVEL_WARNING) return;
}

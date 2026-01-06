#ifndef OPCUACONNECTOR_H
#define OPCUACONNECTOR_H

#include "baseconnector.h"

#include "open62541.h"
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QFuture>

#ifdef Q_OS_WIN
#pragma execution_character_set("utf-8")
#endif

// #define USE_OPCUA_SUBSCRIPTION

class OPCUAConnector : public BaseConnector
{
    Q_OBJECT

public:
    explicit OPCUAConnector(QObject *parent = nullptr);
    ~OPCUAConnector();

    // 初始化
    // 传递两个字符串进来；第一个是连接的主机信息，第二个是变量前缀
    int init(QStringList paramList);

    // 设置默认命名空间序号
    int setNamespaceIndex(int idx = 4);

    // 释放资源，释放后，不能进行读写操作
    Q_INVOKABLE int release();

    // 从PLC读取数据
    int readValue(QString path,
                  QVariant &value,
                  QString *errorString = nullptr);

    // 将数据写入PLC
    int writeValue(QString path,
                   QVariant value,
                   Type type,
                   QString *errorString = nullptr);

    // 批量从PLC读取数据
    int readValueList(QList<DataUnit> &dataList,
                      QString *errorString = nullptr);

    // 批量将数据写入PLC
    int writeValueList(QList<DataUnit> dataList,
                       QString *errorString = nullptr);

    // 监听数据，即告知PLC要监听哪些数据
    int monitorValues(QStringList pathList,
                      QString *errorString = nullptr);

    // 有哪些数据已经被监控
    QStringList monitoredValueList();

    void clearMonitor();

signals:

private:
    // 连接状态变化
    static void onStateChanged(UA_Client *client,
                               UA_SecureChannelState channelState,
                               UA_SessionState sessionState,
                               UA_StatusCode connectStatus);
    // 监视的变量发生变化
    static void handler_paramsChanged(UA_Client *client,
                                      UA_UInt32 subId,
                                      void *subContext,
                                      UA_UInt32 monId,
                                      void *monContext,
                                      UA_DataValue *value);
    // 单个订阅
    int subscribeVariant(int nsIdx, QString name, QString desc);

    // 批量订阅
    int subscribeVariantList(int nsIdx, QMap<QString, QString> varMap, QStringList &failList);

    // 不带线程锁
    int __readValueList(QList<DataUnit> &dataList,
                        QString *errorString = nullptr);

    // 批量读取变量，用于替代opcua自身的订阅
    int customSubsProcess();

private:
    QString mHostInfo;
    QString mPrefix;

    UA_Client *mClient = nullptr;
    QMutex mClientMutex;

    QFuture<void> mFunture;

    bool mTryRelease = false;
    bool mExited = false;

    int mDefaultNsIdx = 4;
    qint32 mSubId = -1;
    QMap<qint32, QString> mMonIdxMap;

    quint32 mMaxNodesPerRead = 1;
    quint32 mMaxNodesPerWrite = 1;
    quint32 mMaxMonitoredItemsPerCall = 1;
    QMap<QString, QVariant> mMonVarMap;
};

#endif // OPCUACONNECTOR_H

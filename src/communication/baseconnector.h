#ifndef BASECONNECTOR_H
#define BASECONNECTOR_H

#include <QObject>
#include <QVariant>
#include <QThread>
#include <QCoreApplication>
#include <QtConcurrentRun>

class BaseConnector : public QObject
{
    Q_OBJECT

    // -1：未初始化 0:未连接 1：已连接
    Q_PROPERTY(int state READ state WRITE setState NOTIFY stateChanged FINAL)

public:
    enum Type {
        BOOLEAN = 0,
        SBYTE,
        BYTE,
        INT16,
        UINT16,
        INT32,
        UINT32,
        INT64,
        UINT64,
        FLOAT,
        DOUBLE,
        STRING
    };
    struct DataUnit{
        QString path;
        QVariant value;
        Type type;
    };

    explicit BaseConnector(QObject *parent = nullptr);

    // 初始化
    virtual int init(QStringList paramList) = 0;

    // 释放资源，释放后，不能进行读写操作
    virtual int release() = 0;

    // 从PLC读取数据
    virtual int readValue(QString path,
                          QVariant &value,
                          QString *errorString = nullptr) = 0;

    // 将数据写入PLC
    virtual int writeValue(QString path,
                           QVariant value,
                           Type type,
                           QString *errorString = nullptr) = 0;

    // 批量从PLC读取数据
    virtual int readValueList(QList<DataUnit> &dataList,
                              QString *errorString = nullptr) = 0;

    // 批量将数据写入PLC
    virtual int writeValueList(QList<DataUnit> dataList,
                               QString *errorString = nullptr) = 0;

    // 监听数据，即告知PLC要监听哪些数据
    virtual int monitorValues(QStringList pathList,
                              QString *errorString = nullptr) = 0;

    // 有哪些数据已经被监控
    virtual QStringList monitoredValueList() = 0;

    // 清除对变量的监听
    virtual void clearMonitor() = 0;

public:
    int state() const;
    void setState(int newState);

signals:
    void valueChanged(QString valuePath, QVariant value);
    void stateChanged();
    void errorOccured(QString err);

protected:
    QVariant toValidValue(QVariant value, BaseConnector::Type type);

private:
    int m_state;
};

#include <QTimer>
#include <QDebug>
// 自定义线程
class MyThreadPrivate : public QThread
{
public:
    MyThreadPrivate(std::function<void()> func)
    {
        mFunc = func;

        // 完成之后，执行析构
        connect(this, &MyThreadPrivate::finished, this, &MyThreadPrivate::deleteLater);
    }
    ~MyThreadPrivate(){
        qWarning() << "~MyThreadPrivate";
    }

protected:
    void run()
    {
        if(mFunc)
        {
            mFunc();
            // qWarning() << "mFunc finished";
        }
    }

private:
    std::function<void()> mFunc;
};

namespace MyThread {
// template <typename Function>
// QThread *run(Function &&f){
//     QThread* thread = QThread::create(f);
//     // thread->setPriority(QThread::LowPriority);
//     thread->start();
//     return thread;
// }

// // 在嵌入式中貌似会出问题，会在启动一定数量后，新的不启动
// // 需要设置一下QThreadPool::globalInstance()->setMaxThreadCount(100);
// template <typename Function>
// auto run(Function &&f)
// {
//     return QtConcurrent::run(f);
// }

// 这个貌似在很多线程时会导致脚本构建/析构出问题，原因不明
// 使用QtConcurrent::run时还是会出现这个问题，但其是过一段时间后才出现。
// 也就是说，线程在析构时才会出问题？
// 经过测试，线程会在退出前，有可能会被重复使用。
template <typename Function>
auto run(Function &&f)
{
    MyThreadPrivate *myThread = new MyThreadPrivate(f);
    // 使用 QThread::TimeCriticalPriority 效果还可以
    myThread->start(QThread::TimeCriticalPriority);

    return myThread;
}

}

#endif // BASECONNECTOR_H

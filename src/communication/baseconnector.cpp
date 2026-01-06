#include "baseconnector.h"

BaseConnector::BaseConnector(QObject *parent)
    : QObject{parent}
{
    setState(-1);
}


QVariant BaseConnector::toValidValue(QVariant value, BaseConnector::Type type)
{
    QVariant tmpVar;

    switch (type) {
    case BaseConnector::Type::BOOLEAN:
        tmpVar = value.value<bool>();
        break;
    case BaseConnector::Type::SBYTE:
        tmpVar = value.value<qint8>();
        break;
    case BaseConnector::Type::BYTE:
        tmpVar = value.value<quint8>();
        break;
    case BaseConnector::Type::INT16:
        tmpVar = value.value<qint16>();
        break;
    case BaseConnector::Type::UINT16:
        tmpVar = value.value<quint16>();
        break;
    case BaseConnector::Type::INT32:
        tmpVar = value.value<qint32>();
        break;
    case BaseConnector::Type::UINT32:
        tmpVar = value.value<quint32>();
        break;
    case BaseConnector::Type::INT64:
        tmpVar = value.value<qint64>();
        break;
    case BaseConnector::Type::UINT64:
        tmpVar = value.value<quint64>();
        break;
    case BaseConnector::Type::FLOAT:
        tmpVar = value.value<float>();
        break;
    case BaseConnector::Type::DOUBLE:
        tmpVar = value.value<double>();
        break;
    case BaseConnector::Type::STRING:
        tmpVar = value;
        break;
    default:
        break;
    }

    return tmpVar;
}

int BaseConnector::state() const
{
    return m_state;
}

void BaseConnector::setState(int newState)
{
    if (m_state == newState)
        return;
    m_state = newState;
    emit stateChanged();
}

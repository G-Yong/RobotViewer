#ifndef OPCUABINDINGMODEL_H
#define OPCUABINDINGMODEL_H

#pragma execution_character_set("utf-8")

#include <QVariantList>
#include <QList>
#include <QStringList>

#include "commontypes.h"

struct OpcuaBindingRow {
    QString jointName;
    QString nodeId;
    bool enabled = true;
};

/**
 * @brief 管理OPC UA绑定列表（便于在QML/C++之间同步）。
 */
class OpcuaBindingModel
{
public:
    OpcuaBindingModel() = default;

    QVariantList toVariantList() const;
    void fromSettings(const QList<OpcuaBinding>& bindings);
    QList<OpcuaBinding> toSettings() const;

    void clear();
    void addBinding(const QStringList& availableJointNames);
    void removeBinding(int index);
    void updateBinding(int index, const QString& jointName,
                       const QString& nodeId, bool enabled);

    const QList<OpcuaBindingRow>& rows() const { return m_rows; }

private:
    bool isIndexValid(int index) const;

    QList<OpcuaBindingRow> m_rows;
};

#endif // OPCUABINDINGMODEL_H

#include "opcuabindingmodel.h"

QVariantList OpcuaBindingModel::toVariantList() const
{
    QVariantList list;
    for (const auto& row : m_rows) {
        QVariantMap map;
        map["jointName"] = row.jointName;
        map["nodeId"] = row.nodeId;
        map["enabled"] = row.enabled;
        list.append(map);
    }
    return list;
}

void OpcuaBindingModel::fromSettings(const QList<OpcuaBinding>& bindings)
{
    m_rows.clear();
    for (const auto& binding : bindings) {
        OpcuaBindingRow row;
        row.jointName = binding.jointName;
        row.nodeId = binding.opcuaNodeId;
        row.enabled = binding.enabled;
        m_rows.append(row);
    }
}

QList<OpcuaBinding> OpcuaBindingModel::toSettings() const
{
    QList<OpcuaBinding> list;
    for (const auto& row : m_rows) {
        OpcuaBinding binding;
        binding.jointName = row.jointName;
        binding.opcuaNodeId = row.nodeId;
        binding.enabled = row.enabled;
        list.append(binding);
    }
    return list;
}

void OpcuaBindingModel::clear()
{
    m_rows.clear();
}

void OpcuaBindingModel::addBinding(const QStringList& availableJointNames)
{
    OpcuaBindingRow row;
    row.jointName = availableJointNames.isEmpty() ? "" : availableJointNames.first();
    row.nodeId = "";
    row.enabled = true;
    m_rows.append(row);
}

void OpcuaBindingModel::removeBinding(int index)
{
    if (!isIndexValid(index)) return;
    m_rows.removeAt(index);
}

void OpcuaBindingModel::updateBinding(int index, const QString& jointName,
                                      const QString& nodeId, bool enabled)
{
    if (!isIndexValid(index)) return;

    m_rows[index].jointName = jointName;
    m_rows[index].nodeId = nodeId;
    m_rows[index].enabled = enabled;
}

bool OpcuaBindingModel::isIndexValid(int index) const
{
    return index >= 0 && index < m_rows.size();
}

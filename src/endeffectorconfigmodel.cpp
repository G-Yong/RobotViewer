#include "endeffectorconfigmodel.h"

#include <QColor>
#include <QStringList>

QVariantList EndEffectorConfigModel::toVariantList() const
{
    QVariantList list;
    for (const auto& row : m_rows) {
        QVariantMap map;
        map["linkName"] = row.linkName;
        map["displayName"] = row.displayName;
        map["colorHex"] = row.colorHex;
        map["enabled"] = row.enabled;
        list.append(map);
    }
    return list;
}

void EndEffectorConfigModel::fromSettings(const QList<EndEffectorConfig>& configs)
{
    m_rows.clear();
    for (const auto& config : configs) {
        EndEffectorConfigRow row;
        row.linkName = config.linkName;
        row.displayName = config.displayName;
        row.colorHex = config.colorHex;
        row.enabled = config.enabled;
        m_rows.append(row);
    }
}

QList<EndEffectorConfig> EndEffectorConfigModel::toSettings() const
{
    QList<EndEffectorConfig> list;
    for (const auto& row : m_rows) {
        EndEffectorConfig config;
        config.linkName = row.linkName;
        config.displayName = row.displayName;
        config.colorHex = row.colorHex;
        config.enabled = row.enabled;
        list.append(config);
    }
    return list;
}

bool EndEffectorConfigModel::addConfig(const QString& linkName,
                                       const QString& displayName,
                                       const QString& colorHex,
                                       QString* error)
{
    if (linkName.isEmpty()) {
        if (error) *error = "链接名称不能为空";
        return false;
    }

    if (containsLink(linkName)) {
        if (error) *error = "该链接已配置为末端执行器";
        return false;
    }

    QString pickedColor = pickColorForIndex(m_rows.size(), colorHex);
    EndEffectorConfigRow row;
    row.linkName = linkName;
    row.displayName = displayName.isEmpty() ? linkName : displayName;
    row.colorHex = pickedColor;
    row.enabled = true;
    m_rows.append(row);
    return true;
}

void EndEffectorConfigModel::removeConfig(int index)
{
    if (!isIndexValid(index)) return;
    m_rows.removeAt(index);
}

void EndEffectorConfigModel::updateConfig(int index, const QString& linkName,
                                          const QString& displayName,
                                          const QString& colorHex, bool enabled)
{
    if (!isIndexValid(index)) return;

    m_rows[index].linkName = linkName;
    m_rows[index].displayName = displayName;
    m_rows[index].colorHex = colorHex;
    m_rows[index].enabled = enabled;
}

void EndEffectorConfigModel::clear()
{
    m_rows.clear();
}

bool EndEffectorConfigModel::containsLink(const QString& linkName) const
{
    for (const auto& row : m_rows) {
        if (row.linkName == linkName) {
            return true;
        }
    }
    return false;
}

QString EndEffectorConfigModel::pickColorForIndex(int index, const QString& fallback) const
{
    if (!fallback.isEmpty()) {
        QColor color(fallback);
        if (color.isValid()) {
            return fallback;
        }
    }

    static const QStringList defaultColors = {
        "#FFFF00", "#00FFFF", "#FF00FF", "#FF8000",
        "#80FF00", "#00FF80", "#8000FF", "#FF0080"
    };
    if (defaultColors.isEmpty()) return "#FFFF00";

    const int colorIndex = index % defaultColors.size();
    return defaultColors[colorIndex];
}

bool EndEffectorConfigModel::isIndexValid(int index) const
{
    return index >= 0 && index < m_rows.size();
}

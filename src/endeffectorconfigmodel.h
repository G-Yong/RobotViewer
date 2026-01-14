#ifndef ENDEFFECTORCONFIGMODEL_H
#define ENDEFFECTORCONFIGMODEL_H

#pragma execution_character_set("utf-8")

#include <QVariantList>
#include <QList>
#include <QString>

#include "commontypes.h"

struct EndEffectorConfigRow {
    QString linkName;
    QString displayName;
    QString colorHex;
    bool enabled = true;
};

/**
 * @brief 末端执行器配置的集中管理与校验。
 */
class EndEffectorConfigModel
{
public:
    EndEffectorConfigModel() = default;

    QVariantList toVariantList() const;
    void fromSettings(const QList<EndEffectorConfig>& configs);
    QList<EndEffectorConfig> toSettings() const;

    bool addConfig(const QString& linkName,
                   const QString& displayName,
                   const QString& colorHex,
                   QString* error);
    void removeConfig(int index);
    void updateConfig(int index, const QString& linkName,
                      const QString& displayName,
                      const QString& colorHex, bool enabled);
    void clear();

    const QList<EndEffectorConfigRow>& rows() const { return m_rows; }
    bool containsLink(const QString& linkName) const;

private:
    QString pickColorForIndex(int index, const QString& fallback) const;
    bool isIndexValid(int index) const;

    QList<EndEffectorConfigRow> m_rows;
};

#endif // ENDEFFECTORCONFIGMODEL_H

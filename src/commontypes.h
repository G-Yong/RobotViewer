#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#pragma execution_character_set("utf-8")

#include <QString>
#include <QColor>

/**
 * @brief OPC UA变量绑定项
 * 共享结构体，用于Widget UI和QML UI
 */
struct OpcuaBinding {
    QString jointName;      // 关节名称
    QString opcuaNodeId;    // OPC UA节点ID
    bool enabled = true;    // 是否启用
};

/**
 * @brief 末端执行器配置项
 * 用于配置轨迹显示的末端执行器
 */
struct EndEffectorConfig {
    QString linkName;       // 链接名称
    QString displayName;    // 显示名称（可选）
    QString colorHex;       // 轨迹颜色（十六进制，如 "#FF0000"）
    bool enabled = true;    // 是否启用轨迹显示
};

#endif // COMMONTYPES_H

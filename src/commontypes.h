#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#pragma execution_character_set("utf-8")

#include <QString>

/**
 * @brief OPC UA变量绑定项
 * 共享结构体，用于Widget UI和QML UI
 */
struct OpcuaBinding {
    QString jointName;      // 关节名称
    QString opcuaNodeId;    // OPC UA节点ID
    bool enabled = true;    // 是否启用
};

#endif // COMMONTYPES_H

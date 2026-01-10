import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import "../components"

// OPC UA通讯面板 - 修复布局
Item {
    id: root
    
    property var robotBridge: null
    
    // 使用Flickable实现滚动
    Flickable {
        anchors.fill: parent
        contentHeight: mainColumn.height
        clip: true
        
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
        
        Column {
            id: mainColumn
            width: parent.width
            spacing: 20
            
            // ===== 连接设置区 =====
            GlassPanel {
                width: parent.width
                height: connectionContent.height + 40
                glassOpacity: 0.85
                cornerRadius: 12
                
                Column {
                    id: connectionContent
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 20
                    spacing: 18
                    
                    // 标题行
                    Row {
                        spacing: 10
                        Text {
                            text: "📡"
                            font.pixelSize: 18
                        }
                        Text {
                            text: qsTr("连接设置")
                            color: "#ffffff"
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }
                    }
                    
                    // 服务器地址
                    Column {
                        width: parent.width
                        spacing: 8
                        
                        Text {
                            text: qsTr("服务器地址")
                            color: "#b0ffffff"
                            font.pixelSize: 13
                        }
                        
                        Rectangle {
                            width: parent.width
                            height: 44
                            radius: 8
                            color: "#25ffffff"
                            border.color: serverUrlInput.activeFocus ? "#00ff88" : "#40ffffff"
                            border.width: 1
                            
                            TextInput {
                                id: serverUrlInput
                                anchors.fill: parent
                                anchors.margins: 14
                                text: robotBridge ? robotBridge.opcuaServerUrl : ""
                                color: "#ffffff"
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                selectByMouse: true
                                
                                onTextChanged: {
                                    if (robotBridge) robotBridge.opcuaServerUrl = text
                                }
                            }
                        }
                    }
                    
                    // 节点前缀
                    Column {
                        width: parent.width
                        spacing: 8
                        
                        Text {
                            text: qsTr("节点前缀")
                            color: "#b0ffffff"
                            font.pixelSize: 13
                        }
                        
                        Rectangle {
                            width: parent.width
                            height: 44
                            radius: 8
                            color: "#25ffffff"
                            border.color: prefixInput.activeFocus ? "#00ff88" : "#40ffffff"
                            border.width: 1
                            
                            TextInput {
                                id: prefixInput
                                anchors.fill: parent
                                anchors.margins: 14
                                text: robotBridge ? robotBridge.opcuaPrefix : ""
                                color: "#ffffff"
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                selectByMouse: true
                                
                                onTextChanged: {
                                    if (robotBridge) robotBridge.opcuaPrefix = text
                                }
                            }
                        }
                    }
                    
                    // 采样间隔和命名空间 - 使用Row而非RowLayout
                    Row {
                        width: parent.width
                        spacing: 16
                        
                        // 采样间隔
                        Column {
                            width: (parent.width - 16) / 2
                            spacing: 8
                            
                            Text {
                                text: qsTr("采样间隔")
                                color: "#b0ffffff"
                                font.pixelSize: 13
                            }
                            
                            Rectangle {
                                width: parent.width
                                height: 44
                                radius: 8
                                color: "#25ffffff"
                                border.color: intervalInput.activeFocus ? "#00ff88" : "#40ffffff"
                                border.width: 1
                                
                                Row {
                                    anchors.centerIn: parent
                                    spacing: 6
                                    
                                    TextInput {
                                        id: intervalInput
                                        text: robotBridge ? robotBridge.opcuaSampleInterval.toString() : "100"
                                        color: "#ffffff"
                                        font.pixelSize: 14
                                        font.family: "Consolas"
                                        validator: IntValidator { bottom: 10; top: 10000 }
                                        selectByMouse: true
                                        
                                        onTextChanged: {
                                            var val = parseInt(text)
                                            if (!isNaN(val) && robotBridge) {
                                                robotBridge.opcuaSampleInterval = val
                                            }
                                        }
                                    }
                                    
                                    Text {
                                        text: "ms"
                                        color: "#80ffffff"
                                        font.pixelSize: 13
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                            }
                        }
                        
                        // 命名空间
                        Column {
                            width: (parent.width - 16) / 2
                            spacing: 8
                            
                            Text {
                                text: qsTr("命名空间")
                                color: "#b0ffffff"
                                font.pixelSize: 13
                            }
                            
                            Rectangle {
                                width: parent.width
                                height: 44
                                radius: 8
                                color: "#25ffffff"
                                border.color: namespaceInput.activeFocus ? "#00ff88" : "#40ffffff"
                                border.width: 1
                                
                                TextInput {
                                    id: namespaceInput
                                    anchors.centerIn: parent
                                    text: robotBridge ? robotBridge.opcuaNamespace.toString() : "2"
                                    color: "#ffffff"
                                    font.pixelSize: 14
                                    font.family: "Consolas"
                                    validator: IntValidator { bottom: 0; top: 100 }
                                    selectByMouse: true
                                    
                                    onTextChanged: {
                                        var val = parseInt(text)
                                        if (!isNaN(val) && robotBridge) {
                                            robotBridge.opcuaNamespace = val
                                        }
                                    }
                                }
                            }
                        }
                    }
                    
                    // 连接按钮组
                    Row {
                        width: parent.width
                        spacing: 12
                        
                        GlassButton {
                            width: (parent.width - 12) / 2
                            height: 48
                            text: robotBridge && robotBridge.opcuaConnected ? qsTr("断开") : qsTr("连接")
                            iconText: robotBridge && robotBridge.opcuaConnected ? "🔌" : "🔗"
                            highlighted: robotBridge && robotBridge.opcuaConnected
                            accentColor: robotBridge && robotBridge.opcuaConnected ? "#ff6b6b" : "#00ff88"
                            onClicked: {
                                if (robotBridge) {
                                    if (robotBridge.opcuaConnected) {
                                        robotBridge.opcuaDisconnect()
                                    } else {
                                        robotBridge.opcuaConnect()
                                    }
                                }
                            }
                        }
                        
                        GlassButton {
                            width: (parent.width - 12) / 2
                            height: 48
                            text: robotBridge && robotBridge.opcuaSampling ? qsTr("停止采样") : qsTr("开始采样")
                            iconText: robotBridge && robotBridge.opcuaSampling ? "⏹" : "▶"
                            highlighted: robotBridge && robotBridge.opcuaSampling
                            enabled: robotBridge && robotBridge.opcuaConnected
                            opacity: enabled ? 1.0 : 0.5
                            onClicked: {
                                if (robotBridge) {
                                    if (robotBridge.opcuaSampling) {
                                        robotBridge.opcuaStopSampling()
                                    } else {
                                        robotBridge.opcuaStartSampling()
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // ===== 状态指示 =====
            Row {
                width: parent.width
                spacing: 12
                
                Rectangle {
                    width: 14
                    height: 14
                    radius: 7
                    color: {
                        if (!robotBridge) return "#ff4444"
                        if (robotBridge.opcuaSampling) return "#00ff88"
                        if (robotBridge.opcuaConnected) return "#ffaa00"
                        return "#ff4444"
                    }
                    anchors.verticalCenter: parent.verticalCenter
                    
                    SequentialAnimation on opacity {
                        running: robotBridge && robotBridge.opcuaSampling
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.3; duration: 500 }
                        NumberAnimation { to: 1.0; duration: 500 }
                    }
                }
                
                Text {
                    text: {
                        if (!robotBridge) return qsTr("未初始化")
                        if (robotBridge.opcuaSampling) return qsTr("正在采样...")
                        if (robotBridge.opcuaConnected) return qsTr("已连接，等待采样")
                        return qsTr("未连接")
                    }
                    color: "#b0ffffff"
                    font.pixelSize: 14
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            // ===== 分隔线 =====
            Rectangle {
                width: parent.width
                height: 1
                color: "#30ffffff"
            }
            
            // ===== 节点绑定区标题 =====
            RowLayout {
                width: parent.width
                spacing: 10
                
                Text {
                    text: "🔧"
                    font.pixelSize: 18
                }
                
                Text {
                    text: qsTr("节点绑定")
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                }
                
                Item { Layout.fillWidth: true }
                
                GlassButton {
                    width: 36
                    height: 36
                    iconText: "+"
                    tooltipText: qsTr("添加绑定")
                    accentColor: "#00ff88"
                    onClicked: {
                        if (robotBridge) robotBridge.addOpcuaBinding()
                    }
                }
            }
            
            // ===== 绑定列表 =====
            Column {
                width: parent.width
                spacing: 8
                
                Repeater {
                    model: robotBridge ? robotBridge.opcuaBindings : []
                    
                    delegate: BindingItem {
                        width: parent.width
                        jointName: modelData.jointName
                        nodeId: modelData.nodeId
                        enabled: modelData.enabled
                        availableJoints: robotBridge ? robotBridge.jointNames : []
                        
                        onRemoveClicked: {
                            if (robotBridge) robotBridge.removeOpcuaBinding(index)
                        }
                        
                        onBindingChanged: function(joint, node, en) {
                            if (robotBridge) robotBridge.updateOpcuaBinding(index, joint, node, en)
                        }
                    }
                }
            }
            
            // ===== 空状态提示 =====
            Column {
                width: parent.width
                spacing: 12
                visible: !robotBridge || !robotBridge.opcuaBindings || robotBridge.opcuaBindings.length === 0
                
                Item { width: 1; height: 20 }
                
                Text {
                    text: "📋"
                    color: "#40ffffff"
                    font.pixelSize: 40
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: qsTr("暂无绑定配置")
                    color: "#70ffffff"
                    font.pixelSize: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: qsTr("点击 + 添加新绑定")
                    color: "#50ffffff"
                    font.pixelSize: 12
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
            
            // 底部留白
            Item { width: 1; height: 20 }
        }
    }
}
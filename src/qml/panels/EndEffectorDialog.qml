import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import "../components"

// 末端执行器配置面板（弹出对话框）
Popup {
    id: root
    
    property var robotBridge: null
    
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    anchors.centerIn: parent
    width: Math.min(parent.width - 40, 650)
    height: Math.min(parent.height - 80, 600)
    
    background: Item {
        // 毛玻璃背景
        Rectangle {
            anchors.fill: parent
            radius: 16
            color: "#e0101020"
            border.color: "#40ffffff"
            border.width: 1
            
            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                horizontalOffset: 0
                verticalOffset: 8
                radius: 32
                samples: 33
                color: "#80000000"
            }
        }
    }
    
    contentItem: ColumnLayout {
        spacing: 16
        
        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            
            Text {
                text: "🎯"
                font.pixelSize: 24
            }
            
            Text {
                text: qsTr("末端执行器配置")
                color: "#ffffff"
                font.pixelSize: 18
                font.weight: Font.Bold
            }
            
            Item { Layout.fillWidth: true }
            
            GlassButton {
                width: 32
                height: 32
                iconText: "✕"
                onClicked: root.close()
            }
        }
        
        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.3; color: "#40ffffff" }
                GradientStop { position: 0.7; color: "#40ffffff" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
        
        // 说明文字
        Text {
            Layout.fillWidth: true
            text: qsTr("配置需要显示轨迹的末端执行器（如工具末端、手指尖端等）。每个末端执行器会显示其运动轨迹。")
            color: "#80ffffff"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
        }
        
        // 配置列表
        ScrollView {
            id: scrollView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            

            ColumnLayout {
                width: scrollView.availableWidth  // 使用 ScrollView 的可用宽度
                spacing: 10

                // 末端执行器列表
                Repeater {
                    id: configRepeater
                    model: robotBridge ? robotBridge.endEffectorConfigs : []

                    delegate: EndEffectorItem {
                        Layout.fillWidth: true
                        // Layout.preferredWidth: root.width
                        linkName: modelData.linkName || ""
                        displayName: modelData.displayName || ""
                        colorHex: modelData.colorHex || "#FFFF00"
                        enabled: modelData.enabled !== undefined ? modelData.enabled : true
                        availableLinks: robotBridge ? robotBridge.linkNames : []
                        
                        onRemoveClicked: {
                            if (robotBridge) {
                                robotBridge.removeEndEffectorConfig(index)
                            }
                        }
                        
                        onConfigChanged: function(link, name, color, en) {
                            if (robotBridge) {
                                robotBridge.updateEndEffectorConfig(index, link, name, color, en)
                            }
                        }
                    }
                }
                
                // 空状态提示
                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 80
                    visible: configRepeater.count === 0
                    
                    Column {
                        anchors.centerIn: parent
                        spacing: 8
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: "📭"
                            font.pixelSize: 32
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("暂无末端执行器配置")
                            color: "#60ffffff"
                            font.pixelSize: 13
                        }
                        
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("点击下方按钮添加")
                            color: "#40ffffff"
                            font.pixelSize: 11
                        }
                    }
                }
                
                // 添加按钮
                GlassButton {
                    Layout.fillWidth: true
                    height: 44
                    text: qsTr("添加末端执行器")
                    iconText: "➕"
                    
                    onClicked: {
                        if (robotBridge && robotBridge.linkNames.length > 0) {
                            // 默认选择最后一个link（通常是末端）
                            var links = robotBridge.linkNames
                            var defaultLink = links[links.length - 1]
                            robotBridge.addEndEffectorConfig(defaultLink, "", "")
                        } else {
                            // 没有可用链接
                        }
                    }
                }
            }
        }
        
        // 底部按钮区
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            
            GlassButton {
                Layout.fillWidth: true
                height: 44
                text: qsTr("应用配置")
                iconText: "✓"
                highlighted: true
                accentColor: "#00ff88"
                
                onClicked: {
                    if (robotBridge) {
                        robotBridge.applyEndEffectorConfigs()
                    }
                }
            }
            
            GlassButton {
                Layout.fillWidth: true
                height: 44
                text: qsTr("关闭")
                iconText: "✕"
                
                onClicked: root.close()
            }
        }
    }
    
    // 打开时刷新数据
    onOpened: {
        // 可以在这里进行数据刷新
    }
}

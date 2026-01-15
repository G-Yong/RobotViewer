import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import "."

// 底部状态栏 - 包含状态信息和操作提示
Item {
    id: root
    
    property string statusText: ""
    property bool connectionStatus: false
    property int fps: 60
    
    GlassPanel {
        anchors.fill: parent
        glassOpacity: 0.8
        cornerRadius: 8
    }
    
    Row {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        spacing: 15
        
        // 状态文本（左侧）
        Row {
            id: statusArea
            spacing: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(implicitWidth, parent.width * 0.35)
            
            // 加载指示器
            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: "#00ff88"
                visible: statusText !== ""
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: statusText
                color: "#a0ffffff"
                font.pixelSize: FontConfig.small
                elide: Text.ElideMiddle
                width: parent.width - 18
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // 分隔线
        Rectangle {
            width: 1
            height: parent.height - 12
            color: "#25ffffff"
            anchors.verticalCenter: parent.verticalCenter
            visible: statusText !== ""
        }
        
        // 操作提示（中间）
        Row {
            id: controlHints
            anchors.verticalCenter: parent.verticalCenter
            spacing: 20
            
            // 左键旋转
            Row {
                spacing: 5
                anchors.verticalCenter: parent.verticalCenter
                
                Rectangle {
                    width: 18
                    height: 18
                    radius: 3
                    color: "#20ffffff"
                    border.color: "#40ffffff"
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Text {
                        anchors.centerIn: parent
                        text: "🖱"
                        font.pixelSize: FontConfig.tiny
                    }
                }
                
                Text {
                    text: "左键旋转"
                    color: "#80ffffff"
                    font.pixelSize: FontConfig.small
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            // 滚轮/右键缩放
            Row {
                spacing: 5
                anchors.verticalCenter: parent.verticalCenter
                
                Rectangle {
                    width: 18
                    height: 18
                    radius: 3
                    color: "#20ffffff"
                    border.color: "#40ffffff"
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Text {
                        anchors.centerIn: parent
                        text: "⚲"
                        font.pixelSize: FontConfig.tiny
                    }
                }
                
                Text {
                    text: "滚轮/右键缩放"
                    color: "#80ffffff"
                    font.pixelSize: FontConfig.small
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            // Shift平移
            Row {
                spacing: 5
                anchors.verticalCenter: parent.verticalCenter
                
                Rectangle {
                    width: 32
                    height: 18
                    radius: 3
                    color: "#20ffffff"
                    border.color: "#40ffffff"
                    border.width: 1
                    anchors.verticalCenter: parent.verticalCenter
                    
                    Text {
                        anchors.centerIn: parent
                        text: "Shift"
                        font.pixelSize: FontConfig.tiny
                        font.family: "Consolas"
                        color: "#c0ffffff"
                    }
                }
                
                Text {
                    text: "+拖动/中键按住 平移"
                    color: "#80ffffff"
                    font.pixelSize: FontConfig.small
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
        
        // 弹性空间
        Item {
            width: Math.max(1, parent.width - statusArea.width - controlHints.width - rightArea.width - 80)
            height: 1
        }
        
        // 右侧区域：连接状态和FPS
        Row {
            id: rightArea
            spacing: 15
            anchors.verticalCenter: parent.verticalCenter
            
            // 连接状态指示器
            Row {
                id: connectionIndicator
                spacing: 6
                anchors.verticalCenter: parent.verticalCenter
                
                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: connectionStatus ? "#00ff88" : "#ff4444"
                    anchors.verticalCenter: parent.verticalCenter
                    
                    SequentialAnimation on opacity {
                        running: connectionStatus
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.5; duration: 1000 }
                        NumberAnimation { to: 1.0; duration: 1000 }
                    }
                }
                
                Text {
                    text: connectionStatus ? "OPC UA 已连接" : "OPC UA 未连接"
                    color: connectionStatus ? "#00ff88" : "#60ffffff"
                    font.pixelSize: FontConfig.small
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            
            // 分隔线
            Rectangle {
                width: 1
                height: parent.height - 6
                color: "#25ffffff"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            // FPS显示
            Row {
                id: fpsDisplay
                spacing: 4
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    text: fps
                    color: fps >= 50 ? "#00ff88" : (fps >= 30 ? "#ffaa00" : "#ff4444")
                    font.pixelSize: FontConfig.normal
                    font.family: "Consolas"
                    font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                }
                
                Text {
                    text: "FPS"
                    color: "#60ffffff"
                    font.pixelSize: FontConfig.tiny
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}

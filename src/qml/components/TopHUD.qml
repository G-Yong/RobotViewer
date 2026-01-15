import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import "."

// 顶部HUD栏
Item {
    id: root
    
    property string robotName: ""
    property vector3d endEffectorPosition: Qt.vector3d(0, 0, 0)
    
    signal settingsClicked()
    signal openFileClicked()
    signal resetCameraClicked()
    
    // 背景面板
    GlassPanel {
        anchors.fill: parent
        glassOpacity: 0.85
    }
    
    Row {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15
        
        // Logo和应用名
        Row {
            spacing: 10
            anchors.verticalCenter: parent.verticalCenter
            
            // Logo图标
            Rectangle {
                width: 40
                height: 40
                radius: 8
                color: "#00ff88"
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    anchors.centerIn: parent
                    text: "🤖"
                    font.pixelSize: FontConfig.xlarge
                }
                
                // 脉冲动画
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.8; duration: 1500 }
                    NumberAnimation { to: 1.0; duration: 1500 }
                }
            }
            
            Column {
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    text: "ROBOT VIEWER"
                    color: "#ffffff"
                    font.pixelSize: FontConfig.medium
                    font.weight: Font.Bold
                    font.letterSpacing: 2
                }
                
                Text {
                    text: robotName || "未加载模型"
                    color: robotName ? "#00ff88" : "#60ffffff"
                    font.pixelSize: FontConfig.small
                    font.weight: Font.Medium
                }
            }
        }
        
        // 分隔线
        Rectangle {
            width: 1
            height: parent.height - 10
            color: "#30ffffff"
            anchors.verticalCenter: parent.verticalCenter
        }
        
        // 工具按钮组
        Row {
            spacing: 8
            anchors.verticalCenter: parent.verticalCenter
            
            GlassButton {
                iconText: "📁"
                tooltipText: qsTr("打开URDF文件 (Ctrl+O)")
                onClicked: openFileClicked()
            }
            
            GlassButton {
                iconText: "🎯"
                tooltipText: qsTr("重置相机视角 (R)")
                onClicked: resetCameraClicked()
            }
        }
        
        // 弹性空间
        Item { 
            width: parent.width - x - endEffectorInfo.width - settingsBtn.width - 40
            height: 1 
        }
        
        // 末端执行器位置显示
        Row {
            id: endEffectorInfo
            spacing: 15
            anchors.verticalCenter: parent.verticalCenter
            visible: robotName !== ""
            
            // 分隔线
            Rectangle {
                width: 1
                height: parent.parent.height - 10
                color: "#30ffffff"
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Column {
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    text: "END EFFECTOR"
                    color: "#60ffffff"
                    font.pixelSize: FontConfig.tiny
                    font.weight: Font.Medium
                    font.letterSpacing: 1
                }
                
                Row {
                    spacing: 15
                    
                    CoordinateDisplay {
                        label: "X"
                        value: endEffectorPosition.x
                        textColor: "#ff6b6b"
                    }
                    
                    CoordinateDisplay {
                        label: "Y"
                        value: endEffectorPosition.y
                        textColor: "#4ecdc4"
                    }
                    
                    CoordinateDisplay {
                        label: "Z"
                        value: endEffectorPosition.z
                        textColor: "#45b7d1"
                    }
                }
            }
        }
        
        // 设置按钮
        GlassButton {
            id: settingsBtn
            iconText: "⚙"
            tooltipText: qsTr("打开设置面板 (Tab)")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: settingsClicked()
        }
    }
}

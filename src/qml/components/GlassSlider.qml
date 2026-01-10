import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

// 游戏风格滑块
Item {
    id: root
    
    property real value: 0
    property real from: 0
    property real to: 100
    property string suffix: ""
    property int decimals: 1
    property color accentColor: "#00ff88"
    property string label: ""
    
    signal valueModified(real newValue)
    
    implicitWidth: 200
    implicitHeight: label ? 50 : 30
    
    Column {
        anchors.fill: parent
        spacing: 6
        
        // 标签行
        Item {
            visible: label !== ""
            width: parent.width
            height: 18
            
            Text {
                text: label
                color: "#a0ffffff"
                font.pixelSize: 13
                font.weight: Font.Medium
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: root.value.toFixed(decimals) + suffix
                color: accentColor
                font.pixelSize: 13
                font.weight: Font.Bold
                font.family: "Consolas"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        
        // 滑块
        Item {
            width: parent.width
            height: 24
            
            // 轨道背景
            Rectangle {
                id: trackBg
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 6
                radius: 3
                color: "#20ffffff"
                
                // 进度条
                Rectangle {
                    width: parent.width * ((root.value - root.from) / (root.to - root.from))
                    height: parent.height
                    radius: parent.radius
                    
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Qt.darker(accentColor, 1.5) }
                        GradientStop { position: 1.0; color: accentColor }
                    }
                    
                    Behavior on width {
                        NumberAnimation { duration: 50 }
                    }
                }
            }
            
            // 滑块手柄
            Rectangle {
                id: handle
                width: 18
                height: 18
                radius: 9
                x: (parent.width - width) * ((root.value - root.from) / (root.to - root.from))
                anchors.verticalCenter: parent.verticalCenter
                
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#ffffff" }
                    GradientStop { position: 1.0; color: "#cccccc" }
                }
                
                border.color: mouseArea.containsMouse ? accentColor : "#60ffffff"
                border.width: 2
                
                Behavior on border.color {
                    ColorAnimation { duration: 150 }
                }
                
                // 发光效果
                layer.enabled: mouseArea.pressed || mouseArea.containsMouse
                layer.effect: Glow {
                    radius: 10
                    samples: 17
                    color: accentColor
                    spread: 0.3
                }
            }
            
            MouseArea {
                id: mouseArea
                anchors.fill: parent
                anchors.margins: -5  // 扩大点击区域
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                preventStealing: true  // 阻止父组件窃取鼠标事件
                
                onPressed: {
                    mouse.accepted = true
                    updateValue(mouse.x + 5)
                }
                onPositionChanged: {
                    if (pressed) updateValue(mouse.x + 5)
                }
                
                function updateValue(mouseX) {
                    var ratio = Math.max(0, Math.min(1, mouseX / parent.width))
                    var newValue = root.from + ratio * (root.to - root.from)
                    if (Math.abs(newValue - root.value) > 0.001) {
                        root.value = newValue
                        root.valueModified(newValue)
                    }
                }
            }
        }
    }
}

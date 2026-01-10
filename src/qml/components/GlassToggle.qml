import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

// 游戏风格开关按钮
Item {
    id: root
    
    property bool checked: false
    property string text: ""
    property color accentColor: "#00ff88"
    
    signal toggled(bool checked)
    
    implicitWidth: row.width
    implicitHeight: 32
    
    Row {
        id: row
        spacing: 10
        anchors.verticalCenter: parent.verticalCenter
        
        // 开关轨道
        Rectangle {
            id: track
            width: 44
            height: 22
            radius: 11
            color: checked ? Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.3) : "#20ffffff"
            border.color: checked ? accentColor : "#40ffffff"
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            
            Behavior on color {
                ColorAnimation { duration: 200 }
            }
            
            Behavior on border.color {
                ColorAnimation { duration: 200 }
            }
            
            // 滑块
            Rectangle {
                id: thumb
                width: 16
                height: 16
                radius: 8
                x: checked ? parent.width - width - 3 : 3
                anchors.verticalCenter: parent.verticalCenter
                
                gradient: Gradient {
                    GradientStop { position: 0.0; color: checked ? accentColor : "#ffffff" }
                    GradientStop { position: 1.0; color: checked ? Qt.darker(accentColor, 1.2) : "#cccccc" }
                }
                
                Behavior on x {
                    NumberAnimation { 
                        duration: 200 
                        easing.type: Easing.OutCubic 
                    }
                }
                
                // 发光效果
                layer.enabled: checked
                layer.effect: Glow {
                    radius: 8
                    samples: 17
                    color: accentColor
                    spread: 0.2
                }
            }
        }
        
        // 标签
        Text {
            text: root.text
            color: checked ? "#ffffff" : "#80ffffff"
            font.pixelSize: 13
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
            
            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }
    }
    
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            checked = !checked
            root.toggled(checked)
        }
    }
}

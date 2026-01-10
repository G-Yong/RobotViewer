import QtQuick 2.15
import QtQuick.Controls 2.15

// 游戏风格Tab按钮
TabButton {
    id: root
    
    property string iconText: ""
    
    implicitHeight: 40
    
    background: Rectangle {
        color: root.checked ? "#20ffffff" : (root.hovered ? "#10ffffff" : "transparent")
        radius: 8
        border.color: root.checked ? "#00ff88" : "transparent"
        border.width: root.checked ? 1 : 0
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
        
        // 底部高亮条
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.checked ? parent.width - 20 : 0
            height: 2
            radius: 1
            color: "#00ff88"
            
            Behavior on width {
                NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
            }
        }
    }
    
    contentItem: Row {
        spacing: 6
        anchors.centerIn: parent
        
        Text {
            text: root.iconText
            font.pixelSize: 14
            anchors.verticalCenter: parent.verticalCenter
        }
        
        Text {
            text: root.text
            color: root.checked ? "#ffffff" : "#80ffffff"
            font.pixelSize: 12
            font.weight: root.checked ? Font.Bold : Font.Medium
            anchors.verticalCenter: parent.verticalCenter
            
            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }
    }
}

import QtQuick 2.15
import "."

// 坐标值显示组件
Row {
    id: root
    
    property string label: ""
    property real value: 0
    property color textColor: "#ffffff"
    
    spacing: 4
    
    Text {
        text: root.label + ":"
        color: root.textColor
        font.pixelSize: FontConfig.small
        font.weight: Font.Bold
        anchors.verticalCenter: parent.verticalCenter
    }
    
    Text {
        text: root.value.toFixed(3)
        color: root.textColor
        font.pixelSize: FontConfig.small
        font.family: "Consolas"
        font.weight: Font.Medium
        anchors.verticalCenter: parent.verticalCenter
    }
}

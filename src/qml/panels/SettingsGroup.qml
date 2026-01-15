import QtQuick 2.15
import QtQuick.Layouts 1.15
import "../components"

// 设置组组件
Item {
    id: root
    
    property string title: ""
    property string iconText: ""
    default property alias content: contentColumn.children
    
    implicitHeight: header.height + contentColumn.height + 24
    
    // 组标题
    Row {
        id: header
        spacing: 10
        
        Text {
            text: root.iconText
            font.pixelSize: FontConfig.medium
            anchors.verticalCenter: parent.verticalCenter
        }
        
        Text {
            text: root.title
            color: "#ffffff"
            font.pixelSize: FontConfig.normal
            font.weight: Font.Bold
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    
    // 内容区
    ColumnLayout {
        id: contentColumn
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 14
        anchors.leftMargin: 10
    }
    
    // 底部分隔线
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#25ffffff"
    }
}

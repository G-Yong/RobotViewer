import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

// 游戏风格按钮
Rectangle {
    id: root
    
    property string iconSource: ""
    property string iconText: ""
    property string text: ""
    property string tooltipText: ""
    property bool highlighted: false
    property color accentColor: "#00ff88"
    property real iconSize: 20
    
    signal clicked()
    
    width: text ? textMetrics.width + 40 : 44
    height: 44
    radius: 8
    color: mouseArea.containsPress ? "#30ffffff" : 
           (mouseArea.containsMouse || highlighted) ? "#20ffffff" : "#10ffffff"
    border.color: highlighted ? accentColor : (mouseArea.containsMouse ? "#40ffffff" : "#20ffffff")
    border.width: 1
    
    Behavior on color {
        ColorAnimation { duration: 150 }
    }
    
    Behavior on border.color {
        ColorAnimation { duration: 150 }
    }
    
    TextMetrics {
        id: textMetrics
        font: textLabel.font
        text: root.text
    }
    
    Row {
        anchors.centerIn: parent
        spacing: 8
        
        // 图标（可以是SVG或文字图标）
        Text {
            visible: iconText !== "" && iconSource === ""
            text: iconText
            color: highlighted ? accentColor : "#ffffff"
            font.pixelSize: root.iconSize
            font.family: "Segoe UI Symbol"
            anchors.verticalCenter: parent.verticalCenter
        }
        
        Image {
            visible: iconSource !== ""
            source: iconSource
            width: root.iconSize
            height: root.iconSize
            sourceSize: Qt.size(width * 2, height * 2)
            anchors.verticalCenter: parent.verticalCenter
            
            ColorOverlay {
                anchors.fill: parent
                source: parent
                color: highlighted ? accentColor : "#ffffff"
            }
        }
        
        Text {
            id: textLabel
            visible: text !== ""
            text: root.text
            color: highlighted ? accentColor : "#ffffff"
            font.pixelSize: 14
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    
    // 悬停时的发光效果
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: 2
        border.color: accentColor
        opacity: mouseArea.containsMouse ? 0.3 : 0
        
        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }
    }
    
    ToolTip {
        visible: tooltipText !== "" && mouseArea.containsMouse
        text: tooltipText
        delay: 500
        
        background: Rectangle {
            color: "#cc1a1a2e"
            border.color: "#40ffffff"
            border.width: 1
            radius: 4
        }
        
        contentItem: Text {
            text: root.tooltipText
            color: "#ffffff"
            font.pixelSize: 12
        }
    }
}

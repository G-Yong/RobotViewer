import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

// 消息弹出提示
Item {
    id: root
    
    width: 400
    height: messageRow.height + 24
    visible: false
    opacity: 0
    
    property bool isError: false
    
    function show(message, error) {
        isError = error || false
        messageText.text = message
        visible = true
        hideTimer.restart()
    }
    
    function hide() {
        visible = false
    }
    
    Timer {
        id: hideTimer
        interval: 4000
        onTriggered: root.hide()
    }
    
    // 背景
    GlassPanel {
        anchors.fill: parent
        glassColor: isError ? "#2e1a1a" : "#1a2e1a"
        borderColor: isError ? "#80ff4444" : "#8000ff88"
    }
    
    Row {
        id: messageRow
        anchors.centerIn: parent
        anchors.margins: 12
        spacing: 12
        
        // 图标
        Text {
            text: isError ? "⚠" : "✓"
            color: isError ? "#ff4444" : "#00ff88"
            font.pixelSize: 20
            anchors.verticalCenter: parent.verticalCenter
        }
        
        // 消息文本
        Text {
            id: messageText
            text: ""
            color: "#ffffff"
            font.pixelSize: 14
            font.weight: Font.Medium
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            width: Math.min(implicitWidth, 340)
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    
    // 关闭按钮
    Text {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 8
        text: "✕"
        color: "#60ffffff"
        font.pixelSize: 14
        
        MouseArea {
            anchors.fill: parent
            anchors.margins: -5
            cursorShape: Qt.PointingHandCursor
            onClicked: root.hide()
        }
    }
    
    // 动画
    states: [
        State {
            name: "visible"
            when: root.visible
            PropertyChanges { target: root; opacity: 1; y: root.parent.height - root.height - 80 }
        },
        State {
            name: "hidden"
            when: !root.visible
            PropertyChanges { target: root; opacity: 0; y: root.parent.height }
        }
    ]
    
    transitions: [
        Transition {
            from: "hidden"; to: "visible"
            NumberAnimation { properties: "opacity,y"; duration: 300; easing.type: Easing.OutCubic }
        },
        Transition {
            from: "visible"; to: "hidden"
            NumberAnimation { properties: "opacity,y"; duration: 200; easing.type: Easing.InCubic }
        }
    ]
}

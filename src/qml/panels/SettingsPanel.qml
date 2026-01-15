import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import "../components"

// 右侧滑出式设置面板
Item {
    id: root
    
    property var robotBridge: null
    
    function focusJoint(jointName) {
        tabBar.currentIndex = 1
        jointControlPanel.focusJoint(jointName)
    }
    
    // 主背景
    GlassPanel {
        anchors.fill: parent
        glassOpacity: 0.92
        glassColor: "#0d1117"
        borderColor: "#50ffffff"
        cornerRadius: 16
    }
    
    // 关闭按钮手柄
    Rectangle {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 24
        height: 80
        radius: 8
        color: "#30ffffff"
        x: -12
        
        Text {
            anchors.centerIn: parent
            text: "❯"
            color: "#80ffffff"
            font.pixelSize: FontConfig.medium
            rotation: 0
        }
        
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: mainWindow.settingsPanelOpen = false
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12
        
        // 标题栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Text {
                text: "⚙"
                color: "#00ff88"
                font.pixelSize: FontConfig.large
            }
            
            Text {
                text: "SETTINGS"
                color: "#ffffff"
                font.pixelSize: FontConfig.medium
                font.weight: Font.Bold
                font.letterSpacing: 2
            }
            
            Item { Layout.fillWidth: true }
            
            // 关闭按钮
            GlassButton {
                width: 32
                height: 32
                iconText: "✕"
                onClicked: mainWindow.settingsPanelOpen = false
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
        
        // Tab切换栏
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            background: Rectangle { color: "transparent" }
            
            GameTabButton {
                text: qsTr("视图")
                iconText: "👁"
            }
            
            GameTabButton {
                text: qsTr("关节")
                iconText: "🔧"
            }
            
            GameTabButton {
                text: qsTr("通讯")
                iconText: "📡"
            }
        }
        
        // Tab内容区
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex
            
            // 视图选项页
            ViewOptionsPanel {
                id: viewOptionsPanel
                robotBridge: root.robotBridge
            }
            
            // 关节控制页
            JointControlPanel {
                id: jointControlPanel
                robotBridge: root.robotBridge
            }
            
            // OPC UA通讯页
            OpcuaPanel {
                id: opcuaPanel
                robotBridge: root.robotBridge
            }
        }
    }
}

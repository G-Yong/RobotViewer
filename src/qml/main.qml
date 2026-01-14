import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import "components"
import "panels"

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1400
    height: 900
    minimumWidth: 1024
    minimumHeight: 768
    title: qsTr("RobotViewer") + " V" + (robotBridge ? robotBridge.version : "0.0.5")
    color: "transparent"
    
    // 全局字体 - 使用微软雅黑，小字号时更清晰
    font.family: "Microsoft YaHei UI"
    
    // 面板状态
    property bool settingsPanelOpen: false
    property bool jointsPanelOpen: true
    
    // 本地引用 C++ 的 robotBridge（避免子组件中的 binding loop）
    readonly property var bridge: robotBridge
    
    // 背景 (3D场景将在这层下方由C++渲染)
    Rectangle {
        id: background
        anchors.fill: parent
        color: "#0a0a0f"
        z: -100
    }
    
    // 3D场景装饰效果
    Scene3DView {
        id: scene3d
        anchors.fill: parent
        z: -50
        robotBridge: mainWindow.bridge
    }
    
    // 顶部HUD条
    TopHUD {
        id: topHud
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        height: 60
        z: 100
        
        robotName: robotBridge ? robotBridge.robotName : ""
        endEffectorPosition: robotBridge ? robotBridge.endEffectorPosition : Qt.vector3d(0,0,0)
        
        onSettingsClicked: settingsPanelOpen = !settingsPanelOpen
        onOpenFileClicked: { if (robotBridge) robotBridge.openURDF() }
        onResetCameraClicked: { if (robotBridge) robotBridge.resetCamera() }
    }
    
    // 关节信息叠加层（显示在3D场景上）
    JointOverlay {
        id: jointOverlay
        anchors.left: parent.left
        anchors.bottom: bottomBar.top
        anchors.margins: 20
        anchors.bottomMargin: 10
        z: 99
        
        robotBridge: mainWindow.bridge  // 使用本地引用
        visible: jointsPanelOpen && (robotBridge ? robotBridge.robotLoaded : false)
        
        onJointClicked: function(jointName) {
            settingsPanelOpen = true
            settingsPanel.focusJoint(jointName)
        }
    }
    
    // 底部状态栏
    BottomStatusBar {
        id: bottomBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 20
        height: 40
        z: 100
        
        statusText: robotBridge ? robotBridge.statusMessage : ""
        connectionStatus: robotBridge ? robotBridge.opcuaConnected : false
        fps: scene3d.fps
    }
    
    // 右侧滑出式设置面板
    SettingsPanel {
        id: settingsPanel
        width: 380
        anchors.top: topHud.bottom
        anchors.bottom: bottomBar.top
        anchors.right: parent.right
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        anchors.rightMargin: settingsPanelOpen ? 20 : -width - 20
        z: 100
        
        robotBridge: mainWindow.bridge
        
        Behavior on anchors.rightMargin {
            NumberAnimation { 
                duration: 350 
                easing.type: Easing.OutCubic 
            }
        }
    }
    
    // 关节面板切换按钮
    GlassButton {
        anchors.left: parent.left
        anchors.bottom: jointOverlay.visible ? jointOverlay.top : bottomBar.top
        anchors.margins: 20
        anchors.bottomMargin: 10
        z: 99  // 比状态栏低一层
        
        width: 44
        height: 44
        iconText: jointsPanelOpen ? "◉" : "○"
        tooltipText: jointsPanelOpen ? qsTr("隐藏关节信息") : qsTr("显示关节信息")
        
        onClicked: jointsPanelOpen = !jointsPanelOpen
    }
    
    // 快捷键
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: { if (robotBridge) robotBridge.openURDF() }
    }
    
    Shortcut {
        sequence: "R"
        onActivated: { if (robotBridge) robotBridge.resetCamera() }
    }
    
    Shortcut {
        sequence: "Tab"
        onActivated: settingsPanelOpen = !settingsPanelOpen
    }
    
    Shortcut {
        sequence: "J"
        onActivated: jointsPanelOpen = !jointsPanelOpen
    }
    
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (settingsPanelOpen) settingsPanelOpen = false
        }
    }
    
    // 拖放支持
    DropArea {
        anchors.fill: parent
        keys: ["text/uri-list"]
        z: 50
        
        onDropped: {
            if (drop.hasUrls && robotBridge) {
                var url = drop.urls[0].toString()
                if (url.toLowerCase().endsWith(".urdf")) {
                    robotBridge.loadRobot(url.replace("file:///", ""))
                }
            }
        }
        
        Rectangle {
            anchors.fill: parent
            color: "#4000ff88"
            visible: parent.containsDrag
            
            GlassPanel {
                anchors.centerIn: parent
                width: 300
                height: 100
                
                Text {
                    anchors.centerIn: parent
                    text: qsTr("释放以加载URDF文件")
                    color: "#00ff88"
                    font.pixelSize: 18
                    font.weight: Font.Medium
                }
            }
        }
    }
    
    // 加载动画
    LoadingOverlay {
        id: loadingOverlay
        anchors.fill: parent
        z: 200
        visible: robotBridge ? robotBridge.isLoading : false
        message: qsTr("正在加载模型...")
    }
    
    // 消息提示
    MessagePopup {
        id: messagePopup
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 80
        z: 200
    }
    
    Connections {
        target: robotBridge
        function onShowMessage(msg, isError) {
            messagePopup.show(msg, isError)
        }
    }
}

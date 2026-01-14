import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import "../components"

// 关节控制面板
Item {
    id: root
    
    property var robotBridge: null
    
    // 使用 ListModel 来避免整体刷新
    ListModel {
        id: jointModel
    }
    
    function focusJoint(jointName) {
        for (var i = 0; i < jointModel.count; i++) {
            if (jointModel.get(i).name === jointName) {
                jointListView.positionViewAtIndex(i, ListView.Center)
                break
            }
        }
    }
    
    // 从 robotBridge 初始化/刷新整个列表（仅在加载模型时）
    function refreshJointList() {
        jointModel.clear()
        if (!robotBridge) return
        var joints = robotBridge.jointInfoList
        for (var i = 0; i < joints.length; i++) {
            jointModel.append({
                "name": joints[i].name,
                "value": joints[i].value,
                "min": joints[i].min,
                "max": joints[i].max,
                "type": joints[i].type
            })
        }
    }
    
    // 更新单个关节值（不刷新整个列表）
    function updateJointValue(jointName, value) {
        for (var i = 0; i < jointModel.count; i++) {
            if (jointModel.get(i).name === jointName) {
                jointModel.setProperty(i, "value", value)
                break
            }
        }
    }
    
    // 监听 robotBridge 信号
    Connections {
        target: robotBridge ? robotBridge : null
        
        function onJointInfoListChanged() {
            refreshJointList()
        }
        
        function onJointValueUpdated(jointName, value) {
            updateJointValue(jointName, value)
        }
    }
    
    // 当 robotBridge 变化时刷新列表
    onRobotBridgeChanged: {
        if (robotBridge) refreshJointList()
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 12
        
        // 顶部工具栏
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            Text {
                text: qsTr("关节控制")
                color: "#ffffff"
                font.pixelSize: 13
                font.weight: Font.Bold
            }
            
            // 关节数量
            Rectangle {
                width: countText.width + 16
                height: 20
                radius: 10
                color: "#20ffffff"
                
                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: jointModel.count + qsTr(" 个关节")
                    color: "#80ffffff"
                    font.pixelSize: 10
                }
            }
            
            Item { Layout.fillWidth: true }
            
            // 全部重置按钮
            GlassButton {
                text: qsTr("重置")
                height: 28
                onClicked: {
                    if (robotBridge) robotBridge.resetAllJoints()
                }
            }
        }
        
        // 分隔线
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#15ffffff"
        }
        
        // 关节列表
        ListView {
            id: jointListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            model: jointModel
            spacing: 8
            clip: true
            cacheBuffer: 400  // 缓存更多项以提高滚动性能
            
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }
            
            delegate: JointControlItem {
                width: jointListView.width - 10
                jointIndex: index
                jointName: model.name
                jointValue: model.value
                jointMin: model.min
                jointMax: model.max
                jointType: model.type
                
                onValueChanged: function(name, val) {
                    if (robotBridge) {
                        robotBridge.setJointValue(name, val)
                    }
                }
            }
            
            // 空状态提示
            Item {
                anchors.centerIn: parent
                visible: jointModel.count === 0
                
                Column {
                    anchors.centerIn: parent
                    spacing: 10
                    
                    Text {
                        text: "🔧"
                        color: "#40ffffff"
                        font.pixelSize: 40
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: qsTr("暂无可控关节")
                        color: "#60ffffff"
                        font.pixelSize: 13
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    
                    Text {
                        text: qsTr("请先加载机器人模型")
                        color: "#40ffffff"
                        font.pixelSize: 11
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }
            }
        }
    }
}

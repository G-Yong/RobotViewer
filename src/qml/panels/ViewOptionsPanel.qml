import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import "../components"

// 视图选项面板
ScrollView {
    id: root
    
    property var robotBridge: null
    
    clip: true
    ScrollBar.vertical.policy: ScrollBar.AsNeeded
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    
    ColumnLayout {
        width: root.width - 10
        spacing: 16
        
        // 显示选项组
        SettingsGroup {
            title: qsTr("显示选项")
            iconText: "👁"
            Layout.fillWidth: true
            
            ColumnLayout {
                spacing: 12
                
                GlassToggle {
                    text: qsTr("显示网格")
                    checked: robotBridge ? robotBridge.showGrid : true
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.showGrid = checked
                    }
                }
                
                GlassToggle {
                    text: qsTr("显示坐标轴")
                    checked: robotBridge ? robotBridge.showAxes : true
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.showAxes = checked
                    }
                }
                
                GlassToggle {
                    text: qsTr("显示关节坐标轴")
                    checked: robotBridge ? robotBridge.showJointAxes : false
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.showJointAxes = checked
                    }
                }
                
                GlassToggle {
                    text: qsTr("零件着色")
                    checked: robotBridge ? robotBridge.coloredLinks : false
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.coloredLinks = checked
                    }
                }
            }
        }
        
        // 坐标系选项
        SettingsGroup {
            title: qsTr("坐标系")
            iconText: "📐"
            Layout.fillWidth: true
            
            ColumnLayout {
                spacing: 12
                
                GlassToggle {
                    text: qsTr("Z轴朝上")
                    checked: robotBridge ? robotBridge.zUpEnabled : false
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.zUpEnabled = checked
                    }
                }
                
                GlassToggle {
                    text: qsTr("自动缩放模型")
                    checked: robotBridge ? robotBridge.autoScaleEnabled : true
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.autoScaleEnabled = checked
                    }
                }
            }
        }
        
        // 轨迹选项
        SettingsGroup {
            title: qsTr("轨迹设置")
            iconText: "〰"
            Layout.fillWidth: true
            
            ColumnLayout {
                spacing: 12
                
                GlassToggle {
                    text: qsTr("显示轨迹")
                    checked: robotBridge ? robotBridge.showTrajectory : true
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.showTrajectory = checked
                    }
                }
                
                GlassSlider {
                    Layout.fillWidth: true
                    label: qsTr("轨迹生命周期")
                    from: 0.5
                    to: 10.0
                    value: robotBridge ? robotBridge.trajectoryLifetime : 2.0
                    suffix: " s"
                    decimals: 1
                    onValueModified: function(newValue) {
                        if (robotBridge) robotBridge.trajectoryLifetime = newValue
                    }
                }
                
                GlassButton {
                    Layout.fillWidth: true
                    text: qsTr("配置末端执行器...")
                    height: 36
                    onClicked: {
                        endEffectorDialog.open()
                    }
                }
            }
        }
        
        // 相机选项
        SettingsGroup {
            title: qsTr("相机控制")
            iconText: "🎥"
            Layout.fillWidth: true
            
            ColumnLayout {
                spacing: 12
                
                GlassButton {
                    Layout.fillWidth: true
                    text: qsTr("重置相机")
                    iconText: "🎯"
                    height: 36
                    onClicked: {
                        if (robotBridge) robotBridge.resetCamera()
                    }
                }
                
                GlassButton {
                    Layout.fillWidth: true
                    text: qsTr("适配视角")
                    iconText: "🔍"
                    height: 36
                    onClicked: {
                        if (robotBridge) robotBridge.fitCamera()
                    }
                }
            }
        }
        
        Item { Layout.fillHeight: true }
    }
    
    // 末端执行器配置对话框
    EndEffectorDialog {
        id: endEffectorDialog
        robotBridge: root.robotBridge
        parent: Overlay.overlay
    }
}

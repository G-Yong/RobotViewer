import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

// 末端执行器配置项组件
Item {
    id: root
    
    property string linkName: ""
    property string displayName: ""
    property string colorHex: "#FFFF00"
    property bool enabled: true
    property var availableLinks: []
    
    signal removeClicked()
    signal configChanged(string link, string name, string color, bool enabled)
    
    Layout.fillWidth: true
    Layout.preferredHeight: 100
    
    GlassPanel {
        anchors.fill: parent
        glassOpacity: root.enabled ? 0.1 : 0.05
        borderColor: root.enabled ? "#30ffffff" : "#15ffffff"
        cornerRadius: 8
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8
        
        // 第一行：启用开关、链接选择、删除按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            // 启用开关
            Rectangle {
                width: 24
                height: 24
                radius: 4
                color: root.enabled ? "#2000ff88" : "#20ffffff"
                border.color: root.enabled ? "#00ff88" : "#40ffffff"
                border.width: 1
                
                Text {
                    anchors.centerIn: parent
                    text: root.enabled ? "✓" : ""
                    color: "#00ff88"
                    font.pixelSize: FontConfig.normal
                    font.weight: Font.Bold
                }
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.configChanged(root.linkName, root.displayName, root.colorHex, !root.enabled)
                }
            }
            
            // 链接选择下拉框
            GlassComboBox {
                Layout.fillWidth: true
                Layout.minimumWidth: 150
                height: 28
                model: root.availableLinks
                currentValue: root.linkName
                placeholder: qsTr("选择链接")
                popupWidth: 250
                
                onValueChanged: function(value) {
                    if (value && value !== root.linkName) {
                        root.configChanged(value, root.displayName, root.colorHex, root.enabled)
                    }
                }
            }
            
            // 删除按钮
            GlassButton {
                width: 28
                height: 28
                iconText: "✕"
                accentColor: "#ff6b6b"
                onClicked: root.removeClicked()
            }
        }
        
        // 第二行：显示名称和颜色选择
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            // 显示名称输入
            Rectangle {
                Layout.fillWidth: true
                Layout.minimumWidth: 120
                height: 28
                radius: 6
                color: "#15ffffff"
                border.color: nameInput.activeFocus ? "#00ff88" : "#30ffffff"
                border.width: 1
                
                TextInput {
                    id: nameInput
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    text: root.displayName
                    color: "#ffffff"
                    font.pixelSize: FontConfig.small
                    verticalAlignment: Text.AlignVCenter
                    selectByMouse: true
                    
                    property string placeholderText: qsTr("显示名称")
                    
                    Text {
                        anchors.fill: parent
                        text: nameInput.placeholderText
                        color: "#60ffffff"
                        font.pixelSize: FontConfig.small
                        verticalAlignment: Text.AlignVCenter
                        visible: !nameInput.text && !nameInput.activeFocus
                    }
                    
                    onTextChanged: {
                        if (text !== root.displayName) {
                            root.configChanged(root.linkName, text, root.colorHex, root.enabled)
                        }
                    }
                }
            }
            
            // 颜色选择
            Rectangle {
                width: 60
                height: 28
                radius: 6
                color: root.colorHex
                border.color: "#60ffffff"
                border.width: 1
                
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: colorPopup.open()
                }
                
                Popup {
                    id: colorPopup
                    x: -120
                    y: parent.height + 4
                    width: 180
                    height: 120
                    
                    background: Rectangle {
                        color: "#1a1a2e"
                        border.color: "#40ffffff"
                        radius: 8
                    }
                    
                    contentItem: GridLayout {
                        columns: 4
                        rowSpacing: 8
                        columnSpacing: 8
                        
                        Repeater {
                            model: ["#FFFF00", "#00FFFF", "#FF00FF", "#FF8000",
                                    "#80FF00", "#00FF80", "#8000FF", "#FF0080",
                                    "#FF0000", "#00FF00", "#0080FF", "#FFFFFF"]
                            
                            Rectangle {
                                width: 32
                                height: 32
                                radius: 4
                                color: modelData
                                border.color: root.colorHex === modelData ? "#ffffff" : "#40ffffff"
                                border.width: root.colorHex === modelData ? 2 : 1
                                
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.configChanged(root.linkName, root.displayName, modelData, root.enabled)
                                        colorPopup.close()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

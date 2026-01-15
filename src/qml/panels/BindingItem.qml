import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "../components"

// 绑定项组件
Item {
    id: root
    
    property string jointName: ""
    property string nodeId: ""
    property bool enabled: true
    property var availableJoints: []
    
    signal removeClicked()
    signal bindingChanged(string joint, string node, bool enabled)
    
    height: 80
    
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
                    onClicked: root.bindingChanged(root.jointName, root.nodeId, !root.enabled)
                }
            }
            
            // 关节选择
            GlassComboBox {
                Layout.fillWidth: true
                height: 28
                model: root.availableJoints
                currentValue: root.jointName
                placeholder: qsTr("选择关节")
                popupWidth: 250
                
                onValueChanged: function(value) {
                    if (value !== root.jointName) {
                        root.bindingChanged(value, root.nodeId, root.enabled)
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
        
        // 节点ID输入
        Rectangle {
            Layout.fillWidth: true
            height: 28
            radius: 6
            color: "#15ffffff"
            border.color: nodeInput.activeFocus ? "#00ff88" : "#30ffffff"
            border.width: 1
            
            TextInput {
                id: nodeInput
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 60
                text: root.nodeId
                color: "#ffffff"
                font.pixelSize: FontConfig.small
                font.family: "Consolas"
                verticalAlignment: Text.AlignVCenter
                selectByMouse: true
                clip: true
                
                onTextChanged: {
                    if (text !== root.nodeId) {
                        root.bindingChanged(root.jointName, text, root.enabled)
                    }
                }
            }
            
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                text: "NodeID"
                color: "#50ffffff"
                font.pixelSize: FontConfig.tiny
                visible: nodeInput.text === ""
            }
        }
    }
}

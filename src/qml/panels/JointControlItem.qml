import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import "../components"

// 单个关节控制项
Item {
    id: root
    
    property int jointIndex: 0
    property string jointName: ""
    property real jointValue: 0
    property real jointMin: -180
    property real jointMax: 180
    property string jointType: "R"
    
    signal valueChanged(string name, real value)
    
    height: 110
    
    // 预定义颜色
    property var jointColors: [
        "#ff6b6b", "#4ecdc4", "#45b7d1", "#96ceb4",
        "#ffeaa7", "#dfe6e9", "#fd79a8", "#a29bfe",
        "#00b894", "#e17055", "#74b9ff", "#55efc4"
    ]
    property color itemColor: jointColors[jointIndex % jointColors.length] === undefined ? "#ffffff"
                                                                                         : jointColors[jointIndex % jointColors.length]
    
    GlassPanel {
        anchors.fill: parent
        glassOpacity: 0.75
        borderColor: "#30ffffff"
        cornerRadius: 8
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 6
        
        // 顶行：序号、名称和当前值
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            
            // 序号
            Rectangle {
                width: 28
                height: 28
                radius: 6
                color: Qt.rgba(root.itemColor.r, root.itemColor.g, root.itemColor.b, 0.2)
                border.color: root.itemColor
                border.width: 1
                
                Text {
                    anchors.centerIn: parent
                    text: (root.jointIndex + 1).toString().padStart(2, '0')
                    color: root.itemColor
                    font.pixelSize: FontConfig.tiny
                    font.weight: Font.Bold
                    font.family: "Consolas"
                }
            }
            
            // 关节名称
            Column {
                Layout.fillWidth: true
                spacing: 2
                
                Text {
                    text: root.jointName
                    color: "#ffffff"
                    font.pixelSize: FontConfig.normal
                    font.weight: Font.Medium
                    elide: Text.ElideRight
                    width: parent.width
                }
                
                Text {
                    text: {
                        switch(root.jointType) {
                        case "R": return qsTr("旋转关节")
                        case "P": return qsTr("平移关节")
                        case "C": return qsTr("连续关节")
                        default: return qsTr("未知类型")
                        }
                    }
                    color: "#80ffffff"
                    font.pixelSize: FontConfig.small
                }
            }

            // 当前值显示/编辑
            Rectangle {
                width: 80
                height: 28
                radius: 6
                color: "#15ffffff"
                border.color: valueInput.activeFocus ? root.itemColor : "#30ffffff"
                border.width: 1

                TextInput {
                    id: valueInput
                    anchors.centerIn: parent
                    width: parent.width - 20
                    text: root.jointValue.toFixed(2)
                    color: root.itemColor
                    font.pixelSize: FontConfig.normal
                    font.weight: Font.Bold
                    font.family: "Consolas"
                    horizontalAlignment: Text.AlignHCenter
                    selectByMouse: true

                    validator: DoubleValidator {
                        bottom: root.jointMin
                        top: root.jointMax
                    }

                    onEditingFinished: {
                        var val = parseFloat(text)
                        if (!isNaN(val)) {
                            root.valueChanged(root.jointName, val)
                        }
                    }
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.jointType === "P" ? "m" : "°"
                    color: "#80ffffff"
                    font.pixelSize: FontConfig.small
                }
            }
        }

        // 滑块区域
        Item {
            id: sliderContainer
            Layout.fillWidth: true
            Layout.preferredHeight: 36

            // 轨道背景
            Rectangle {
                id: trackBg
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                height: 8
                radius: 4
                color: "#15ffffff"

                // 进度条
                Rectangle {
                    width: {
                        var range = root.jointMax - root.jointMin
                        if (range === 0) return 0
                        return Math.max(0, Math.min(1, (root.jointValue - root.jointMin) / range)) * parent.width
                    }
                    height: parent.height
                    radius: parent.radius

                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Qt.darker(root.itemColor, 1.5) }
                        GradientStop { position: 1.0; color: root.itemColor }
                    }
                }

                // 刻度线
                Row {
                    anchors.fill: parent

                    Repeater {
                        model: 5

                        Item {
                            width: parent.width / 4
                            height: parent.height
                            visible: index > 0

                            Rectangle {
                                anchors.right: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: 1
                                height: 12
                                color: "#20ffffff"
                            }
                        }
                    }
                }
            }

            // 手柄（仅视觉效果）
            Rectangle {
                id: handle
                width: 20
                height: 20
                radius: 10
                x: {
                    var range = root.jointMax - root.jointMin
                    if (range === 0) return 0
                    return Math.max(0, Math.min(1, (root.jointValue - root.jointMin) / range)) * (sliderContainer.width - width)
                }
                anchors.verticalCenter: parent.verticalCenter

                gradient: Gradient {
                    GradientStop { position: 0.0; color: sliderArea.pressed ? root.itemColor : "#ffffff" }
                    GradientStop { position: 1.0; color: sliderArea.pressed ? Qt.darker(root.itemColor, 1.2) : "#cccccc" }
                }

                border.color: (sliderArea.containsMouse || sliderArea.pressed) ? root.itemColor : "#60ffffff"
                border.width: 2

                scale: sliderArea.pressed ? 1.1 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }

                layer.enabled: sliderArea.pressed
                layer.effect: Glow {
                    radius: 12
                    samples: 17
                    color: root.itemColor
                    spread: 0.3
                }
            }

            // 鼠标交互区域 - 放在最上层
            MouseArea {
                id: sliderArea
                anchors.fill: parent
                anchors.topMargin: -10
                anchors.bottomMargin: -10
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                preventStealing: true

                property real startX: 0
                property real startValue: 0

                onPressed: {
                    startX = mouseX
                    startValue = root.jointValue
                    updateValueFromMouse(mouseX)
                }

                onPositionChanged: {
                    if (pressed) {
                        updateValueFromMouse(mouseX)
                    }
                }

                function updateValueFromMouse(mx) {
                    // 计算相对于滑块容器的位置
                    var effectiveWidth = sliderContainer.width
                    var clampedX = Math.max(0, Math.min(effectiveWidth, mx))
                    var ratio = clampedX / effectiveWidth
                    var newVal = root.jointMin + ratio * (root.jointMax - root.jointMin)
                    root.valueChanged(root.jointName, newVal)
                }
            }
        }

        // 范围标签
        RowLayout {
            Layout.fillWidth: true
            
            Text {
                text: root.jointMin.toFixed(0) + (root.jointType === "P" ? "m" : "°")
                color: "#a0ffffff"
                font.pixelSize: FontConfig.tiny
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.jointMax.toFixed(0) + (root.jointType === "P" ? "m" : "°")
                color: "#a0ffffff"
                font.pixelSize: FontConfig.tiny
            }
        }
    }
}


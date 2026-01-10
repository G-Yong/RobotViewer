import QtQuick 2.15

// 单个关节信息条目
Item {
    id: root
    
    property int jointIndex: 0
    property string jointName: ""
    property real jointValue: 0
    property real jointMin: -180
    property real jointMax: 180
    property string jointType: "R"  // R=Revolute, P=Prismatic, C=Continuous
    
    signal clicked()
    
    height: 52
    
    // 预定义颜色列表
    property var jointColors: [
        "#ff6b6b", "#4ecdc4", "#45b7d1", "#96ceb4", 
        "#ffeaa7", "#dfe6e9", "#fd79a8", "#a29bfe",
        "#00b894", "#e17055", "#74b9ff", "#55efc4"
    ]
    
    property color itemColor: jointColors[jointIndex % jointColors.length]
    
    Rectangle {
        anchors.fill: parent
        radius: 6
        color: mouseArea.containsMouse ? "#20ffffff" : "transparent"
        border.color: mouseArea.containsMouse ? "#30ffffff" : "transparent"
        border.width: 1
        
        Behavior on color {
            ColorAnimation { duration: 150 }
        }
    }
    
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    
    Row {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 8
        
        // 序号和类型标识
        Rectangle {
            width: 36
            height: 36
            radius: 6
            color: Qt.rgba(root.itemColor.r, root.itemColor.g, root.itemColor.b, 0.2)
            border.color: root.itemColor
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter
            
            Column {
                anchors.centerIn: parent
                spacing: 1
                
                Text {
                    text: (root.jointIndex + 1).toString().padStart(2, '0')
                    color: root.itemColor
                    font.pixelSize: 12
                    font.weight: Font.Bold
                    font.family: "Consolas"
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
                Text {
                    text: "[" + root.jointType + "]"
                    color: Qt.rgba(root.itemColor.r, root.itemColor.g, root.itemColor.b, 0.7)
                    font.pixelSize: 8
                    font.weight: Font.Medium
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }
        
        // 关节信息
        Column {
            width: parent.width - 44 - 60
            spacing: 4
            anchors.verticalCenter: parent.verticalCenter
            
            // 关节名称
            Text {
                text: root.jointName
                color: "#ffffff"
                font.pixelSize: 11
                font.weight: Font.Medium
                elide: Text.ElideRight
                width: parent.width
            }
            
            // 进度条
            Rectangle {
                width: parent.width
                height: 4
                radius: 2
                color: "#20ffffff"
                
                Rectangle {
                    width: {
                        var range = root.jointMax - root.jointMin
                        if (range === 0) return 0
                        var ratio = (root.jointValue - root.jointMin) / range
                        return Math.max(0, Math.min(1, ratio)) * parent.width
                    }
                    height: parent.height
                    radius: parent.radius
                    color: root.itemColor
                    
                    Behavior on width {
                        NumberAnimation { duration: 100 }
                    }
                }
            }
            
            // 范围标签
            Row {
                width: parent.width
                
                Text {
                    text: root.jointMin.toFixed(1) + (root.jointType === "P" ? "m" : "°")
                    color: "#40ffffff"
                    font.pixelSize: 8
                }
                
                Item { 
                    width: parent.width - minLabelWidth.width - maxLabelWidth.width
                    height: 1 
                }
                
                Text {
                    id: minLabelWidth
                    text: ""
                    visible: false
                }
                
                Text {
                    id: maxLabelWidth
                    text: root.jointMax.toFixed(1) + (root.jointType === "P" ? "m" : "°")
                    color: "#40ffffff"
                    font.pixelSize: 8
                }
            }
        }
        
        // 当前值显示
        Column {
            width: 56
            anchors.verticalCenter: parent.verticalCenter
            
            Text {
                text: root.jointValue.toFixed(2)
                color: root.itemColor
                font.pixelSize: 14
                font.weight: Font.Bold
                font.family: "Consolas"
                anchors.right: parent.right
            }
            
            Text {
                text: root.jointType === "P" ? "m" : "deg"
                color: "#60ffffff"
                font.pixelSize: 9
                anchors.right: parent.right
            }
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

// 关节信息叠加显示（显示在3D场景上方）
Item {
    id: root
    
    property var joints: []  // 关节信息列表 [{name, value, min, max, type}]
    property color accentColor: "#00ff88"
    
    signal jointClicked(string jointName)
    
    implicitWidth: jointList.width + 20
    implicitHeight: Math.min(jointList.height + 20, 500)
    
    // 背景毛玻璃效果
    GlassPanel {
        anchors.fill: parent
        glassOpacity: 0.88
        glassColor: "#0a0a1a"
        borderColor: "#40ffffff"
    }
    
    // 标题
    Item {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        height: 30
        
        Row {
            spacing: 8
            anchors.verticalCenter: parent.verticalCenter
            
            // 关节图标
            Text {
                text: "⚡"
                color: accentColor
                font.pixelSize: 16
                anchors.verticalCenter: parent.verticalCenter
            }
            
            Text {
                text: "JOINTS"
                color: "#ffffff"
                font.pixelSize: 12
                font.weight: Font.Bold
                font.letterSpacing: 2
                anchors.verticalCenter: parent.verticalCenter
            }
            
            // 关节数量徽章
            Rectangle {
                width: countText.width + 12
                height: 18
                radius: 9
                color: "#30" + accentColor.toString().substring(1)
                border.color: accentColor
                border.width: 1
                anchors.verticalCenter: parent.verticalCenter
                
                Text {
                    id: countText
                    anchors.centerIn: parent
                    text: joints.length
                    color: accentColor
                    font.pixelSize: 10
                    font.weight: Font.Bold
                }
            }
        }
    }
    
    // 分隔线
    Rectangle {
        id: separator
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        height: 1
        
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.2; color: "#40ffffff" }
            GradientStop { position: 0.8; color: "#40ffffff" }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }
    
    // 关节列表（滚动区域）
    ScrollView {
        id: scrollView
        anchors.top: separator.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 10
        anchors.topMargin: 5
        
        clip: true
        ScrollBar.vertical.policy: ScrollBar.AsNeeded
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
        
        Column {
            id: jointList
            width: 260
            spacing: 4
            
            Repeater {
                model: joints
                
                JointInfoItem {
                    width: parent.width
                    jointIndex: index
                    jointName: modelData.name
                    jointValue: modelData.value
                    jointMin: modelData.min
                    jointMax: modelData.max
                    jointType: modelData.type
                    
                    onClicked: root.jointClicked(jointName)
                }
            }
        }
    }
    
    // 无数据提示
    Column {
        anchors.centerIn: parent
        spacing: 10
        visible: joints.length === 0
        
        Text {
            text: "⚙"
            color: "#40ffffff"
            font.pixelSize: 32
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        Text {
            text: qsTr("暂无关节数据")
            color: "#60ffffff"
            font.pixelSize: 12
            anchors.horizontalCenter: parent.horizontalCenter
        }
        
        Text {
            text: qsTr("请加载URDF文件")
            color: "#40ffffff"
            font.pixelSize: 10
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

// 加载叠加层
Rectangle {
    id: root
    
    property string message: ""
    
    color: "#d0000000"
    
    // 点击阻止穿透
    MouseArea {
        anchors.fill: parent
    }
    
    Column {
        anchors.centerIn: parent
        spacing: 30
        
        // 旋转加载环
        Item {
            width: 80
            height: 80
            anchors.horizontalCenter: parent.horizontalCenter
            
            // 外圈
            Canvas {
                id: spinnerCanvas
                anchors.fill: parent
                
                property real rotation: 0
                
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    ctx.translate(width/2, height/2)
                    ctx.rotate(rotation * Math.PI / 180)
                    
                    var gradient = ctx.createLinearGradient(-width/2, 0, width/2, 0)
                    gradient.addColorStop(0, "transparent")
                    gradient.addColorStop(0.5, "#00ff88")
                    gradient.addColorStop(1, "#00ff88")
                    
                    ctx.strokeStyle = gradient
                    ctx.lineWidth = 4
                    ctx.lineCap = "round"
                    ctx.beginPath()
                    ctx.arc(0, 0, 35, 0, Math.PI * 1.5)
                    ctx.stroke()
                }
                
                NumberAnimation on rotation {
                    from: 0
                    to: 360
                    duration: 1000
                    loops: Animation.Infinite
                }
                
                onRotationChanged: requestPaint()
            }
            
            // 内部图标
            Text {
                anchors.centerIn: parent
                text: "⚙"
                font.pixelSize: 24
                color: "#00ff88"
                
                RotationAnimation on rotation {
                    from: 0
                    to: -360
                    duration: 2000
                    loops: Animation.Infinite
                }
            }
        }
        
        // 加载文本
        Text {
            text: message
            color: "#ffffff"
            font.pixelSize: 16
            font.weight: Font.Medium
            anchors.horizontalCenter: parent.horizontalCenter
            
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                NumberAnimation { to: 0.5; duration: 800 }
                NumberAnimation { to: 1.0; duration: 800 }
            }
        }
        
        // 进度条（脉冲动画）
        Rectangle {
            width: 200
            height: 3
            color: "#20ffffff"
            radius: 1.5
            anchors.horizontalCenter: parent.horizontalCenter
            
            Rectangle {
                id: progressPulse
                height: parent.height
                radius: parent.radius
                color: "#00ff88"
                
                SequentialAnimation on width {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 200; duration: 1500; easing.type: Easing.InOutQuad }
                }
                
                SequentialAnimation on x {
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 0; duration: 1500 }
                }
            }
        }
    }
    
    // 淡入动画
    opacity: 0
    
    Behavior on visible {
        enabled: root.visible
        SequentialAnimation {
            PropertyAction { target: root; property: "opacity"; value: 0 }
            NumberAnimation { target: root; property: "opacity"; to: 1; duration: 200 }
        }
    }
}

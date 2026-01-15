import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

// 毛玻璃效果面板 - 高质量模拟效果
Item {
    id: root
    
    property real blurRadius: 32
    property real glassOpacity: 0.75
    property color glassColor: "#0d1520"
    property color borderColor: "#60ffffff"
    property real borderWidth: 1
    property real cornerRadius: 12
    
    // 阴影层
    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: cornerRadius + 2
        color: "transparent"
        
        layer.enabled: true
        layer.effect: DropShadow {
            transparentBorder: true
            horizontalOffset: 0
            verticalOffset: 4
            radius: 20
            samples: 25
            color: "#40000000"
        }
    }
    
    // 主背景 - 深色基底
    Rectangle {
        id: baseLayer
        anchors.fill: parent
        radius: cornerRadius
        color: Qt.rgba(glassColor.r, glassColor.g, glassColor.b, glassOpacity)
    }
    
    // 模糊模拟层1 - 使用递归模糊创建柔和效果
    Item {
        id: blurSimulation
        anchors.fill: parent
        
        layer.enabled: true
        layer.effect: FastBlur {
            radius: 0
        }
        
        // 多层渐变模拟光线散射
        Rectangle {
            anchors.fill: parent
            radius: cornerRadius
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#18ffffff" }
                GradientStop { position: 0.15; color: "#08ffffff" }
                GradientStop { position: 0.5; color: "#04ffffff" }
                GradientStop { position: 0.85; color: "#06ffffff" }
                GradientStop { position: 1.0; color: "#10ffffff" }
            }
        }
        
        // 横向渐变叠加
        Rectangle {
            anchors.fill: parent
            radius: cornerRadius
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#08ffffff" }
                GradientStop { position: 0.5; color: "#00ffffff" }
                GradientStop { position: 1.0; color: "#06ffffff" }
            }
        }
    }
    
    // // 噪点纹理 - 模拟磨砂质感
    // Item {
    //     anchors.fill: parent
    //     opacity: 0.015
        
    //     layer.enabled: true
    //     layer.effect: OpacityMask {
    //         maskSource: Rectangle {
    //             width: root.width
    //             height: root.height
    //             radius: root.cornerRadius
    //         }
    //     }
        
    //     Repeater {
    //         model: 50
    //         Rectangle {
    //             x: Math.random() * root.width
    //             y: Math.random() * root.height
    //             width: 1 + Math.random() * 2
    //             height: 1 + Math.random() * 2
    //             radius: width / 2
    //             color: Math.random() > 0.5 ? "#ffffff" : "#000000"
    //             opacity: 0.3 + Math.random() * 0.7
    //         }
    //     }
    // }
    
    // 内发光效果
    Rectangle {
        anchors.fill: parent
        radius: cornerRadius
        color: "transparent"
        
        // 内部渐变边框
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: cornerRadius - 1
            color: "transparent"
            border.width: 1
            border.color: "#15ffffff"
        }
    }
    
    // 顶部高光 - 模拟玻璃反射
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 1
        anchors.leftMargin: cornerRadius * 0.3
        anchors.rightMargin: cornerRadius * 0.3
        height: 1
        radius: 0.5
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.15; color: "#30ffffff" }
            GradientStop { position: 0.5; color: "#60ffffff" }
            GradientStop { position: 0.85; color: "#30ffffff" }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }
    
    // 顶部区域微光
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height * 0.4
        radius: cornerRadius
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0cffffff" }
            GradientStop { position: 1.0; color: "transparent" }
        }
        
        // 只显示顶部圆角部分
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: parent.height - cornerRadius
            color: "transparent"
            visible: false
        }
    }
    
    // 主边框
    Rectangle {
        anchors.fill: parent
        radius: cornerRadius
        color: "transparent"
        border.color: borderColor
        border.width: borderWidth
    }
    
    // 底部阴影线
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottomMargin: 1
        anchors.leftMargin: cornerRadius * 0.5
        anchors.rightMargin: cornerRadius * 0.5
        height: 1
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.3; color: "#20000000" }
            GradientStop { position: 0.7; color: "#20000000" }
            GradientStop { position: 1.0; color: "transparent" }
        }
    }
}

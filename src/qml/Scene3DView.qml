import QtQuick 2.15
import QtQuick.Controls 2.15
import Qt3D.Core 2.15
import Qt3D.Render 2.15
import Qt3D.Input 2.15
import Qt3D.Extras 2.15
import QtQuick.Scene3D 2.15
import RobotViewer 1.0

// 3D场景视图 - 使用 Scene3D 组件，机器人模型和场景元素由C++端创建
Item {
    id: root
    
    property var robotBridge: null
    property int fps: 60
    
    // 场景状态属性（从robotBridge同步）
    property bool showGrid: robotBridge ? robotBridge.showGrid : true
    property bool showAxes: robotBridge ? robotBridge.showAxes : true
    property bool zUpEnabled: robotBridge ? robotBridge.zUpEnabled : false
    
    // 相机属性
    property vector3d cameraPosition: Qt.vector3d(3, 3, 3)
    property vector3d cameraViewCenter: Qt.vector3d(0, 0, 0)
    
    // Scene3D 主组件
    Scene3D {
        id: scene3d
        anchors.fill: parent
        focus: true
        aspects: ["input", "logic"]
        cameraAspectRatioMode: Scene3D.AutomaticAspectRatio
        hoverEnabled: true
        
        Entity {
            id: sceneRoot
            objectName: "qmlSceneRoot"
            
            // 渲染设置
            components: [
                RenderSettings {
                    activeFrameGraph: ForwardRenderer {
                        id: forwardRenderer
                        camera: mainCamera
                        clearColor: "#0a0a15"
                    }
                },
                InputSettings { }
            ]
            
            // 主相机
            Camera {
                id: mainCamera
                projectionType: CameraLens.PerspectiveProjection
                fieldOfView: 45
                nearPlane: 0.01
                farPlane: 1000.0
                position: root.cameraPosition
                viewCenter: root.cameraViewCenter
                upVector: Qt.vector3d(0, 1, 0)
            }
            
            // 轨道相机控制器（使用自定义C++控制器）
            CustomOrbitCameraController {
                id: cameraController
                camera: mainCamera
                rotationSpeed: 0.3
                panSpeed: 0.005
                zoomSpeed: 0.001
            }
            
            // C++ 创建的 Entity 树将被挂载为 sceneRoot 的子节点
            // 通过 robotBridge.attachToSceneRoot(sceneRoot) 实现
            // 包含：worldEntity -> lights, grid, axes, robotEntity, trajectoryEntity
        }
    }
    
    // 场景边缘虚化效果
    Rectangle {
        anchors.fill: parent
        z: 1
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#30000000" }
            GradientStop { position: 0.05; color: "transparent" }
            GradientStop { position: 0.95; color: "transparent" }
            GradientStop { position: 1.0; color: "#30000000" }
        }
    }
    
    // 左右边缘虚化
    Row {
        anchors.fill: parent
        z: 1
        
        Rectangle {
            width: parent.width * 0.05
            height: parent.height
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "#40000000" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
        
        Item { width: parent.width * 0.90; height: parent.height }
        
        Rectangle {
            width: parent.width * 0.05
            height: parent.height
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: "#40000000" }
            }
        }
    }
    
    // 角落装饰框
    Item {
        anchors.fill: parent
        z: 3
        
        // 左上角
        Canvas {
            width: 50
            height: 50
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 15
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.strokeStyle = "#4000ff88"
                ctx.lineWidth = 2
                ctx.lineCap = "square"
                
                ctx.beginPath()
                ctx.moveTo(0, 25)
                ctx.lineTo(0, 0)
                ctx.lineTo(25, 0)
                ctx.stroke()
            }
            
            Component.onCompleted: requestPaint()
        }
        
        // 右上角
        Canvas {
            width: 50
            height: 50
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 15
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.strokeStyle = "#4000ff88"
                ctx.lineWidth = 2
                ctx.lineCap = "square"
                
                ctx.beginPath()
                ctx.moveTo(50, 25)
                ctx.lineTo(50, 0)
                ctx.lineTo(25, 0)
                ctx.stroke()
            }
            
            Component.onCompleted: requestPaint()
        }
        
        // 左下角
        Canvas {
            width: 50
            height: 50
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.margins: 15
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.strokeStyle = "#4000ff88"
                ctx.lineWidth = 2
                ctx.lineCap = "square"
                
                ctx.beginPath()
                ctx.moveTo(0, 25)
                ctx.lineTo(0, 50)
                ctx.lineTo(25, 50)
                ctx.stroke()
            }
            
            Component.onCompleted: requestPaint()
        }
        
        // 右下角
        Canvas {
            width: 50
            height: 50
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 15
            
            onPaint: {
                var ctx = getContext("2d")
                ctx.strokeStyle = "#4000ff88"
                ctx.lineWidth = 2
                ctx.lineCap = "square"
                
                ctx.beginPath()
                ctx.moveTo(50, 25)
                ctx.lineTo(50, 50)
                ctx.lineTo(25, 50)
                ctx.stroke()
            }
            
            Component.onCompleted: requestPaint()
        }
    }
    
    // 暴露给外部的方法
    function resetCamera() {
        mainCamera.position = Qt.vector3d(3, 3, 3)
        mainCamera.viewCenter = Qt.vector3d(0, 0, 0)
        mainCamera.upVector = Qt.vector3d(0, 1, 0)
    }
    
    function fitToModel() {
        // 调整相机以适应模型
        if (robotBridge && robotBridge.robotLoaded) {
            robotBridge.fitCamera()
        }
    }
    
    function setCameraPosition(pos) {
        mainCamera.position = pos
    }
    
    function setCameraViewCenter(center) {
        mainCamera.viewCenter = center
    }
    
    // 监听robotBridge属性变化
    Connections {
        target: robotBridge
        enabled: robotBridge !== null
        
        function onZUpEnabledChanged() {
            root.zUpEnabled = robotBridge.zUpEnabled
        }
        
        function onShowGridChanged() {
            root.showGrid = robotBridge.showGrid
        }
        
        function onShowAxesChanged() {
            root.showAxes = robotBridge.showAxes
        }
        
        function onResetCameraRequested() {
            resetCamera()
        }
        
        function onFitCameraRequested(center, position) {
            // 设置相机位置和观察中心
            mainCamera.viewCenter = center
            mainCamera.position = position
            mainCamera.upVector = Qt.vector3d(0, 1, 0)
            
            // 同步轨道控制器参数
            cameraController.syncFromCamera()
        }
    }
    
    // 挂载C++ Entity到QML Scene3D的函数
    function attachCppScene() {
        if (robotBridge) {
            var cppRoot = robotBridge.sceneRoot
            if (cppRoot) {
                cppRoot.parent = sceneRoot
                console.log("Scene3DView: C++ Entity树已挂载到QML Scene3D")
                
                // 同步初始状态
                root.showGrid = robotBridge.showGrid
                root.showAxes = robotBridge.showAxes
                root.zUpEnabled = robotBridge.zUpEnabled
                return true
            }
        }
        return false
    }
    
    // 延迟重试定时器
    Timer {
        id: attachRetryTimer
        interval: 100
        repeat: true
        running: false
        
        property int retryCount: 0
        
        onTriggered: {
            retryCount++
            console.log("Scene3DView: 尝试挂载 C++ Entity (第" + retryCount + "次)")
            if (attachCppScene()) {
                running = false
                console.log("Scene3DView: 挂载成功!")
            } else if (retryCount >= 30) {  // 最多重试30次（3秒）
                running = false
                console.log("Scene3DView: 挂载失败，放弃重试")
            }
        }
    }
    
    // 初始化：将C++的Entity树挂载到QML Scene3D
    Component.onCompleted: {
        console.log("Scene3DView: 初始化完成")
        
        // 尝试立即挂载
        if (!attachCppScene()) {
            // 如果失败，启动定时器重试
            console.log("Scene3DView: robotBridge 或 sceneRoot 未就绪，启动重试...")
            attachRetryTimer.running = true
        }
    }
}

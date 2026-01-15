#include "orbitcameracontroller.h"
#include <QtMath>
#include <QDebug>

OrbitCameraController::OrbitCameraController(Qt3DCore::QEntity* parent)
    : Qt3DCore::QEntity(parent)
    , m_lookAtCenter(0, 1, 0)
    , m_defaultPosition(0, 1, 0)
    , m_defaultCenter(0, 0, 0)
{
    // 创建鼠标设备和处理器
    m_mouseDevice = new Qt3DInput::QMouseDevice(this);
    m_mouseHandler = new Qt3DInput::QMouseHandler(this);
    m_mouseHandler->setSourceDevice(m_mouseDevice);
    
    connect(m_mouseHandler, &Qt3DInput::QMouseHandler::pressed,
            this, &OrbitCameraController::onMousePressed);
    connect(m_mouseHandler, &Qt3DInput::QMouseHandler::released,
            this, &OrbitCameraController::onMouseReleased);
    connect(m_mouseHandler, &Qt3DInput::QMouseHandler::positionChanged,
            this, &OrbitCameraController::onMouseMoved);
    connect(m_mouseHandler, &Qt3DInput::QMouseHandler::wheel,
            this, &OrbitCameraController::onMouseWheel);
    
    addComponent(m_mouseHandler);
    
    // 创建键盘设备和处理器
    m_keyboardDevice = new Qt3DInput::QKeyboardDevice(this);
    m_keyboardHandler = new Qt3DInput::QKeyboardHandler(this);
    m_keyboardHandler->setSourceDevice(m_keyboardDevice);
    m_keyboardHandler->setFocus(true);
    
    connect(m_keyboardHandler, &Qt3DInput::QKeyboardHandler::pressed,
            this, &OrbitCameraController::onKeyPressed);
    connect(m_keyboardHandler, &Qt3DInput::QKeyboardHandler::released,
            this, &OrbitCameraController::onKeyReleased);
    
    addComponent(m_keyboardHandler);
}

OrbitCameraController::~OrbitCameraController()
{
}

void OrbitCameraController::setCamera(Qt3DRender::QCamera* camera)
{
    if (m_camera == camera) return;
    
    m_camera = camera;
    
    if (m_camera) {
        // 连接相机属性变化信号，以便在相机位置改变时同步轨道参数
        connect(m_camera, &Qt3DRender::QCamera::positionChanged,
                this, &OrbitCameraController::onCameraChanged, Qt::UniqueConnection);
        connect(m_camera, &Qt3DRender::QCamera::viewCenterChanged,
                this, &OrbitCameraController::onCameraChanged, Qt::UniqueConnection);
        
        // 初始同步
        syncFromCamera();
    }
    
    emit cameraChanged();
}

void OrbitCameraController::setRotationSpeed(float speed)
{
    if (qFuzzyCompare(m_rotationSpeed, speed)) return;
    m_rotationSpeed = speed;
    emit rotationSpeedChanged();
}

void OrbitCameraController::setPanSpeed(float speed)
{
    if (qFuzzyCompare(m_panSpeed, speed)) return;
    m_panSpeed = speed;
    emit panSpeedChanged();
}

void OrbitCameraController::setZoomSpeed(float speed)
{
    if (qFuzzyCompare(m_zoomSpeed, speed)) return;
    m_zoomSpeed = speed;
    emit zoomSpeedChanged();
}

void OrbitCameraController::setLookAtCenter(const QVector3D& center)
{
    if (m_lookAtCenter == center) return;
    m_lookAtCenter = center;
    updateCameraPosition();
    emit lookAtCenterChanged();
}

void OrbitCameraController::resetView()
{
    if (!m_camera) return;
    
    m_camera->setPosition(m_defaultPosition);
    m_camera->setViewCenter(m_defaultCenter);
    m_camera->setUpVector(QVector3D(0, 1, 0));
    
    // 重新计算轨道参数
    QVector3D diff = m_defaultPosition - m_defaultCenter;
    m_lookAtCenter = m_defaultCenter;
    m_distance = diff.length();
    
    if (m_distance > 0.001f) {
        m_elevation = qRadiansToDegrees(qAsin(diff.y() / m_distance));
        m_azimuth = qRadiansToDegrees(qAtan2(diff.x(), diff.z()));
    }
}

void OrbitCameraController::setDefaultView(const QVector3D& position, const QVector3D& center)
{
    m_defaultPosition = position;
    m_defaultCenter = center;
}

void OrbitCameraController::fitToBoundingBox(const QVector3D& minPoint, const QVector3D& maxPoint)
{
    if (!m_camera) return;
    
    // 计算包围盒中心和尺寸
    QVector3D center = (minPoint + maxPoint) * 0.5f;
    QVector3D size = maxPoint - minPoint;
    float maxDim = qMax(qMax(size.x(), size.y()), size.z());
    
    // 计算合适的相机距离
    float fov = m_camera->fieldOfView();
    m_distance = maxDim / (2.0f * qTan(qDegreesToRadians(fov / 2.0f))) * 1.5f;
    m_distance = qMax(m_distance, 1.0f);
    
    m_lookAtCenter = center;
    m_azimuth = 45.0f;
    m_elevation = 30.0f;
    
    updateCameraPosition();
    
    // 更新默认视角
    m_defaultCenter = center;
    m_defaultPosition = m_camera->position();
}

void OrbitCameraController::onMousePressed(Qt3DInput::QMouseEvent* event)
{
    m_lastMousePos = QPoint(event->x(), event->y());
    
    switch (event->button()) {
    case Qt3DInput::QMouseEvent::LeftButton:
        m_leftButtonPressed = true;
        break;
    case Qt3DInput::QMouseEvent::MiddleButton:
        m_middleButtonPressed = true;
        break;
    case Qt3DInput::QMouseEvent::RightButton:
        m_rightButtonPressed = true;
        break;
    default:
        break;
    }
}

void OrbitCameraController::onMouseReleased(Qt3DInput::QMouseEvent* event)
{
    switch (event->button()) {
    case Qt3DInput::QMouseEvent::LeftButton:
        m_leftButtonPressed = false;
        break;
    case Qt3DInput::QMouseEvent::MiddleButton:
        m_middleButtonPressed = false;
        break;
    case Qt3DInput::QMouseEvent::RightButton:
        m_rightButtonPressed = false;
        break;
    default:
        break;
    }
}

void OrbitCameraController::onMouseMoved(Qt3DInput::QMouseEvent* event)
{
    QPoint currentPos(event->x(), event->y());
    QPoint delta = currentPos - m_lastMousePos;
    m_lastMousePos = currentPos;
    
    if (m_leftButtonPressed) {
        if (m_shiftPressed) {
            // Shift + 左键：平移（备用方式）
            pan(delta.x(), delta.y());
        } else {
            // 左键：旋转
            rotate(delta.x(), delta.y());
        }
    } else if (m_middleButtonPressed) {
        // 中键：平移
        pan(delta.x(), delta.y());
    } else if (m_rightButtonPressed) {
            // 右键：缩放
            zoom(-delta.y() * 10);
    }
}

void OrbitCameraController::onMouseWheel(Qt3DInput::QWheelEvent* event)
{
    zoom(event->angleDelta().y());
}

void OrbitCameraController::onKeyPressed(Qt3DInput::QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_shiftPressed = true;
    }
}

void OrbitCameraController::onKeyReleased(Qt3DInput::QKeyEvent* event)
{
    if (event->key() == Qt::Key_Shift) {
        m_shiftPressed = false;
    }
}

void OrbitCameraController::rotate(float dx, float dy)
{
    m_azimuth -= dx * m_rotationSpeed;
    m_elevation += dy * m_rotationSpeed;
    
    // 限制垂直角度
    m_elevation = qBound(-89.0f, m_elevation, 89.0f);
    
    // 规范化水平角度
    while (m_azimuth > 180.0f) m_azimuth -= 360.0f;
    while (m_azimuth < -180.0f) m_azimuth += 360.0f;
    
    updateCameraPosition();
}

void OrbitCameraController::pan(float dx, float dy)
{
    if (!m_camera) return;
    
    // 计算相机的右向量和上向量
    QVector3D viewDir = (m_lookAtCenter - m_camera->position()).normalized();
    QVector3D right = QVector3D::crossProduct(viewDir, QVector3D(0, 1, 0)).normalized();
    QVector3D up = QVector3D::crossProduct(right, viewDir).normalized();
    
    // 根据距离调整平移速度（距离越远，平移越快）
    float adjustedPanSpeed = m_panSpeed * m_distance;
    
    // 平移观察中心点
    // 注意：dx为负表示向右移动相机（等效于场景向左移动）
    m_lookAtCenter -= right * dx * adjustedPanSpeed;
    m_lookAtCenter += up * dy * adjustedPanSpeed;
    
    updateCameraPosition();
}

void OrbitCameraController::zoom(float delta)
{
    // 根据当前距离调整缩放速度
    float adjustedZoomSpeed = m_zoomSpeed * m_distance;
    m_distance -= delta * adjustedZoomSpeed;
    
    // 限制最小和最大距离
    m_distance = qMax(0.1f, m_distance);
    m_distance = qMin(1000.0f, m_distance);
    
    updateCameraPosition();
}

void OrbitCameraController::updateCameraPosition()
{
    if (!m_camera) return;
    
    m_updatingCamera = true;  // 防止循环更新
    
    // 从球坐标转换到笛卡尔坐标
    float azimuthRad = qDegreesToRadians(m_azimuth);
    float elevationRad = qDegreesToRadians(m_elevation);
    
    float x = m_distance * qCos(elevationRad) * qSin(azimuthRad);
    float y = m_distance * qSin(elevationRad);
    float z = m_distance * qCos(elevationRad) * qCos(azimuthRad);
    
    QVector3D newPosition = m_lookAtCenter + QVector3D(x, y, z);
    
    m_camera->setPosition(newPosition);
    m_camera->setViewCenter(m_lookAtCenter);
    m_camera->setUpVector(QVector3D(0, 1, 0));
    
    m_updatingCamera = false;
}

void OrbitCameraController::syncFromCamera()
{
    if (!m_camera || m_updatingCamera) return;
    
    // 从相机当前位置计算轨道参数
    QVector3D cameraPos = m_camera->position();
    QVector3D viewCenter = m_camera->viewCenter();
    QVector3D diff = cameraPos - viewCenter;
    
    m_lookAtCenter = viewCenter;
    m_distance = diff.length();
    
    if (m_distance > 0.001f) {
        m_elevation = qRadiansToDegrees(qAsin(qBound(-1.0f, diff.y() / m_distance, 1.0f)));
        m_azimuth = qRadiansToDegrees(qAtan2(diff.x(), diff.z()));
    }
}

void OrbitCameraController::onCameraChanged()
{
    syncFromCamera();
}

#ifndef ORBITCAMERACONTROLLER_H
#define ORBITCAMERACONTROLLER_H

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DInput/QMouseDevice>
#include <Qt3DInput/QMouseHandler>
#include <Qt3DInput/QKeyboardDevice>
#include <Qt3DInput/QKeyboardHandler>
#include <QPoint>
#include <QVector3D>

/**
 * @brief 轨道相机控制器
 * 支持鼠标平移、缩放、旋转操作
 */
class OrbitCameraController : public Qt3DCore::QEntity
{
    Q_OBJECT
    Q_PROPERTY(Qt3DRender::QCamera* camera READ camera WRITE setCamera NOTIFY cameraChanged)
    Q_PROPERTY(float rotationSpeed READ rotationSpeed WRITE setRotationSpeed NOTIFY rotationSpeedChanged)
    Q_PROPERTY(float panSpeed READ panSpeed WRITE setPanSpeed NOTIFY panSpeedChanged)
    Q_PROPERTY(float zoomSpeed READ zoomSpeed WRITE setZoomSpeed NOTIFY zoomSpeedChanged)
    Q_PROPERTY(QVector3D lookAtCenter READ lookAtCenter WRITE setLookAtCenter NOTIFY lookAtCenterChanged)
    
public:
    explicit OrbitCameraController(Qt3DCore::QEntity* parent = nullptr);
    ~OrbitCameraController();
    
    Qt3DRender::QCamera* camera() const { return m_camera; }
    void setCamera(Qt3DRender::QCamera* camera);
    
    float rotationSpeed() const { return m_rotationSpeed; }
    void setRotationSpeed(float speed);
    
    float panSpeed() const { return m_panSpeed; }
    void setPanSpeed(float speed);
    
    float zoomSpeed() const { return m_zoomSpeed; }
    void setZoomSpeed(float speed);
    
    QVector3D lookAtCenter() const { return m_lookAtCenter; }
    void setLookAtCenter(const QVector3D& center);
    
    /**
     * @brief 重置相机视角
     */
    void resetView();
    
    /**
     * @brief 设置默认视角参数
     */
    void setDefaultView(const QVector3D& position, const QVector3D& center);
    
    /**
     * @brief 适配到包围盒
     */
    void fitToBoundingBox(const QVector3D& minPoint, const QVector3D& maxPoint);
    
signals:
    void cameraChanged();
    void rotationSpeedChanged();
    void panSpeedChanged();
    void zoomSpeedChanged();
    void lookAtCenterChanged();
    
private slots:
    void onMousePressed(Qt3DInput::QMouseEvent* event);
    void onMouseReleased(Qt3DInput::QMouseEvent* event);
    void onMouseMoved(Qt3DInput::QMouseEvent* event);
    void onMouseWheel(Qt3DInput::QWheelEvent* event);
    void onKeyPressed(Qt3DInput::QKeyboardHandler* handler);
    
private:
    void updateCameraPosition();
    void rotate(float dx, float dy);
    void pan(float dx, float dy);
    void zoom(float delta);
    
    Qt3DRender::QCamera* m_camera = nullptr;
    Qt3DInput::QMouseDevice* m_mouseDevice = nullptr;
    Qt3DInput::QMouseHandler* m_mouseHandler = nullptr;
    Qt3DInput::QKeyboardDevice* m_keyboardDevice = nullptr;
    Qt3DInput::QKeyboardHandler* m_keyboardHandler = nullptr;
    
    // 相机参数
    QVector3D m_lookAtCenter;
    float m_distance = 5.0f;
    float m_azimuth = 45.0f;    // 水平角度（度）
    float m_elevation = 30.0f;  // 垂直角度（度）
    
    // 控制参数
    float m_rotationSpeed = 0.5f;
    float m_panSpeed = 0.005f;
    float m_zoomSpeed = 0.001f;
    
    // 鼠标状态
    QPoint m_lastMousePos;
    bool m_leftButtonPressed = false;
    bool m_middleButtonPressed = false;
    bool m_rightButtonPressed = false;
    bool m_shiftPressed = false;
    
    // 默认视角
    QVector3D m_defaultPosition;
    QVector3D m_defaultCenter;
};

#endif // ORBITCAMERACONTROLLER_H

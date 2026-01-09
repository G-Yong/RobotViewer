#include "robotscene.h"
#include "robotentity.h"
#include "orbitcameracontroller.h"
#include "trajectoryentity.h"

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <QQuaternion>
#include <QDebug>

RobotScene::RobotScene(QObject* parent)
    : QObject(parent)
{
    // // 设置 OpenGL 版本
    // QSurfaceFormat format;
    // format.setVersion(3, 3);  // 设置 OpenGL 3.3
    // format.setProfile(QSurfaceFormat::CoreProfile);  // 使用 Core Profile
    // format.setRenderableType(QSurfaceFormat::OpenGL);
    // format.setDepthBufferSize(24);
    // format.setSamples(4);  // 4x MSAA

    // // 设置为全局默认
    // QSurfaceFormat::setDefaultFormat(format);

    // 默认Z轴朝上
    setZUpEnabled(true);
}

RobotScene::~RobotScene()
{
    // Qt3DWindow和container会自动管理
}

void RobotScene::initialize()
{
    // 创建3D视图
    m_view = new Qt3DExtras::Qt3DWindow();
    m_view->defaultFrameGraph()->setClearColor(QColor(64, 64, 64));
    
    // 创建Widget容器
    m_container = QWidget::createWindowContainer(m_view);
    m_container->setMinimumSize(400, 300);
    m_container->setFocusPolicy(Qt::StrongFocus);

    
    // 创建根实体
    m_rootEntity = new Qt3DCore::QEntity();

    // 创建可整体旋转的世界根，用于切换Y-up / Z-up
    m_worldEntity = new Qt3DCore::QEntity(m_rootEntity);
    m_sceneTransform = new Qt3DCore::QTransform(m_worldEntity);
    m_worldEntity->addComponent(m_sceneTransform);
    setZUpEnabled(false); // 默认保持Y轴朝上
    
    // 设置相机
    m_camera = m_view->camera();
    m_camera->lens()->setPerspectiveProjection(45.0f, 16.0f / 9.0f, 0.01f, 1000.0f);
    m_camera->setPosition(QVector3D(3, 3, 3));
    m_camera->setViewCenter(QVector3D(0, 0, 0));
    m_camera->setUpVector(QVector3D(0, 1, 0));
    
    // 创建相机控制器
    m_cameraController = new OrbitCameraController(m_rootEntity);
    m_cameraController->setCamera(m_camera);
    
    // 创建场景元素
    createLights();
    createGrid();
    createAxes();

    // 创建机器人实体（挂到世界根上，便于整体旋转）
    m_robotEntity = new RobotEntity(m_worldEntity);

    // 创建轨迹实体
    m_trajectoryEntity = new TrajectoryEntity(m_worldEntity);
    m_trajectoryEntity->setLifetime(2000); // 2秒生命周期
    m_robotEntity->setTrajectoryEntity(m_trajectoryEntity);
    
    connect(m_robotEntity, &RobotEntity::robotLoaded, this, &RobotScene::robotLoaded);
    
    // 设置根实体
    m_view->setRootEntity(m_rootEntity);
}

void RobotScene::createLights()
{
    // 主光源（定向光）
    Qt3DCore::QEntity* lightEntity1 = new Qt3DCore::QEntity(m_worldEntity);
    Qt3DRender::QDirectionalLight* directionalLight = new Qt3DRender::QDirectionalLight(lightEntity1);
    directionalLight->setColor(QColor(255, 255, 255));
    directionalLight->setIntensity(0.8f);
    directionalLight->setWorldDirection(QVector3D(-1, -1, -1).normalized());
    lightEntity1->addComponent(directionalLight);
    
    // 辅助光源（定向光）
    Qt3DCore::QEntity* lightEntity2 = new Qt3DCore::QEntity(m_worldEntity);
    Qt3DRender::QDirectionalLight* fillLight = new Qt3DRender::QDirectionalLight(lightEntity2);
    fillLight->setColor(QColor(200, 200, 255));
    fillLight->setIntensity(0.3f);
    fillLight->setWorldDirection(QVector3D(1, 0.5, 1).normalized());
    lightEntity2->addComponent(fillLight);
    
    // 点光源（提供额外照明）
    Qt3DCore::QEntity* lightEntity3 = new Qt3DCore::QEntity(m_worldEntity);
    Qt3DRender::QPointLight* pointLight = new Qt3DRender::QPointLight(lightEntity3);
    pointLight->setColor(QColor(255, 255, 255));
    pointLight->setIntensity(0.5f);
    Qt3DCore::QTransform* lightTransform = new Qt3DCore::QTransform(lightEntity3);
    lightTransform->setTranslation(QVector3D(0, 5, 0));
    lightEntity3->addComponent(pointLight);
    lightEntity3->addComponent(lightTransform);
}

void RobotScene::createGrid()
{
    m_gridEntity = new Qt3DCore::QEntity(m_rootEntity);
    
    // 创建网格线的几何体
    Qt3DRender::QGeometry* geometry = new Qt3DRender::QGeometry(m_gridEntity);
    
    // 网格参数
    const float gridSize = 5.0f;
    const float gridStep = 0.5f;
    const float gridYOffset = -0.001f; // 网格稍微下移，避免与坐标轴Z-fighting
    const int lineCount = static_cast<int>(gridSize * 2 / gridStep) + 1;
    
    // 创建顶点数据
    QByteArray vertexData;
    vertexData.resize(lineCount * 4 * 3 * sizeof(float)); // 每条线2个顶点，每个顶点3个float，共2个方向
    float* vertices = reinterpret_cast<float*>(vertexData.data());
    
    int idx = 0;
    for (int i = 0; i < lineCount; ++i) {
        float pos = -gridSize + i * gridStep;
        
        // X方向的线
        vertices[idx++] = -gridSize;
        vertices[idx++] = gridYOffset;
        vertices[idx++] = pos;
        
        vertices[idx++] = gridSize;
        vertices[idx++] = gridYOffset;
        vertices[idx++] = pos;
        
        // Z方向的线
        vertices[idx++] = pos;
        vertices[idx++] = gridYOffset;
        vertices[idx++] = -gridSize;
        
        vertices[idx++] = pos;
        vertices[idx++] = gridYOffset;
        vertices[idx++] = gridSize;
    }
    
    Qt3DRender::QBuffer* vertexBuffer = new Qt3DRender::QBuffer(geometry);
    vertexBuffer->setData(vertexData);
    
    Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(geometry);
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(vertexBuffer);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(lineCount * 4);
    geometry->addAttribute(positionAttribute);
    
    // 创建几何渲染器
    Qt3DRender::QGeometryRenderer* renderer = new Qt3DRender::QGeometryRenderer(m_gridEntity);
    renderer->setGeometry(geometry);
    renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
    m_gridEntity->addComponent(renderer);
    
    // 材质
    Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(m_gridEntity);
    material->setAmbient(QColor(80, 80, 80));
    material->setDiffuse(QColor(100, 100, 100));
    m_gridEntity->addComponent(material);
}

void RobotScene::createAxes()
{
    m_axesEntity = new Qt3DCore::QEntity(m_worldEntity);
    
    const float axisLength = 1.0f;

    // 为每个轴创建单独的线条实体，这样可以设置不同的颜色
    // X轴 - 红色
    {
        Qt3DCore::QEntity* xAxisEntity = new Qt3DCore::QEntity(m_axesEntity);
        Qt3DRender::QGeometry* xGeometry = new Qt3DRender::QGeometry(xAxisEntity);
        
        QByteArray xVertexData;
        xVertexData.resize(2 * 3 * sizeof(float));
        float* xVerts = reinterpret_cast<float*>(xVertexData.data());
        xVerts[0] = 0; xVerts[1] = 0; xVerts[2] = 0;
        xVerts[3] = axisLength; xVerts[4] = 0; xVerts[5] = 0;
        
        Qt3DRender::QBuffer* xBuffer = new Qt3DRender::QBuffer(xGeometry);
        xBuffer->setData(xVertexData);
        
        Qt3DRender::QAttribute* xPosAttr = new Qt3DRender::QAttribute(xGeometry);
        xPosAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        xPosAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        xPosAttr->setVertexSize(3);
        xPosAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        xPosAttr->setBuffer(xBuffer);
        xPosAttr->setByteStride(3 * sizeof(float));
        xPosAttr->setCount(2);
        xGeometry->addAttribute(xPosAttr);
        
        Qt3DRender::QGeometryRenderer* xRenderer = new Qt3DRender::QGeometryRenderer(xAxisEntity);
        xRenderer->setGeometry(xGeometry);
        xRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        xAxisEntity->addComponent(xRenderer);
        
        Qt3DExtras::QPhongMaterial* xMat = new Qt3DExtras::QPhongMaterial(xAxisEntity);
        xMat->setAmbient(QColor(255, 0, 0));
        xMat->setDiffuse(QColor(255, 0, 0));
        xAxisEntity->addComponent(xMat);
    }
    
    // Y轴 - 绿色
    {
        Qt3DCore::QEntity* yAxisEntity = new Qt3DCore::QEntity(m_axesEntity);
        Qt3DRender::QGeometry* yGeometry = new Qt3DRender::QGeometry(yAxisEntity);
        
        QByteArray yVertexData;
        yVertexData.resize(2 * 3 * sizeof(float));
        float* yVerts = reinterpret_cast<float*>(yVertexData.data());
        yVerts[0] = 0; yVerts[1] = 0; yVerts[2] = 0;
        yVerts[3] = 0; yVerts[4] = axisLength; yVerts[5] = 0;
        
        Qt3DRender::QBuffer* yBuffer = new Qt3DRender::QBuffer(yGeometry);
        yBuffer->setData(yVertexData);
        
        Qt3DRender::QAttribute* yPosAttr = new Qt3DRender::QAttribute(yGeometry);
        yPosAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        yPosAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        yPosAttr->setVertexSize(3);
        yPosAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        yPosAttr->setBuffer(yBuffer);
        yPosAttr->setByteStride(3 * sizeof(float));
        yPosAttr->setCount(2);
        yGeometry->addAttribute(yPosAttr);
        
        Qt3DRender::QGeometryRenderer* yRenderer = new Qt3DRender::QGeometryRenderer(yAxisEntity);
        yRenderer->setGeometry(yGeometry);
        yRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        yAxisEntity->addComponent(yRenderer);
        
        Qt3DExtras::QPhongMaterial* yMat = new Qt3DExtras::QPhongMaterial(yAxisEntity);
        yMat->setAmbient(QColor(0, 255, 0));
        yMat->setDiffuse(QColor(0, 255, 0));
        yAxisEntity->addComponent(yMat);
    }
    
    // Z轴 - 蓝色
    {
        Qt3DCore::QEntity* zAxisEntity = new Qt3DCore::QEntity(m_axesEntity);
        Qt3DRender::QGeometry* zGeometry = new Qt3DRender::QGeometry(zAxisEntity);
        
        QByteArray zVertexData;
        zVertexData.resize(2 * 3 * sizeof(float));
        float* zVerts = reinterpret_cast<float*>(zVertexData.data());
        zVerts[0] = 0; zVerts[1] = 0; zVerts[2] = 0;
        zVerts[3] = 0; zVerts[4] = 0; zVerts[5] = axisLength;
        
        Qt3DRender::QBuffer* zBuffer = new Qt3DRender::QBuffer(zGeometry);
        zBuffer->setData(zVertexData);
        
        Qt3DRender::QAttribute* zPosAttr = new Qt3DRender::QAttribute(zGeometry);
        zPosAttr->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
        zPosAttr->setVertexBaseType(Qt3DRender::QAttribute::Float);
        zPosAttr->setVertexSize(3);
        zPosAttr->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        zPosAttr->setBuffer(zBuffer);
        zPosAttr->setByteStride(3 * sizeof(float));
        zPosAttr->setCount(2);
        zGeometry->addAttribute(zPosAttr);
        
        Qt3DRender::QGeometryRenderer* zRenderer = new Qt3DRender::QGeometryRenderer(zAxisEntity);
        zRenderer->setGeometry(zGeometry);
        zRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);
        zAxisEntity->addComponent(zRenderer);
        
        Qt3DExtras::QPhongMaterial* zMat = new Qt3DExtras::QPhongMaterial(zAxisEntity);
        zMat->setAmbient(QColor(0, 0, 255));
        zMat->setDiffuse(QColor(0, 0, 255));
        zAxisEntity->addComponent(zMat);
    }
}

bool RobotScene::loadRobot(const QString& urdfFile)
{
    if (!m_robotEntity) return false;
    
    bool success = m_robotEntity->loadFromURDF(urdfFile);
    
    if (success) {
        // 自动缩放模型
        if (m_autoScaleEnabled) {
            float modelSize = m_robotEntity->getModelSize();
            if (modelSize > 0.001f) {  // 避免除零
                float scale = m_targetModelSize / modelSize;
                m_robotEntity->setScale(scale);
                qDebug() << "Model size:" << modelSize << "Scale factor:" << scale;
            }
        }
        
        // 加载成功后适配相机视角
        fitCameraToRobot();

        qDebug() << "load completed";
        // 应用各种显示设置
        setGridVisible(m_gridVisible);
        setAxesVisible(m_axesVisible);
        setJointAxesVisible(m_jointAxesVisible);
        setColoredLinksEnabled(m_coloredLinksEnabled);
        // setZUpEnabled(m_zUpEnabled);
    } else {
        emit loadError(m_robotEntity->getErrorMessage());
    }
    
    return success;
}

void RobotScene::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    if (m_gridEntity) {
        m_gridEntity->setEnabled(visible);
    }
}

void RobotScene::setAxesVisible(bool visible)
{
    m_axesVisible = visible;
    if (m_axesEntity) {
        m_axesEntity->setEnabled(visible);
    }
}

void RobotScene::setJointAxesVisible(bool visible)
{
    m_jointAxesVisible = visible;
    if (m_robotEntity) {
        m_robotEntity->setJointAxesVisible(visible);
    }
}

void RobotScene::setColoredLinksEnabled(bool enabled)
{
    m_coloredLinksEnabled = enabled;
    if (m_robotEntity) {
        m_robotEntity->setColoredLinksEnabled(enabled);
    }
}

void RobotScene::setZUpEnabled(bool enabled)
{
    m_zUpEnabled = enabled;
    if (m_sceneTransform) {
        if (enabled) {
            // 绕X轴旋转：原Y轴变为Z轴朝上
            m_sceneTransform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), -90.0f));
        } else {
            m_sceneTransform->setRotation(QQuaternion());
        }
    }
}

void RobotScene::setAutoScaleEnabled(bool enabled)
{
    m_autoScaleEnabled = enabled;
}

void RobotScene::setTargetModelSize(float size)
{
    m_targetModelSize = size;
}

void RobotScene::setTrajectoryVisible(bool visible)
{
    // 处理单个轨迹（向后兼容）
    if (m_trajectoryEntity) {
        m_trajectoryEntity->setEnabled(visible);
    }
    
    // 处理多末端执行器轨迹
    for (auto trajectory : m_endEffectorTrajectories) {
        if (trajectory) {
            trajectory->setEnabled(visible);
        }
    }
    
    if (m_robotEntity) {
        m_robotEntity->setTrajectoryEnabled(visible);
    }
}

void RobotScene::setTrajectoryLifetime(float seconds)
{
    m_trajectoryLifetime = seconds;
    int msec = static_cast<int>(seconds * 1000);
    
    // 更新单个轨迹
    if (m_trajectoryEntity) {
        m_trajectoryEntity->setLifetime(msec);
    }
    
    // 更新所有多末端执行器轨迹
    for (auto trajectory : m_endEffectorTrajectories) {
        if (trajectory) {
            trajectory->setLifetime(msec);
        }
    }
}

TrajectoryEntity* RobotScene::addEndEffectorTrajectory(const QString& linkName, 
                                                        const QString& name,
                                                        const QColor& color)
{
    if (linkName.isEmpty()) {
        qWarning() << "Cannot add trajectory for empty link name";
        return nullptr;
    }
    
    // 如果已存在，先移除
    if (m_endEffectorTrajectories.contains(linkName)) {
        removeEndEffectorTrajectory(linkName);
    }
    
    // 创建新的轨迹实体
    TrajectoryEntity* trajectory = new TrajectoryEntity(m_worldEntity);
    trajectory->setLifetime(static_cast<int>(m_trajectoryLifetime * 1000));
    
    // 设置颜色（如果指定）
    if (color.isValid()) {
        trajectory->setColor(color);
    } else {
        // 使用预定义的颜色方案
        static const QColor defaultColors[] = {
            QColor(255, 255, 0),   // 黄色
            QColor(0, 255, 255),   // 青色
            QColor(255, 0, 255),   // 品红
            QColor(255, 128, 0),   // 橙色
            QColor(128, 255, 0),   // 黄绿
            QColor(0, 255, 128),   // 春绿
            QColor(128, 0, 255),   // 紫色
            QColor(255, 0, 128)    // 玫红
        };
        int colorIndex = m_endEffectorTrajectories.size() % 8;
        trajectory->setColor(defaultColors[colorIndex]);
    }
    
    m_endEffectorTrajectories[linkName] = trajectory;
    
    // 添加到机器人实体
    if (m_robotEntity) {
        m_robotEntity->addEndEffector(linkName, trajectory, name);
    }
    
    qDebug() << "Added trajectory for end effector:" << linkName;
    return trajectory;
}

void RobotScene::removeEndEffectorTrajectory(const QString& linkName)
{
    if (!m_endEffectorTrajectories.contains(linkName)) {
        return;
    }
    
    TrajectoryEntity* trajectory = m_endEffectorTrajectories.value(linkName);
    if (trajectory) {
        trajectory->deleteLater();
    }
    
    m_endEffectorTrajectories.remove(linkName);
    
    if (m_robotEntity) {
        m_robotEntity->removeEndEffector(linkName);
    }
}

void RobotScene::clearEndEffectorTrajectories()
{
    for (auto trajectory : m_endEffectorTrajectories) {
        if (trajectory) {
            trajectory->deleteLater();
        }
    }
    
    m_endEffectorTrajectories.clear();
    
    if (m_robotEntity) {
        m_robotEntity->clearEndEffectors();
    }
}

QMap<QString, TrajectoryEntity*> RobotScene::getEndEffectorTrajectories() const
{
    return m_endEffectorTrajectories;
}

QMatrix4x4 RobotScene::getWorldMatrix(Qt3DCore::QEntity *entity) const
{
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    
    Qt3DCore::QEntity* current = entity;
    while (current) {
        if (current == m_rootEntity) {
            break;
        }
        
        auto transforms = current->componentsOfType<Qt3DCore::QTransform>();
        if (transforms.length() > 0) {
            auto transform = transforms.first();
            matrix = transform->matrix() * matrix; // 注意乘法顺序
        }
        current = current->parentEntity();
    }
    return matrix;
}

void RobotScene::resetCamera()
{
    if (m_cameraController) {
        m_cameraController->resetView();
    }
}

void RobotScene::fitCameraToRobot()
{
    if (!m_cameraController || !m_robotEntity) return;
    
    // 简单地设置一个默认的合适视角
    // 如果需要更精确的适配，可以从模型获取包围盒信息
    m_cameraController->setDefaultView(QVector3D(-1.5, 1.8, 0.5), QVector3D(0, 0.5, 0));
    m_cameraController->resetView();
}

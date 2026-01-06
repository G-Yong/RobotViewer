#include "trajectoryentity.h"
#include <Qt3DExtras/QPhongMaterial>
#include <QTimer>
#include <QDebug>

TrajectoryEntity::TrajectoryEntity(Qt3DCore::QEntity* parent)
    : Qt3DCore::QEntity(parent)
{
    m_timer.start();
    
    // 创建几何体
    m_geometry = new Qt3DRender::QGeometry(this);
    
    // 创建顶点缓冲区
    m_vertexBuffer = new Qt3DRender::QBuffer(m_geometry);
    
    // 创建位置属性
    m_positionAttribute = new Qt3DRender::QAttribute(m_geometry);
    m_positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    m_positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    m_positionAttribute->setVertexSize(3);
    m_positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_positionAttribute->setBuffer(m_vertexBuffer);
    m_positionAttribute->setByteStride(3 * sizeof(float));
    m_positionAttribute->setCount(0);
    m_geometry->addAttribute(m_positionAttribute);
    
    // 创建几何渲染器
    m_renderer = new Qt3DRender::QGeometryRenderer(this);
    m_renderer->setGeometry(m_geometry);
    m_renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::LineStrip);
    m_renderer->setEnabled(false); // 初始时禁用，直到有足够的点
    addComponent(m_renderer);
    
    // 创建材质
    Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(this);
    material->setDiffuse(m_color);
    material->setAmbient(m_color);
    material->setSpecular(QColor(0, 0, 0)); // 无高光
    addComponent(material);
    
    // 定时更新（清除过期点）
    QTimer* updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &TrajectoryEntity::updateGeometry);
    updateTimer->start(50); // 50ms更新一次
}

TrajectoryEntity::~TrajectoryEntity()
{
}

void TrajectoryEntity::addPoint(const QVector3D& point)
{
    // 添加新点
    TrajectoryPoint tp;
    tp.position = point;
    tp.timestamp = m_timer.elapsed();
    m_points.enqueue(tp);
    
    // 移除过期点
    removeExpiredPoints();
    
    // 限制最大点数
    while (m_points.size() > m_maxPoints) {
        m_points.dequeue();
    }
    
    // 更新几何体
    updateGeometry();
}

void TrajectoryEntity::clear()
{
    m_points.clear();
    updateGeometry();
}

void TrajectoryEntity::setLifetime(int msec)
{
    m_lifetime = msec;
    // 根据50ms采样间隔计算最大点数
    m_maxPoints = msec / 50 + 1;
}

void TrajectoryEntity::setMaxPoints(int maxPoints)
{
    m_maxPoints = maxPoints;
}

void TrajectoryEntity::setColor(const QColor& color)
{
    m_color = color;
    
    // 更新材质颜色
    auto components = this->components();
    for (auto component : components) {
        auto material = qobject_cast<Qt3DExtras::QPhongMaterial*>(component);
        if (material) {
            material->setDiffuse(color);
            material->setAmbient(color);
            break;
        }
    }
}

void TrajectoryEntity::setLineWidth(float width)
{
    m_lineWidth = width;
    // 注意：Qt3D在OpenGL核心模式下可能不支持设置线宽
}

void TrajectoryEntity::removeExpiredPoints()
{
    qint64 currentTime = m_timer.elapsed();
    
    while (!m_points.isEmpty()) {
        const TrajectoryPoint& oldest = m_points.head();
        if (currentTime - oldest.timestamp > m_lifetime) {
            m_points.dequeue();
        } else {
            break;
        }
    }
}

void TrajectoryEntity::updateGeometry()
{
    // 移除过期点
    removeExpiredPoints();
    
    int pointCount = m_points.size();
    
    if (pointCount < 2) {
        // 至少需要2个点才能画线，禁用渲染器避免崩溃
        m_renderer->setEnabled(false);
        return;
    }
    
    // 有足够的点，启用渲染器
    m_renderer->setEnabled(true);
    
    // 创建顶点数据
    QByteArray vertexData;
    vertexData.resize(pointCount * 3 * sizeof(float));
    float* vertices = reinterpret_cast<float*>(vertexData.data());
    
    int idx = 0;
    for (const auto& tp : m_points) {
        vertices[idx++] = tp.position.x();
        vertices[idx++] = tp.position.y();
        vertices[idx++] = tp.position.z();
    }
    
    // 更新缓冲区
    m_vertexBuffer->setData(vertexData);
    m_positionAttribute->setCount(pointCount);
}

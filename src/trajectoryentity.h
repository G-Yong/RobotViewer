#ifndef TRAJECTORYENTITY_H
#define TRAJECTORYENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>
#include <QVector3D>
#include <QQueue>
#include <QElapsedTimer>
#include <QPair>
#include <QColor>

/**
 * @brief 轨迹显示实体
 * 显示末端执行器的运动轨迹
 */
class TrajectoryEntity : public Qt3DCore::QEntity
{
    Q_OBJECT
    
public:
    explicit TrajectoryEntity(Qt3DCore::QEntity* parent = nullptr);
    ~TrajectoryEntity();
    
    /**
     * @brief 添加轨迹点
     * @param point 空间点坐标
     */
    void addPoint(const QVector3D& point);
    
    /**
     * @brief 清除轨迹
     */
    void clear();
    
    /**
     * @brief 设置轨迹生命周期（毫秒）
     * @param msec 生命周期毫秒数
     */
    void setLifetime(int msec);
    int lifetime() const { return m_lifetime; }
    
    /**
     * @brief 设置最大点数
     */
    void setMaxPoints(int maxPoints);
    int maxPoints() const { return m_maxPoints; }
    
    /**
     * @brief 设置轨迹颜色
     */
    void setColor(const QColor& color);
    QColor color() const { return m_color; }
    
    /**
     * @brief 设置线宽（注意：OpenGL核心模式下可能不支持线宽）
     */
    void setLineWidth(float width);
    
private slots:
    void updateGeometry();
    
private:
    void removeExpiredPoints();
    
    // 轨迹点数据：位置 + 时间戳
    struct TrajectoryPoint {
        QVector3D position;
        qint64 timestamp;
    };
    
    QQueue<TrajectoryPoint> m_points;
    
    // 参数
    int m_lifetime = 2000;      // 默认2秒
    int m_maxPoints = 100;      // 最大点数 = lifetime / sampleInterval
    QColor m_color = QColor(255, 255, 0); // 默认黄色
    float m_lineWidth = 2.0f;
    
    // Qt3D组件
    Qt3DRender::QGeometry* m_geometry = nullptr;
    Qt3DRender::QGeometryRenderer* m_renderer = nullptr;
    Qt3DRender::QBuffer* m_vertexBuffer = nullptr;
    Qt3DRender::QAttribute* m_positionAttribute = nullptr;
    
    QElapsedTimer m_timer;
};

#endif // TRAJECTORYENTITY_H

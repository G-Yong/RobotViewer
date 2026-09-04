#ifndef POINTCLOUDENTITY_H
#define POINTCLOUDENTITY_H

#include <Qt3DCore/QEntity>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QMaterial>
#include <Qt3DRender/QEffect>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QRenderPass>
#include <Qt3DRender/QShaderProgram>
#include <Qt3DRender/QParameter>
#include <Qt3DRender/QPointSize>
#include <QColor>
#include <QVector3D>
#include <QList>

#pragma execution_character_set("utf-8")

/**
 * @brief 点云显示实体
 * 解析 ASCII PLY 文件（位置 + RGB），以 Points 图元渲染，支持 RGB 逐点着色。
 * 作为某个 LinkEntity 的子节点使用时，会自动对齐并跟随该 link。
 */
class PointCloudEntity : public Qt3DCore::QEntity
{
    Q_OBJECT

public:
    explicit PointCloudEntity(Qt3DCore::QEntity* parent = nullptr);
    ~PointCloudEntity() override;

    /**
     * @brief 从 ASCII PLY 文件加载点云
     * @param filePath PLY 文件路径
     * @param fallbackColor 无颜色属性时的回退颜色
     * @return 是否成功
     */
    bool loadFromPLY(const QString& filePath, const QColor& fallbackColor = QColor(200, 200, 200));

    void setPointSize(float size);
    float pointSize() const { return m_pointSize; }

    void setVisible(bool visible);
    bool isVisible() const { return isEnabled(); }

    int vertexCount() const { return m_vertexCount; }

private:
    struct PlyHeader {
        QString format;
        int vertexCount = 0;
        struct Property { QString type; QString name; };
        QList<Property> properties;
    };

    bool parseHeader(class QTextStream& in, PlyHeader& header) const;
    bool loadAscii(class QTextStream& in, const PlyHeader& header, const QColor& fallbackColor);
    void createShaderMaterial();

    Qt3DRender::QGeometry* m_geometry = nullptr;
    Qt3DRender::QGeometryRenderer* m_renderer = nullptr;
    Qt3DRender::QBuffer* m_vertexBuffer = nullptr;
    Qt3DRender::QBuffer* m_colorBuffer = nullptr;
    Qt3DRender::QAttribute* m_positionAttribute = nullptr;
    Qt3DRender::QAttribute* m_colorAttribute = nullptr;
    Qt3DRender::QParameter* m_pointSizeParameter = nullptr;
    Qt3DRender::QMaterial* m_material = nullptr;

    QByteArray m_positionData;
    QByteArray m_colorData;
    int m_vertexCount = 0;
    float m_pointSize = 2.0f;
};

#endif // POINTCLOUDENTITY_H

#ifndef ASSIMPMODELLOADER_H
#define ASSIMPMODELLOADER_H

#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DExtras/QPhongMaterial>
#include <QVector3D>
#include <QColor>
#include <QString>
#include <QVector>

struct aiScene;
struct aiNode;
struct aiMesh;

/**
 * @brief Assimp模型加载器
 * 使用Assimp库加载3D模型文件，并转换为Qt3D实体
 */
class AssimpModelLoader
{
public:
    AssimpModelLoader();
    ~AssimpModelLoader();
    
    /**
     * @brief 加载模型文件
     * @param filename 模型文件路径
     * @param parent 父实体
     * @param color 材质颜色
     * @param scale 缩放比例
     * @return 加载的实体，失败返回nullptr
     */
    Qt3DCore::QEntity* loadModel(const QString& filename, 
                                  Qt3DCore::QEntity* parent,
                                  const QColor& color = QColor(128, 128, 128),
                                  const QVector3D& scale = QVector3D(1, 1, 1));
    
    /**
     * @brief 获取错误信息
     */
    QString getErrorMessage() const { return m_errorMessage; }
    
    /**
     * @brief 获取模型的包围盒
     */
    void getBoundingBox(QVector3D& minPoint, QVector3D& maxPoint) const;
    
private:
    void processNode(aiNode* node, const aiScene* scene, 
                    Qt3DCore::QEntity* parent, const QColor& color);
    Qt3DCore::QEntity* processMesh(aiMesh* mesh, const aiScene* scene, 
                                   Qt3DCore::QEntity* parent, const QColor& color);
    
    QString m_errorMessage;
    QVector3D m_minPoint;
    QVector3D m_maxPoint;
    QVector3D m_scale;
};

#endif // ASSIMPMODELLOADER_H

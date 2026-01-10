#include "assimpmodelloader.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <QDebug>
#include <QFileInfo>
#include <limits>

AssimpModelLoader::AssimpModelLoader()
    : m_minPoint(std::numeric_limits<float>::max(), 
                 std::numeric_limits<float>::max(), 
                 std::numeric_limits<float>::max())
    , m_maxPoint(std::numeric_limits<float>::lowest(), 
                 std::numeric_limits<float>::lowest(), 
                 std::numeric_limits<float>::lowest())
    , m_scale(1, 1, 1)
{
}

AssimpModelLoader::~AssimpModelLoader()
{
}

Qt3DCore::QEntity* AssimpModelLoader::loadModel(const QString& filename, 
                                                 Qt3DCore::QEntity* parent,
                                                 const QColor& color,
                                                 const QVector3D& scale)
{
    m_scale = scale;
    m_errorMessage.clear();
    
    // 重置包围盒
    m_minPoint = QVector3D(std::numeric_limits<float>::max(), 
                           std::numeric_limits<float>::max(), 
                           std::numeric_limits<float>::max());
    m_maxPoint = QVector3D(std::numeric_limits<float>::lowest(), 
                           std::numeric_limits<float>::lowest(), 
                           std::numeric_limits<float>::lowest());
    
    QFileInfo fileInfo(filename);
    if (!fileInfo.exists()) {
        m_errorMessage = QString("File does not exist: %1").arg(filename);
        qWarning() << m_errorMessage;
        return nullptr;
    }
    
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(
        filename.toStdString(),
        aiProcess_Triangulate | 
        aiProcess_GenSmoothNormals | 
        aiProcess_FlipUVs |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        m_errorMessage = QString("Assimp error: %1").arg(importer.GetErrorString());
        qWarning() << m_errorMessage;
        return nullptr;
    }
    
    // 创建根实体
    Qt3DCore::QEntity* rootEntity = new Qt3DCore::QEntity(parent);
    
    // 添加缩放变换
    Qt3DCore::QTransform* transform = new Qt3DCore::QTransform(rootEntity);
    transform->setScale3D(scale);
    rootEntity->addComponent(transform);
    
    // 递归处理节点
    processNode(scene->mRootNode, scene, rootEntity, color);
    
    // qDebug() << "Model loaded:" << filename;
    // qDebug() << "  Meshes:" << scene->mNumMeshes;
    // qDebug() << "  Bounding box:" << m_minPoint << "-" << m_maxPoint;
    
    return rootEntity;
}

void AssimpModelLoader::processNode(aiNode* node, const aiScene* scene, 
                                    Qt3DCore::QEntity* parent, const QColor& color)
{
    // 处理节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene, parent, color);
    }
    
    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene, parent, color);
    }
}

Qt3DCore::QEntity* AssimpModelLoader::processMesh(aiMesh* mesh, const aiScene* scene, 
                                                   Qt3DCore::QEntity* parent, const QColor& color)
{
    if (mesh->mNumVertices == 0 || mesh->mNumFaces == 0) {
        return nullptr;
    }
    
    // 创建网格实体
    Qt3DCore::QEntity* meshEntity = new Qt3DCore::QEntity(parent);
    
    // 创建几何体
    Qt3DRender::QGeometry* geometry = new Qt3DRender::QGeometry(meshEntity);
    
    // ===== 顶点位置 =====
    QByteArray positionData;
    positionData.resize(mesh->mNumVertices * 3 * sizeof(float));
    float* posPtr = reinterpret_cast<float*>(positionData.data());
    
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        aiVector3D& v = mesh->mVertices[i];
        *posPtr++ = v.x;
        *posPtr++ = v.y;
        *posPtr++ = v.z;
        
        // 更新包围盒
        m_minPoint.setX(qMin(m_minPoint.x(), v.x * m_scale.x()));
        m_minPoint.setY(qMin(m_minPoint.y(), v.y * m_scale.y()));
        m_minPoint.setZ(qMin(m_minPoint.z(), v.z * m_scale.z()));
        m_maxPoint.setX(qMax(m_maxPoint.x(), v.x * m_scale.x()));
        m_maxPoint.setY(qMax(m_maxPoint.y(), v.y * m_scale.y()));
        m_maxPoint.setZ(qMax(m_maxPoint.z(), v.z * m_scale.z()));
    }
    
    Qt3DRender::QBuffer* positionBuffer = new Qt3DRender::QBuffer(geometry);
    positionBuffer->setData(positionData);
    
    Qt3DRender::QAttribute* positionAttribute = new Qt3DRender::QAttribute(geometry);
    positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    positionAttribute->setVertexSize(3);
    positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    positionAttribute->setBuffer(positionBuffer);
    positionAttribute->setByteStride(3 * sizeof(float));
    positionAttribute->setCount(mesh->mNumVertices);
    geometry->addAttribute(positionAttribute);
    
    // ===== 法线 =====
    if (mesh->HasNormals()) {
        QByteArray normalData;
        normalData.resize(mesh->mNumVertices * 3 * sizeof(float));
        float* normPtr = reinterpret_cast<float*>(normalData.data());
        
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D& n = mesh->mNormals[i];
            *normPtr++ = n.x;
            *normPtr++ = n.y;
            *normPtr++ = n.z;
        }
        
        Qt3DRender::QBuffer* normalBuffer = new Qt3DRender::QBuffer(geometry);
        normalBuffer->setData(normalData);
        
        Qt3DRender::QAttribute* normalAttribute = new Qt3DRender::QAttribute(geometry);
        normalAttribute->setName(Qt3DRender::QAttribute::defaultNormalAttributeName());
        normalAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
        normalAttribute->setVertexSize(3);
        normalAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
        normalAttribute->setBuffer(normalBuffer);
        normalAttribute->setByteStride(3 * sizeof(float));
        normalAttribute->setCount(mesh->mNumVertices);
        geometry->addAttribute(normalAttribute);
    }
    
    // ===== 索引 =====
    QByteArray indexData;
    unsigned int indexCount = 0;
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        indexCount += mesh->mFaces[i].mNumIndices;
    }
    
    indexData.resize(indexCount * sizeof(unsigned int));
    unsigned int* indexPtr = reinterpret_cast<unsigned int*>(indexData.data());
    
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            *indexPtr++ = face.mIndices[j];
        }
    }
    
    Qt3DRender::QBuffer* indexBuffer = new Qt3DRender::QBuffer(geometry);
    indexBuffer->setData(indexData);
    
    Qt3DRender::QAttribute* indexAttribute = new Qt3DRender::QAttribute(geometry);
    indexAttribute->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
    indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
    indexAttribute->setBuffer(indexBuffer);
    indexAttribute->setCount(indexCount);
    geometry->addAttribute(indexAttribute);
    
    // ===== 几何渲染器 =====
    Qt3DRender::QGeometryRenderer* geometryRenderer = new Qt3DRender::QGeometryRenderer(meshEntity);
    geometryRenderer->setGeometry(geometry);
    geometryRenderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
    meshEntity->addComponent(geometryRenderer);
    
    // ===== 材质 =====
    Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial(meshEntity);
    
    // 尝试从Assimp场景获取材质颜色
    QColor diffuseColor = color;
    if (mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];
        aiColor3D aiDiffuse;
        if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, aiDiffuse) == AI_SUCCESS) {
            diffuseColor = QColor::fromRgbF(aiDiffuse.r, aiDiffuse.g, aiDiffuse.b);
        }
    }
    
    material->setDiffuse(diffuseColor);
    material->setAmbient(diffuseColor.darker(150));
    material->setSpecular(QColor(255, 255, 255));
    material->setShininess(50.0f);
    meshEntity->addComponent(material);
    
    return meshEntity;
}

void AssimpModelLoader::getBoundingBox(QVector3D& minPoint, QVector3D& maxPoint) const
{
    minPoint = m_minPoint;
    maxPoint = m_maxPoint;
}

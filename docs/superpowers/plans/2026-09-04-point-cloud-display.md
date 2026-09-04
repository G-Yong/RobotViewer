# 点云加载与显示 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 在加载 URDF 机器人后，允许用户在场景中加载并显示一个带 RGB 着色的点云（`.ply`），点云坐标系建立在所选 link 上并与机械臂对齐、随该 link 实时跟随。

**Architecture:** 新增 `PointCloudEntity`（QEntity），用 `QGeometry+QBuffer+QAttribute` 建 `Points` 图元 + 自定义 RGB shader；将其作为所选 `LinkEntity` 的子节点，从而自动对齐并跟随。`RobotScene` 管理点云生命周期，`RobotBridge` 暴露给 QML，`ViewOptionsPanel` 提供 UI。

**Tech Stack:** Qt 5.15.2 / Qt3D / QML / MSVC2019 x64 / qmake。

## Global Constraints

- 仅支持 **ASCII** PLY（`format ascii 1.0`）；非 ASCII 返回错误。目标文件：`C:\Users\Administrator\Desktop\dualRobot\motion\tj7\build\Desktop_Qt_5_15_2_MSVC2019_64bit_Profile\release\reachable_pc_projY.ply`（`element vertex 89501`，属性 `float x/y/z` + `uchar red/green/blue`）。
- 仅支持同时一个点云；加载新点云前先移除旧的；加载新 URDF 时清除点云。
- 点云实体挂在 `LinkEntity` 下作为子节点，**实时跟随**该 link。
- 无颜色属性时用 `QColor(200,200,200)` 回退。
- 编译验证统一用 Qt Creator MCP `mcp_qt_creator_mc_build`。源码文件路径前缀：`f:\workData\work\2026\robotViewer\repo\RobotViewer\src\`。
- 提交信息中文注释使用 UTF-8 编码（项目已 `QMAKE_CXXFLAGS += /utf-8`、`#pragma execution_character_set("utf-8")`）。

---

### Task 1: 新增 `PointCloudEntity`（几何 + PLY 解析 + RGB shader）

**Files:**
- Create: `src/pointcloudentity.h`
- Create: `src/pointcloudentity.cpp`
- Modify: `src/RobotViewer.pro`

**Interfaces:**
- Consumes: 无（独立自包含）。
- Produces:
  - `class PointCloudEntity : public Qt3DCore::QEntity`
  - `bool loadFromPLY(const QString& filePath, const QColor& fallbackColor = QColor(200,200,200))`
  - `void setPointSize(float size)`
  - `void setVisible(bool visible)`
  - `int vertexCount() const`

- [ ] **Step 1: 创建头文件 `src/pointcloudentity.h`**

```cpp
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
```

- [ ] **Step 2: 创建实现文件 `src/pointcloudentity.cpp`**

```cpp
#include "pointcloudentity.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <Qt3DRender/QGraphicsApiFilter>
#include <QDebug>

#pragma execution_character_set("utf-8")

namespace {

double toDouble(const QString& s, bool& ok)
{
    return s.toDouble(&ok);
}

float toColorComponent(const QString& s, const QString& type)
{
    if (type == "uchar" || type == "uint8" || type == "uint8_t")
        return s.toInt() / 255.0f;
    if (type == "ushort" || type == "uint16" || type == "uint16_t")
        return s.toInt() / 65535.0f;
    if (type == "float")
        return s.toFloat();
    if (type == "double")
        return static_cast<float>(s.toDouble());
    return s.toFloat();
}

} // namespace

PointCloudEntity::PointCloudEntity(Qt3DCore::QEntity* parent)
    : Qt3DCore::QEntity(parent)
{
    m_geometry = new Qt3DRender::QGeometry(this);

    m_vertexBuffer = new Qt3DRender::QBuffer(m_geometry);
    m_colorBuffer = new Qt3DRender::QBuffer(m_geometry);

    m_positionAttribute = new Qt3DRender::QAttribute(m_geometry);
    m_positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
    m_positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    m_positionAttribute->setVertexSize(3);
    m_positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_positionAttribute->setBuffer(m_vertexBuffer);
    m_positionAttribute->setByteStride(3 * sizeof(float));
    m_positionAttribute->setCount(0);
    m_geometry->addAttribute(m_positionAttribute);

    m_colorAttribute = new Qt3DRender::QAttribute(m_geometry);
    m_colorAttribute->setName(QStringLiteral("color"));
    m_colorAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
    m_colorAttribute->setVertexSize(3);
    m_colorAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
    m_colorAttribute->setBuffer(m_colorBuffer);
    m_colorAttribute->setByteStride(3 * sizeof(float));
    m_colorAttribute->setCount(0);
    m_geometry->addAttribute(m_colorAttribute);

    m_renderer = new Qt3DRender::QGeometryRenderer(this);
    m_renderer->setGeometry(m_geometry);
    m_renderer->setPrimitiveType(Qt3DRender::QGeometryRenderer::Points);
    m_renderer->setEnabled(false); // 未加载数据时禁用
    addComponent(m_renderer);

    createShaderMaterial();
}

PointCloudEntity::~PointCloudEntity()
{
}

void PointCloudEntity::createShaderMaterial()
{
    m_material = new Qt3DRender::QMaterial(this);

    Qt3DRender::QEffect* effect = new Qt3DRender::QEffect(m_material);
    m_material->setEffect(effect);

    Qt3DRender::QTechnique* technique = new Qt3DRender::QTechnique(effect);
    Qt3DRender::QGraphicsApiFilter* api = technique->graphicsApiFilter();
    api->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    api->setMajorVersion(4);
    api->setMinorVersion(3);
    api->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
    effect->addTechnique(technique);

    Qt3DRender::QRenderPass* pass = new Qt3DRender::QRenderPass(technique);
    technique->addRenderPass(pass);

    // 启用 GL_PROGRAM_POINT_SIZE，使 shader 中的 gl_PointSize 生效（Programmable 模式）
    Qt3DRender::QPointSize* pointSize = new Qt3DRender::QPointSize();
    pointSize->setSizeMode(Qt3DRender::QPointSize::Programmable);
    pointSize->setValue(m_pointSize);
    pass->addRenderState(pointSize);

    Qt3DRender::QShaderProgram* shader = new Qt3DRender::QShaderProgram(pass);
    shader->setVertexShaderCode(QByteArrayLiteral(
        "#version 430 core\n"
        "in vec3 position;\n"
        "in vec3 color;\n"
        "uniform mat4 modelViewProjection;\n"
        "uniform float pointSize;\n"
        "out vec3 vColor;\n"
        "void main() {\n"
        "    vColor = color;\n"
        "    gl_Position = modelViewProjection * vec4(position, 1.0);\n"
        "    gl_PointSize = pointSize;\n"
        "}\n"));
    shader->setFragmentShaderCode(QByteArrayLiteral(
        "#version 430 core\n"
        "in vec3 vColor;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = vec4(vColor, 1.0);\n"
        "}\n"));
    pass->setShaderProgram(shader);

    m_pointSizeParameter = new Qt3DRender::QParameter(QStringLiteral("pointSize"), m_pointSize);
    m_material->addParameter(m_pointSizeParameter);

    addComponent(m_material);
}

bool PointCloudEntity::parseHeader(QTextStream& in, PlyHeader& header) const
{
    bool inVertex = false;
    bool sawEnd = false;
    const QRegularExpression re("\\s+");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QStringList parts = line.split(re, Qt::SkipEmptyParts);
        if (parts.isEmpty())
            continue;

        const QString keyword = parts[0];
        if (keyword == QStringLiteral("ply"))
            continue;
        if (keyword == QStringLiteral("comment"))
            continue;
        if (keyword == QStringLiteral("format")) {
            header.format = parts.value(1);
        } else if (keyword == QStringLiteral("element")) {
            const QString type = parts.value(1);
            const int count = parts.value(2).toInt();
            if (type == QStringLiteral("vertex")) {
                header.vertexCount = count;
                inVertex = true;
            } else if (inVertex) {
                inVertex = false;
            }
        } else if (keyword == QStringLiteral("property")) {
            if (inVertex) {
                PlyHeader::Property p;
                p.type = parts.value(1);
                p.name = parts.value(2);
                header.properties.append(p);
            }
        } else if (keyword == QStringLiteral("end_header")) {
            sawEnd = true;
            break;
        }
    }
    return sawEnd;
}

bool PointCloudEntity::loadAscii(QTextStream& in, const PlyHeader& header, const QColor& fallbackColor)
{
    int xi = -1, yi = -1, zi = -1, ri = -1, gi = -1, bi = -1;
    for (int i = 0; i < header.properties.size(); ++i) {
        const QString& name = header.properties[i].name;
        if (name == QStringLiteral("x")) xi = i;
        else if (name == QStringLiteral("y")) yi = i;
        else if (name == QStringLiteral("z")) zi = i;
        else if (name == QStringLiteral("red") || name == QStringLiteral("r")) ri = i;
        else if (name == QStringLiteral("green") || name == QStringLiteral("g")) gi = i;
        else if (name == QStringLiteral("blue") || name == QStringLiteral("b")) bi = i;
    }

    if (xi < 0 || yi < 0 || zi < 0) {
        qWarning() << "PointCloud: missing x/y/z properties";
        return false;
    }

    const bool hasColor = (ri >= 0 && gi >= 0 && bi >= 0);
    const float fr = fallbackColor.redF();
    const float fg = fallbackColor.greenF();
    const float fb = fallbackColor.blueF();

    const int n = header.vertexCount;
    if (n <= 0)
        return false;

    m_positionData.resize(n * 3 * sizeof(float));
    m_colorData.resize(n * 3 * sizeof(float));
    float* pos = reinterpret_cast<float*>(m_positionData.data());
    float* col = reinterpret_cast<float*>(m_colorData.data());

    const QRegularExpression re("\\s+");
    int count = 0;
    while (!in.atEnd() && count < n) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;
        QStringList f = line.split(re, Qt::SkipEmptyParts);
        if (f.size() <= zi)
            continue;

        bool ok = false;
        const double x = toDouble(f[xi], ok); if (!ok) return false;
        const double y = toDouble(f[yi], ok); if (!ok) return false;
        const double z = toDouble(f[zi], ok); if (!ok) return false;

        pos[count * 3 + 0] = static_cast<float>(x);
        pos[count * 3 + 1] = static_cast<float>(y);
        pos[count * 3 + 2] = static_cast<float>(z);

        if (hasColor) {
            col[count * 3 + 0] = toColorComponent(f[ri], header.properties[ri].type);
            col[count * 3 + 1] = toColorComponent(f[gi], header.properties[gi].type);
            col[count * 3 + 2] = toColorComponent(f[bi], header.properties[bi].type);
        } else {
            col[count * 3 + 0] = fr;
            col[count * 3 + 1] = fg;
            col[count * 3 + 2] = fb;
        }
        ++count;
    }

    m_vertexCount = count;
    return count > 0;
}

bool PointCloudEntity::loadFromPLY(const QString& filePath, const QColor& fallbackColor)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "PointCloud: cannot open" << filePath;
        m_renderer->setEnabled(false);
        return false;
    }

    QTextStream in(&file);

    PlyHeader header;
    bool ok = parseHeader(in, header);
    if (!ok) {
        qWarning() << "PointCloud: invalid PLY header";
        m_renderer->setEnabled(false);
        return false;
    }

    if (!header.format.startsWith(QStringLiteral("ascii"))) {
        qWarning() << "PointCloud: only ASCII PLY supported, got" << header.format;
        m_renderer->setEnabled(false);
        return false;
    }

    ok = loadAscii(in, header, fallbackColor);
    if (!ok || m_vertexCount <= 0) {
        qWarning() << "PointCloud: no vertices loaded";
        m_renderer->setEnabled(false);
        return false;
    }

    m_vertexBuffer->setData(m_positionData);
    m_colorBuffer->setData(m_colorData);
    m_positionAttribute->setCount(m_vertexCount);
    m_colorAttribute->setCount(m_vertexCount);
    m_renderer->setEnabled(true);
    return true;
}

void PointCloudEntity::setPointSize(float size)
{
    m_pointSize = size;
    if (m_pointSizeParameter)
        m_pointSizeParameter->setValue(size);
}

void PointCloudEntity::setVisible(bool visible)
{
    setEnabled(visible);
}
```

- [ ] **Step 3: 把 `PointCloudEntity` 加入 `src/RobotViewer.pro`**

在 `SOURCES` 末尾追加 `pointcloudentity.cpp`，在 `HEADERS` 末尾追加 `pointcloudentity.h`：

```pro
SOURCES += \
    urdfparser.cpp \
    assimpmodelloader.cpp \
    robotentity.cpp \
    robotscene.cpp \
    trajectoryentity.cpp \
    settingsmanager.cpp \
    viewoptions.cpp \
    opcuabindingmodel.cpp \
    endeffectorconfigmodel.cpp \
    orbitcameracontroller.cpp \
    pointcloudentity.cpp

HEADERS += \
    commontypes.h \
    urdfparser.h \
    assimpmodelloader.h \
    robotentity.h \
    robotscene.h \
    trajectoryentity.h \
    settingsmanager.h \
    viewoptions.h \
    opcuabindingmodel.h \
    endeffectorconfigmodel.h \
    orbitcameracontroller.h \
    pointcloudentity.h
```

- [ ] **Step 4: 编译验证**

Run: `mcp_qt_creator_mc_build`
Expected: 编译通过，无报错。若有错误，先修复（重点检查 `QGraphicsApiFilter`、`QShaderProgram` 接口在 Qt 5.15.2 的正确性）。

- [ ] **Step 5: 提交**

```bash
git add src/pointcloudentity.h src/pointcloudentity.cpp src/RobotViewer.pro
git commit -m "feat: add PointCloudEntity with ASCII PLY + RGB shading"
```

---

### Task 2: `RobotScene` 增加点云管理（加载/清除/显隐/换 link）

**Files:**
- Modify: `src/robotscene.h`
- Modify: `src/robotscene.cpp`

**Interfaces:**
- Consumes: `PointCloudEntity`（Task 1）；`RobotEntity::getLinkEntity(const QString&)` → `LinkEntity*`；`RobotEntity::robotLoaded` 信号。
- Produces（供 Task 3 使用）:
  - `bool addPointCloud(const QString& plyFile, const QString& linkName)`
  - `void removePointCloud()`
  - `void setPointCloudVisible(bool visible)`
  - `bool isPointCloudVisible() const`
  - `bool pointCloudLoaded() const`
  - `QString pointCloudLink() const`
  - `void setPointCloudLink(const QString& linkName)`
  - `void setPointCloudPointSize(float size)` / `float pointCloudPointSize() const`
  - signals: `pointCloudLoadedChanged()`、`pointCloudVisibleChanged()`、`pointCloudLinkChanged()`

- [ ] **Step 1: 修改 `src/robotscene.h`**

在文件顶部前向声明处加 `class PointCloudEntity;`；在 `public` 区（`setTrajectoryVisible` 附近）加入方法；在 `signals` 区加信号；在 `private` 区加成员。

```cpp
// 文件顶部，第 9 行附近
class RobotEntity;
class TrajectoryEntity;
class PointCloudEntity;
```

```cpp
    /**
     * @brief 加载并显示点云（挂到指定 link 下）
     */
    bool addPointCloud(const QString& plyFile, const QString& linkName);

    /**
     * @brief 移除点云
     */
    void removePointCloud();

    /**
     * @brief 设置点云可见性
     */
    void setPointCloudVisible(bool visible);
    bool isPointCloudVisible() const { return m_pointCloudVisible; }

    /**
     * @brief 点云是否已加载
     */
    bool pointCloudLoaded() const { return m_pointCloud != nullptr; }

    /**
     * @brief 点云坐标系所在 link
     */
    QString pointCloudLink() const { return m_pointCloudLink; }

    /**
     * @brief 重新设置点云坐标系 link（会重新挂到新 link 下）
     */
    void setPointCloudLink(const QString& linkName);

    /**
     * @brief 设置点云点大小
     */
    void setPointCloudPointSize(float size);
    float pointCloudPointSize() const { return m_pointCloudPointSize; }
```

```cpp
signals:
    void robotLoaded();
    void loadError(const QString& error);
    void fitCameraRequested(const QVector3D& center, const QVector3D& position);
    void pointCloudLoadedChanged();
    void pointCloudVisibleChanged();
    void pointCloudLinkChanged();
```

```cpp
private:
    Qt3DCore::QEntity* m_gridEntity = nullptr;
    Qt3DCore::QEntity* m_axesEntity = nullptr;

    PointCloudEntity* m_pointCloud = nullptr;
    QString m_pointCloudLink;
    bool m_pointCloudVisible = true;
    float m_pointCloudPointSize = 2.0f;
```

- [ ] **Step 2: 修改 `src/robotscene.cpp`**

引入头文件并实现方法。在文件顶部：

```cpp
#include "robotscene.h"
#include "robotentity.h"
#include "trajectoryentity.h"
#include "pointcloudentity.h"
```

在 `loadRobot` 开头调用 `removePointCloud()`：

```cpp
bool RobotScene::loadRobot(const QString& urdfFile)
{
    if (!m_robotEntity) return false;

    // 加载新机器人时清除旧点云
    removePointCloud();

    bool success = m_robotEntity->loadFromURDF(urdfFile);
```

在文件末尾（`fitCameraToRobot` 之后）追加：

```cpp
bool RobotScene::addPointCloud(const QString& plyFile, const QString& linkName)
{
    if (!m_robotEntity) {
        emit loadError(tr("请先加载机器人再添加点云。"));
        return false;
    }

    LinkEntity* link = m_robotEntity->getLinkEntity(linkName);
    if (!link) {
        emit loadError(tr("点云坐标系 Link 不存在: %1").arg(linkName));
        return false;
    }

    removePointCloud();

    PointCloudEntity* pc = new PointCloudEntity(link);
    if (!pc->loadFromPLY(plyFile)) {
        pc->deleteLater();
        emit loadError(tr("点云加载失败: %1").arg(plyFile));
        return false;
    }

    m_pointCloud = pc;
    m_pointCloudLink = linkName;
    m_pointCloudVisible = true;
    pc->setVisible(true);
    pc->setPointSize(m_pointCloudPointSize);

    emit pointCloudLoadedChanged();
    emit pointCloudLinkChanged();
    emit pointCloudVisibleChanged();
    qDebug() << "PointCloud loaded:" << plyFile << "points:" << pc->vertexCount()
             << "link:" << linkName;
    return true;
}

void RobotScene::removePointCloud()
{
    if (!m_pointCloud)
        return;

    m_pointCloud->deleteLater();
    m_pointCloud = nullptr;
    m_pointCloudLink.clear();
    emit pointCloudLoadedChanged();
}

void RobotScene::setPointCloudVisible(bool visible)
{
    m_pointCloudVisible = visible;
    if (m_pointCloud)
        m_pointCloud->setVisible(visible);
    emit pointCloudVisibleChanged();
}

void RobotScene::setPointCloudLink(const QString& linkName)
{
    // 始终记录所选 link（即使尚未加载点云，供 QML combo 记住选择）
    m_pointCloudLink = linkName;

    if (!m_pointCloud)
        return;

    LinkEntity* link = m_robotEntity ? m_robotEntity->getLinkEntity(linkName) : nullptr;
    if (link) {
        // 重新挂到新 link 下（Qt3D QNode 会处理场景图重挂）
        m_pointCloud->setParent(link);
    } else {
        emit loadError(tr("点云坐标系 Link 不存在: %1").arg(linkName));
    }

    emit pointCloudLinkChanged();
}

void RobotScene::setPointCloudPointSize(float size)
{
    m_pointCloudPointSize = size;
    if (m_pointCloud)
        m_pointCloud->setPointSize(size);
}
```

- [ ] **Step 3: 编译验证**

Run: `mcp_qt_creator_mc_build`
Expected: 编译通过。

- [ ] **Step 4: 提交**

```bash
git add src/robotscene.h src/robotscene.cpp
git commit -m "feat: RobotScene manages point cloud lifecycle"
```

---

### Task 3: `RobotBridge` 暴露点云给 QML

**Files:**
- Modify: `src/robotbridge.h`
- Modify: `src/robotbridge.cpp`

**Interfaces:**
- Consumes: `RobotScene`（Task 2）点云方法/信号。
- Produces（供 Task 4 QML 使用）:
  - Q_PROPERTY `bool pointCloudVisible`、`bool pointCloudLoaded`、`QString pointCloudLink`、`float pointCloudPointSize`
  - `Q_INVOKABLE void openPointCloud()`
  - `Q_INVOKABLE void removePointCloud()`

- [ ] **Step 1: 修改 `src/robotbridge.h`**

在 `Q_PROPERTY` 区（`showTrajectory` 属性后）加入：

```cpp
    // 点云属性
    Q_PROPERTY(bool pointCloudVisible READ pointCloudVisible WRITE setPointCloudVisible NOTIFY pointCloudVisibleChanged)
    Q_PROPERTY(bool pointCloudLoaded READ pointCloudLoaded NOTIFY pointCloudLoadedChanged)
    Q_PROPERTY(QString pointCloudLink READ pointCloudLink WRITE setPointCloudLink NOTIFY pointCloudLinkChanged)
    Q_PROPERTY(float pointCloudPointSize READ pointCloudPointSize WRITE setPointCloudPointSize NOTIFY pointCloudPointSizeChanged)
```

在 `public` 的 getter 区（`showTrajectory()` 附近）加入：

```cpp
    // 点云 Getters
    bool pointCloudVisible() const;
    bool pointCloudLoaded() const;
    QString pointCloudLink() const;
    float pointCloudPointSize() const;
```

在 setter 区（`setShowTrajectory` 附近）加入：

```cpp
    // 点云 Setters
    void setPointCloudVisible(bool visible);
    void setPointCloudLink(const QString& linkName);
    void setPointCloudPointSize(float size);
```

在 `public slots` 区（`fitCamera()` 附近）加入：

```cpp
    // 点云操作
    Q_INVOKABLE void openPointCloud();
    Q_INVOKABLE void removePointCloud();
```

在 `signals` 区加入：

```cpp
    // 点云信号
    void pointCloudVisibleChanged();
    void pointCloudLoadedChanged();
    void pointCloudLinkChanged();
    void pointCloudPointSizeChanged();
```

在 `private` 成员区加入：

```cpp
    QString m_lastPlyPath;
```

- [ ] **Step 2: 修改 `src/robotbridge.cpp`**

在 `setupConnections()` 里连接点云信号转发：

```cpp
void RobotBridge::setupConnections()
{
    // 场景信号连接
    connect(m_scene, &RobotScene::robotLoaded, this, &RobotBridge::onRobotLoaded);
    connect(m_scene, &RobotScene::loadError, this, &RobotBridge::onLoadError);
    // 信号直连：RobotScene::fitCameraRequested -> RobotBridge::fitCameraRequested
    connect(m_scene, &RobotScene::fitCameraRequested, this, &RobotBridge::fitCameraRequested);
    connect(m_scene, &RobotScene::pointCloudLoadedChanged, this, &RobotBridge::pointCloudLoadedChanged);
    connect(m_scene, &RobotScene::pointCloudVisibleChanged, this, &RobotBridge::pointCloudVisibleChanged);
    connect(m_scene, &RobotScene::pointCloudLinkChanged, this, &RobotBridge::pointCloudLinkChanged);
}
```

实现 getters/setters 与 `openPointCloud`/`removePointCloud`。放在 `fitCamera()` 之后：

```cpp
// 点云 Getters/Setters
bool RobotBridge::pointCloudVisible() const
{
    return m_scene ? m_scene->isPointCloudVisible() : false;
}

bool RobotBridge::pointCloudLoaded() const
{
    return m_scene ? m_scene->pointCloudLoaded() : false;
}

QString RobotBridge::pointCloudLink() const
{
    return m_scene ? m_scene->pointCloudLink() : QString();
}

void RobotBridge::setPointCloudVisible(bool visible)
{
    if (m_scene)
        m_scene->setPointCloudVisible(visible);
}

void RobotBridge::setPointCloudLink(const QString& linkName)
{
    if (m_scene)
        m_scene->setPointCloudLink(linkName);
}

float RobotBridge::pointCloudPointSize() const
{
    return m_scene ? m_scene->pointCloudPointSize() : 2.0f;
}

void RobotBridge::setPointCloudPointSize(float size)
{
    if (m_scene)
        m_scene->setPointCloudPointSize(size);
    emit pointCloudPointSizeChanged();
}

void RobotBridge::openPointCloud()
{
    QString filename = QFileDialog::getOpenFileName(
        nullptr,
        tr("打开点云文件"),
        m_lastPlyPath.isEmpty() ? QString() : QFileInfo(m_lastPlyPath).absolutePath(),
        tr("点云文件 (*.ply);;所有文件 (*.*)")
    );
    if (filename.isEmpty())
        return;

    m_lastPlyPath = filename;

    // 若尚未选择 link，使用第一个 link 作为默认坐标系
    QString link = m_scene ? m_scene->pointCloudLink() : QString();
    if (link.isEmpty()) {
        const QStringList links = linkNames();
        if (!links.isEmpty())
            link = links.first();
    }

    if (m_scene && m_scene->addPointCloud(filename, link)) {
        m_statusMessage = tr("已加载点云: %1").arg(filename);
        emit statusMessageChanged();
    }
}

void RobotBridge::removePointCloud()
{
    if (m_scene)
        m_scene->removePointCloud();
    m_statusMessage = tr("已清除点云");
    emit statusMessageChanged();
}
```

- [ ] **Step 3: 编译验证**

Run: `mcp_qt_creator_mc_build`
Expected: 编译通过。

- [ ] **Step 4: 提交**

```bash
git add src/robotbridge.h src/robotbridge.cpp
git commit -m "feat: expose point cloud to QML via RobotBridge"
```

---

### Task 4: QML「点云」UI 分组

**Files:**
- Modify: `src/qml/panels/ViewOptionsPanel.qml`

**Interfaces:**
- Consumes: `robotBridge.pointCloudVisible`、`pointCloudLoaded`、`pointCloudLink`、`pointCloudPointSize`、`linkNames`、`openPointCloud()`、`removePointCloud()`；现有组件 `GlassComboBox`、`GlassToggle`、`GlassButton`、`GlassSlider`、`SettingsGroup`。

- [ ] **Step 1: 在「相机控制」`SettingsGroup` 之前插入「点云」分组**

在 `ViewOptionsPanel.qml` 中，`// 相机选项` `SettingsGroup` 上方插入：

```qml
        // 点云选项
        SettingsGroup {
            title: qsTr("点云")
            iconText: "🧊"
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 12

                GlassComboBox {
                    Layout.fillWidth: true
                    model: robotBridge ? robotBridge.linkNames : []
                    currentValue: robotBridge ? robotBridge.pointCloudLink : ""
                    placeholder: qsTr("选择点云坐标系Link")
                    onValueChanged: function(value) {
                        if (robotBridge) robotBridge.pointCloudLink = value
                    }
                }

                GlassButton {
                    Layout.fillWidth: true
                    text: qsTr("加载点云…")
                    iconText: "🧊"
                    height: 36
                    onClicked: {
                        if (robotBridge) robotBridge.openPointCloud()
                    }
                }

                GlassToggle {
                    text: qsTr("显示点云")
                    checked: robotBridge ? robotBridge.pointCloudLoaded && robotBridge.pointCloudVisible : false
                    onToggled: function(checked) {
                        if (robotBridge) robotBridge.pointCloudVisible = checked
                    }
                }

                GlassSlider {
                    Layout.fillWidth: true
                    label: qsTr("点大小")
                    from: 1
                    to: 10
                    value: robotBridge ? robotBridge.pointCloudPointSize : 2
                    suffix: " px"
                    decimals: 0
                    onValueModified: function(newValue) {
                        if (robotBridge) robotBridge.pointCloudPointSize = newValue
                    }
                }

                GlassButton {
                    Layout.fillWidth: true
                    text: qsTr("清除点云")
                    iconText: "🗑"
                    height: 36
                    onClicked: {
                        if (robotBridge) robotBridge.removePointCloud()
                    }
                }
            }
        }
```

- [ ] **Step 2: 编译验证**

Run: `mcp_qt_creator_mc_build`
Expected: 编译通过；QML 若在运行时警告可忽略页面跑起来后检查控制台。

- [ ] **Step 3: 运行验证（关键）**

运行应用，加载 URDF 机器人后：
1. 设置面板出现「点云」分组。
2. 选择 link（默认第一个/base），点「加载点云…」，选择 `reachable_pc_projY.ply`。
3. 观察 3D 场景中带灰白 RGB 的点云出现，并与机械臂底座对齐（点云数据在 base 附近分布）。
4. 拖动一个会动的 link 对应的关节，点云跟随（选非 base 的 link 时更明显）。
5. 切换「显示点云」开关、点大小滑块、清除点云，均即时生效。

Expected: 全部符合预期。若 shader 的 `modelViewProjection` 未生效导致点云不显示，调试该 uniform（可临时改用 `#version 330 core` 或核对 Qt3D 注入参数名）。

- [ ] **Step 4: 提交**

```bash
git add src/qml/panels/ViewOptionsPanel.qml
git commit -m "feat: add point cloud UI group in view options panel"
```

---

## 自检

- **Spec 覆盖**：
  - ASCII PLY 解析 + RGB 着色 → Task 1。
  - 坐标系建立在所选 link 上（挂到 LinkEntity）→ Task 2 `addPointCloud`。
  - 实时跟随 link → 挂为 LinkEntity 子节点 + `setPointCloudLink` 重挂。
  - 仅一个点云 → `addPointCloud` 先 `removePointCloud`；`loadRobot` 清除。
  - 无颜色回退 → `loadAscii` 的 `fallbackColor`。
  - QML UI → Task 4。
- **Placeholder scan**：无 TBD/TODO；每个代码步骤包含完整代码。
- **类型一致性**：
  - `PointCloudEntity::loadFromPLY/setPointSize/setVisible` 在 Task 1 定义、Task 2 使用，签名一致。
  - `RobotScene` 的方法/信号在 Task 2 产生、Task 3 消费，名称一致。
  - `RobotBridge` 的 `pointCloudVisible/pointCloudLoaded/pointCloudLink/pointCloudPointSize/openPointCloud/removePointCloud` 在 Task 3 产生、Task 4 消费，名称一致。

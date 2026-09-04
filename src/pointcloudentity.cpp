#include "pointcloudentity.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QFilterKey>
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
    m_colorAttribute->setName(Qt3DRender::QAttribute::defaultColorAttributeName());
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
    // 与程序统一：OpenGL 4.3 Core（应用 QSurfaceFormat 相同）
    Qt3DRender::QGraphicsApiFilter* api = technique->graphicsApiFilter();
    api->setApi(Qt3DRender::QGraphicsApiFilter::OpenGL);
    api->setMajorVersion(4);
    api->setMinorVersion(3);
    api->setProfile(Qt3DRender::QGraphicsApiFilter::CoreProfile);
    // ForwardRenderer 帧图按 renderingStyle=forward 筛选 technique，缺失则不渲染
    Qt3DRender::QFilterKey* filterKey = new Qt3DRender::QFilterKey(technique);
    filterKey->setName(QStringLiteral("renderingStyle"));
    filterKey->setValue(QStringLiteral("forward"));
    technique->addFilterKey(filterKey);
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
        "in vec3 vertexPosition;\n"
        "in vec3 vertexColor;\n"
        "uniform mat4 mvp;\n"
        "uniform float pointSize;\n"
        "out vec3 vColor;\n"
        "void main() {\n"
        "    vColor = vertexColor;\n"
        "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
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

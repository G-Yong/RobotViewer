#include "urdfparser.h"
#include <QFile>
#include <QDomDocument>
#include <QFileInfo>
#include <QDir>
#include <QtMath>
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpression>

// ==================== Origin ====================

QMatrix4x4 Origin::toMatrix() const
{
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    
    // 先平移
    matrix.translate(xyz[0], xyz[1], xyz[2]);
    
    // // 再旋转 (ZYX顺序，即先绕Z轴旋转yaw，再绕Y轴旋转pitch，最后绕X轴旋转roll)
    matrix.rotate(qRadiansToDegrees(rpy[2]), 0, 0, 1); // yaw
    matrix.rotate(qRadiansToDegrees(rpy[1]), 0, 1, 0); // pitch
    matrix.rotate(qRadiansToDegrees(rpy[0]), 1, 0, 0); // roll

    // matrix.rotate(qRadiansToDegrees(rpy[0]), 0, 0, 1); // yaw
    // matrix.rotate(qRadiansToDegrees(rpy[1]), 0, 1, 0); // pitch
    // matrix.rotate(qRadiansToDegrees(rpy[2]), 1, 0, 0); // roll
    
    return matrix;
}

QQuaternion Origin::rotation() const
{
    // RPY to Quaternion (ZYX order)
    double cy = qCos(rpy[2] * 0.5);
    double sy = qSin(rpy[2] * 0.5);
    double cp = qCos(rpy[1] * 0.5);
    double sp = qSin(rpy[1] * 0.5);
    double cr = qCos(rpy[0] * 0.5);
    double sr = qSin(rpy[0] * 0.5);

    QQuaternion q;
    q.setScalar(cr * cp * cy + sr * sp * sy);
    q.setX(sr * cp * cy - cr * sp * sy);
    q.setY(cr * sp * cy + sr * cp * sy);
    q.setZ(cr * cp * sy - sr * sp * cy);
    
    return q;
}

// ==================== URDFJoint ====================

QMatrix4x4 URDFJoint::getTransform(double value) const
{
    QMatrix4x4 matrix;
    matrix.setToIdentity();
    
    QVector3D axisVec(axis[0], axis[1], axis[2]);
    axisVec.normalize();
    
    switch (type) {
    case JointType::Revolute:
    case JointType::Continuous:
        // 旋转关节
        matrix.rotate(qRadiansToDegrees(value), axisVec);
        break;
        
    case JointType::Prismatic:
        // 移动关节
        matrix.translate(axisVec * value);
        break;
        
    default:
        break;
    }
    
    return matrix;
}

// ==================== URDFModel ====================

QVector<std::shared_ptr<URDFJoint>> URDFModel::getMovableJoints() const
{
    QVector<std::shared_ptr<URDFJoint>> movableJoints;
    for (auto& joint : joints) {
        if (joint->isMovable()) {
            movableJoints.append(joint);
        }
    }
    return movableJoints;
}

QVector<std::shared_ptr<URDFJoint>> URDFModel::getChildJoints(const QString& linkName) const
{
    QVector<std::shared_ptr<URDFJoint>> childJoints;
    for (auto& joint : joints) {
        if (joint->parentLink == linkName) {
            childJoints.append(joint);
        }
    }
    return childJoints;
}

std::shared_ptr<URDFJoint> URDFModel::getParentJoint(const QString& linkName) const
{
    for (auto& joint : joints) {
        if (joint->childLink == linkName) {
            return joint;
        }
    }
    return nullptr;
}

// ==================== URDFParser ====================

URDFParser::URDFParser()
    : m_model(std::make_shared<URDFModel>())
{
}

URDFParser::~URDFParser()
{
}

bool URDFParser::loadFromFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_errorMessage = QString("Cannot open file: %1").arg(filename);
        return false;
    }
    
    QFileInfo fileInfo(filename);
    m_basePath = fileInfo.absolutePath();
    
    QString content = QString::fromUtf8(file.readAll());
    file.close();
    
    return loadFromString(content, m_basePath);
}

bool URDFParser::loadFromString(const QString& content, const QString& basePath)
{
    m_basePath = basePath;
    m_model = std::make_shared<URDFModel>();
    m_materials.clear();
    
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    if (!doc.setContent(content, &errorMsg, &errorLine, &errorColumn)) {
        m_errorMessage = QString("XML parse error at line %1, column %2: %3")
                             .arg(errorLine).arg(errorColumn).arg(errorMsg);
        return false;
    }
    
    QDomElement root = doc.documentElement();
    if (root.tagName() != "robot") {
        m_errorMessage = "Root element is not 'robot'";
        return false;
    }
    
    return parseRobot(root);
}

bool URDFParser::parseRobot(const QDomElement& element)
{
    m_model->name = element.attribute("name");
    
    // 首先解析全局材质定义
    QDomNodeList materialNodes = element.childNodes();
    for (int i = 0; i < materialNodes.count(); ++i) {
        QDomElement child = materialNodes.at(i).toElement();
        if (child.tagName() == "material") {
            Material mat = parseMaterial(child);
            if (!mat.name.isEmpty()) {
                m_materials[mat.name] = mat;
            }
        }
    }
    
    // 解析link和joint
    for (int i = 0; i < materialNodes.count(); ++i) {
        QDomElement child = materialNodes.at(i).toElement();
        if (child.isNull()) continue;
        
        if (child.tagName() == "link") {
            if (!parseLink(child)) {
                return false;
            }
        } else if (child.tagName() == "joint") {
            if (!parseJoint(child)) {
                return false;
            }
        }
    }
    
    // 找到根链接
    findRootLink();
    
    if (m_model->rootLink.isEmpty() && !m_model->links.isEmpty()) {
        // 如果没找到根链接，使用第一个链接
        m_model->rootLink = m_model->links.keys().first();
    }
    
    qDebug() << "URDF loaded:" << m_model->name;
    qDebug() << "  Links:" << m_model->links.count();
    qDebug() << "  Joints:" << m_model->joints.count();
    qDebug() << "  Root link:" << m_model->rootLink;
    
    return true;
}

bool URDFParser::parseLink(const QDomElement& element)
{
    auto link = std::make_shared<URDFLink>();
    link->name = element.attribute("name");
    
    if (link->name.isEmpty()) {
        m_errorMessage = "Link without name";
        return false;
    }
    
    QDomNodeList children = element.childNodes();
    for (int i = 0; i < children.count(); ++i) {
        QDomElement child = children.at(i).toElement();
        if (child.isNull()) continue;
        
        if (child.tagName() == "visual") {
            Visual visual;
            visual.name = child.attribute("name");
            
            QDomElement originElem = child.firstChildElement("origin");
            if (!originElem.isNull()) {
                visual.origin = parseOrigin(originElem);
            }
            
            QDomElement geomElem = child.firstChildElement("geometry");
            if (!geomElem.isNull()) {
                visual.geometry = parseGeometry(geomElem);
            }
            
            QDomElement matElem = child.firstChildElement("material");
            if (!matElem.isNull()) {
                visual.material = parseMaterial(matElem);
                // 如果材质只有名称引用，从全局材质中查找
                if (!visual.material.name.isEmpty() && m_materials.contains(visual.material.name)) {
                    Material& globalMat = m_materials[visual.material.name];
                    // 如果本地没有定义颜色，使用全局材质的颜色
                    if (visual.material.color[0] == 0.8 && visual.material.color[1] == 0.8 && 
                        visual.material.color[2] == 0.8 && visual.material.color[3] == 1.0) {
                        for (int j = 0; j < 4; ++j) {
                            visual.material.color[j] = globalMat.color[j];
                        }
                    }
                }
            }
            
            link->visuals.append(visual);
        }
        else if (child.tagName() == "collision") {
            Collision collision;
            collision.name = child.attribute("name");
            
            QDomElement originElem = child.firstChildElement("origin");
            if (!originElem.isNull()) {
                collision.origin = parseOrigin(originElem);
            }
            
            QDomElement geomElem = child.firstChildElement("geometry");
            if (!geomElem.isNull()) {
                collision.geometry = parseGeometry(geomElem);
            }
            
            link->collisions.append(collision);
        }
        else if (child.tagName() == "inertial") {
            QDomElement originElem = child.firstChildElement("origin");
            if (!originElem.isNull()) {
                link->inertial.origin = parseOrigin(originElem);
            }
            
            QDomElement massElem = child.firstChildElement("mass");
            if (!massElem.isNull()) {
                link->inertial.mass = massElem.attribute("value").toDouble();
            }
            
            QDomElement inertiaElem = child.firstChildElement("inertia");
            if (!inertiaElem.isNull()) {
                link->inertial.ixx = inertiaElem.attribute("ixx").toDouble();
                link->inertial.ixy = inertiaElem.attribute("ixy").toDouble();
                link->inertial.ixz = inertiaElem.attribute("ixz").toDouble();
                link->inertial.iyy = inertiaElem.attribute("iyy").toDouble();
                link->inertial.iyz = inertiaElem.attribute("iyz").toDouble();
                link->inertial.izz = inertiaElem.attribute("izz").toDouble();
            }
        }
    }
    
    m_model->links[link->name] = link;
    return true;
}

bool URDFParser::parseJoint(const QDomElement& element)
{
    auto joint = std::make_shared<URDFJoint>();
    joint->name = element.attribute("name");
    
    QString typeStr = element.attribute("type");
    if (typeStr == "fixed") joint->type = JointType::Fixed;
    else if (typeStr == "revolute") joint->type = JointType::Revolute;
    else if (typeStr == "continuous") joint->type = JointType::Continuous;
    else if (typeStr == "prismatic") joint->type = JointType::Prismatic;
    else if (typeStr == "floating") joint->type = JointType::Floating;
    else if (typeStr == "planar") joint->type = JointType::Planar;
    else joint->type = JointType::Unknown;
    
    QDomNodeList children = element.childNodes();
    for (int i = 0; i < children.count(); ++i) {
        QDomElement child = children.at(i).toElement();
        if (child.isNull()) continue;
        
        if (child.tagName() == "origin") {
            joint->origin = parseOrigin(child);
        }
        else if (child.tagName() == "parent") {
            joint->parentLink = child.attribute("link");
        }
        else if (child.tagName() == "child") {
            joint->childLink = child.attribute("link");
        }
        else if (child.tagName() == "axis") {
            QString xyz = child.attribute("xyz");
            QStringList parts = xyz.split(" ", Qt::SkipEmptyParts);
            if (parts.size() == 3) {
                joint->axis[0] = parts[0].toDouble();
                joint->axis[1] = parts[1].toDouble();
                joint->axis[2] = parts[2].toDouble();
            }
        }
        else if (child.tagName() == "limit") {
            joint->limits.lower = child.attribute("lower").toDouble();
            joint->limits.upper = child.attribute("upper").toDouble();
            joint->limits.effort = child.attribute("effort").toDouble();
            joint->limits.velocity = child.attribute("velocity").toDouble();
        }
        else if (child.tagName() == "dynamics") {
            joint->dynamics.damping = child.attribute("damping").toDouble();
            joint->dynamics.friction = child.attribute("friction").toDouble();
        }
    }
    
    m_model->joints[joint->name] = joint;
    return true;
}

Origin URDFParser::parseOrigin(const QDomElement& element)
{
    Origin origin;
    
    QString xyz = element.attribute("xyz");
    if (!xyz.isEmpty()) {
        // 展开XACRO宏
        xyz = expandXacroMacros(xyz);
        QStringList parts = xyz.split(" ", Qt::SkipEmptyParts);
        if (parts.size() == 3) {
            origin.xyz[0] = parts[0].toDouble();
            origin.xyz[1] = parts[1].toDouble();
            origin.xyz[2] = parts[2].toDouble();
        }
    }
    
    QString rpy = element.attribute("rpy");
    if (!rpy.isEmpty()) {
        // 展开XACRO宏
        rpy = expandXacroMacros(rpy);
        QStringList parts = rpy.split(" ", Qt::SkipEmptyParts);
        if (parts.size() == 3) {
            origin.rpy[0] = parts[0].toDouble();
            origin.rpy[1] = parts[1].toDouble();
            origin.rpy[2] = parts[2].toDouble();
        }
    }
    
    return origin;
}

Geometry URDFParser::parseGeometry(const QDomElement& element)
{
    Geometry geom;
    
    QDomElement child = element.firstChildElement();
    if (child.isNull()) return geom;
    
    QString tag = child.tagName();
    
    if (tag == "mesh") {
        geom.type = GeometryType::Mesh;
        geom.meshFilename = child.attribute("filename");
        
        QString scale = child.attribute("scale");
        if (!scale.isEmpty()) {
            QStringList parts = scale.split(" ", Qt::SkipEmptyParts);
            if (parts.size() == 3) {
                geom.meshScale[0] = parts[0].toDouble();
                geom.meshScale[1] = parts[1].toDouble();
                geom.meshScale[2] = parts[2].toDouble();
            } else if (parts.size() == 1) {
                double s = parts[0].toDouble();
                geom.meshScale[0] = s;
                geom.meshScale[1] = s;
                geom.meshScale[2] = s;
            }
        }
    }
    else if (tag == "box") {
        geom.type = GeometryType::Box;
        QString size = child.attribute("size");
        QStringList parts = size.split(" ", Qt::SkipEmptyParts);
        if (parts.size() == 3) {
            geom.boxSize[0] = parts[0].toDouble();
            geom.boxSize[1] = parts[1].toDouble();
            geom.boxSize[2] = parts[2].toDouble();
        }
    }
    else if (tag == "cylinder") {
        geom.type = GeometryType::Cylinder;
        geom.cylinderRadius = child.attribute("radius").toDouble();
        geom.cylinderLength = child.attribute("length").toDouble();
    }
    else if (tag == "sphere") {
        geom.type = GeometryType::Sphere;
        geom.sphereRadius = child.attribute("radius").toDouble();
    }
    
    return geom;
}

Material URDFParser::parseMaterial(const QDomElement& element)
{
    Material mat;
    mat.name = element.attribute("name");
    
    QDomElement colorElem = element.firstChildElement("color");
    if (!colorElem.isNull()) {
        QString rgba = colorElem.attribute("rgba");
        QStringList parts = rgba.split(" ", Qt::SkipEmptyParts);
        if (parts.size() == 4) {
            mat.color[0] = parts[0].toDouble();
            mat.color[1] = parts[1].toDouble();
            mat.color[2] = parts[2].toDouble();
            mat.color[3] = parts[3].toDouble();
        }
    }
    
    QDomElement textureElem = element.firstChildElement("texture");
    if (!textureElem.isNull()) {
        mat.textureFilename = textureElem.attribute("filename");
    }
    
    return mat;
}

void URDFParser::findRootLink()
{
    // 根链接是没有作为任何关节的子链接的链接
    QSet<QString> childLinks;
    for (auto& joint : m_model->joints) {
        childLinks.insert(joint->childLink);
    }
    
    for (auto& link : m_model->links) {
        if (!childLinks.contains(link->name)) {
            m_model->rootLink = link->name;
            return;
        }
    }
}

QString URDFParser::expandXacroMacros(const QString& value) const
{
    QString result = value;
    
    // 定义PI常量
    const double PI = M_PI;
    
    // 查找并替换所有的 ${...} 表达式
    QRegularExpression regex(R"(\$\{([^}]+)\})");
    QRegularExpressionMatchIterator it = regex.globalMatch(result);
    
    // 从后往前替换，避免位置偏移问题
    QList<QRegularExpressionMatch> matches;
    while (it.hasNext()) {
        matches.prepend(it.next());
    }
    
    for (const auto& match : matches) {
        QString expr = match.captured(1).trimmed();
        double calculatedValue = 0.0;
        bool success = false;
        
        // 简单的数学表达式求值
        // 支持: PI, PI/2, -PI, -PI/2, PI*2, 2*PI 等
        expr.replace("PI", QString::number(PI, 'f', 10));
        
        // 使用 QJSEngine 进行表达式求值（如果Qt版本支持）
        // 或者简单解析常见模式
        QStringList parts;
        
        // 处理除法
        if (expr.contains('/')) {
            parts = expr.split('/');
            if (parts.size() == 2) {
                double num = parts[0].toDouble(&success);
                double den = parts[1].toDouble();
                if (success && den != 0.0) {
                    calculatedValue = num / den;
                }
            }
        }
        // 处理乘法
        else if (expr.contains('*')) {
            parts = expr.split('*');
            if (parts.size() == 2) {
                double a = parts[0].toDouble(&success);
                double b = parts[1].toDouble();
                if (success) {
                    calculatedValue = a * b;
                }
            }
        }
        // 处理加法
        else if (expr.contains('+')) {
            parts = expr.split('+');
            if (parts.size() == 2) {
                double a = parts[0].toDouble(&success);
                double b = parts[1].toDouble();
                if (success) {
                    calculatedValue = a + b;
                }
            }
        }
        // 处理减法（注意负数）
        else if (expr.lastIndexOf('-') > 0) { // 不是开头的负号
            int idx = expr.lastIndexOf('-');
            QString part1 = expr.left(idx);
            QString part2 = expr.mid(idx + 1);
            double a = part1.toDouble(&success);
            double b = part2.toDouble();
            if (success) {
                calculatedValue = a - b;
            }
        }
        // 直接是数值
        else {
            calculatedValue = expr.toDouble(&success);
        }
        
        if (success) {
            result.replace(match.capturedStart(), match.capturedLength(), 
                          QString::number(calculatedValue, 'f', 10));
        }
    }
    
    // qDebug() << "value:" << value << result;

    return result;
}

QString URDFParser::resolveMeshPath(const QString& meshPath) const
{
    QString path = meshPath;

    // 由于这是独立软件，直接去掉模型的相对路径，包路径等前缀，
    // 而是直接取文件名部分, 然后uedf文件所在的目录下的meshes目录中查找
    QFileInfo fi(path);
    path = fi.fileName();
    path = m_basePath + "/meshes/" + path;

    // 处理 file:// 路径
    if (path.startsWith("file://")) {
        path = path.mid(7);
    }
    
    // 如果是相对路径，则相对于URDF文件所在目录
    QFileInfo fileInfo(path);
    if (!fileInfo.isAbsolute()) {
        path = m_basePath + "/" + path;
    }
    
    // 规范化路径
    return QDir::cleanPath(path);
}

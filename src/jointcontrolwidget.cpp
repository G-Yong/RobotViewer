#include "jointcontrolwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QGroupBox>
#include <QtMath>

// ==================== JointSlider ====================

JointSlider::JointSlider(std::shared_ptr<URDFJoint> joint, int index, QWidget* parent)
    : QWidget(parent)
    , m_joint(joint)
    , m_index(index)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(2);
    
    // 名称行
    QHBoxLayout* nameLayout = new QHBoxLayout();
    // 添加序号
    QString indexStr = QString("[%1] ").arg(index, 2, 10, QChar('0'));
    m_nameLabel = new QLabel(indexStr + joint->name, this);
    m_nameLabel->setStyleSheet("font-weight: bold;");
    
    QString typeStr;
    switch (joint->type) {
    case JointType::Revolute: typeStr = "[R]"; break;
    case JointType::Continuous: typeStr = "[C]"; break;
    case JointType::Prismatic: typeStr = "[P]"; break;
    default: typeStr = "[?]"; break;
    }
    QLabel* typeLabel = new QLabel(typeStr, this);
    typeLabel->setStyleSheet("color: gray;");
    
    nameLayout->addWidget(m_nameLabel);
    nameLayout->addWidget(typeLabel);
    nameLayout->addStretch();
    mainLayout->addLayout(nameLayout);
    
    // 滑块行
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    
    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setMinimum(0);
    m_slider->setMaximum(1000);
    m_slider->setValue(500);
    
    m_spinBox = new QDoubleSpinBox(this);
    m_spinBox->setDecimals(4);
    m_spinBox->setFixedWidth(100);
    
    // 设置范围
    double lower = joint->limits.lower;
    double upper = joint->limits.upper;
    
    // 对于continuous关节，默认范围为-π到π
    if (joint->type == JointType::Continuous) {
        lower = -M_PI;
        upper = M_PI;
    }
    
    // 对于revolute关节，转换为度显示
    bool isDegree = (joint->type == JointType::Revolute || joint->type == JointType::Continuous);
    if (isDegree) {
        m_spinBox->setRange(qRadiansToDegrees(lower), qRadiansToDegrees(upper));
        m_spinBox->setSuffix(" °");
    } else {
        m_spinBox->setRange(lower, upper);
        m_spinBox->setSuffix(" m");
    }
    m_spinBox->setValue(0);
    
    sliderLayout->addWidget(m_slider);
    sliderLayout->addWidget(m_spinBox);
    mainLayout->addLayout(sliderLayout);
    
    // 范围标签
    QHBoxLayout* rangeLayout = new QHBoxLayout();
    QString rangeText;
    if (isDegree) {
        rangeText = tr("范围: [%1°, %2°]")
                        .arg(qRadiansToDegrees(lower), 0, 'f', 1)
                        .arg(qRadiansToDegrees(upper), 0, 'f', 1);
    } else {
        rangeText = tr("范围: [%1m, %2m]")
                        .arg(lower, 0, 'f', 3)
                        .arg(upper, 0, 'f', 3);
    }
    QLabel* rangeLabel = new QLabel(rangeText, this);
    rangeLabel->setStyleSheet("color: gray; font-size: 10px;");
    rangeLayout->addWidget(rangeLabel);
    rangeLayout->addStretch();
    mainLayout->addLayout(rangeLayout);
    
    // 连接信号
    connect(m_slider, &QSlider::valueChanged, this, &JointSlider::onSliderChanged);
    connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
            this, &JointSlider::onSpinBoxChanged);
}

double JointSlider::value() const
{
    // 返回弧度值
    double displayValue = m_spinBox->value();
    if (m_joint->type == JointType::Revolute || m_joint->type == JointType::Continuous) {
        return qDegreesToRadians(displayValue);
    }
    return displayValue;
}

void JointSlider::setValue(double value)
{
    m_updating = true;
    
    // 转换为显示值
    double displayValue = value;
    if (m_joint->type == JointType::Revolute || m_joint->type == JointType::Continuous) {
        displayValue = qRadiansToDegrees(value);
    }
    
    m_spinBox->setValue(displayValue);
    
    // 更新滑块
    double lower = m_spinBox->minimum();
    double upper = m_spinBox->maximum();
    double range = upper - lower;
    if (range > 0) {
        int sliderValue = static_cast<int>((displayValue - lower) / range * 1000);
        m_slider->setValue(sliderValue);
    }
    
    m_updating = false;
}

void JointSlider::onSliderChanged(int sliderValue)
{
    if (m_updating) return;
    
    m_updating = true;
    
    double lower = m_spinBox->minimum();
    double upper = m_spinBox->maximum();
    double range = upper - lower;
    double displayValue = lower + (sliderValue / 1000.0) * range;
    
    m_spinBox->setValue(displayValue);
    
    m_updating = false;
    
    emit valueChanged(m_joint->name, value());
}

void JointSlider::onSpinBoxChanged(double displayValue)
{
    if (m_updating) return;
    
    m_updating = true;
    
    // 更新滑块
    double lower = m_spinBox->minimum();
    double upper = m_spinBox->maximum();
    double range = upper - lower;
    if (range > 0) {
        int sliderValue = static_cast<int>((displayValue - lower) / range * 1000);
        m_slider->setValue(sliderValue);
    }
    
    m_updating = false;
    
    emit valueChanged(m_joint->name, value());
}

// ==================== JointControlWidget ====================

JointControlWidget::JointControlWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

JointControlWidget::~JointControlWidget()
{
}

void JointControlWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 标题和重置按钮
    QHBoxLayout* headerLayout = new QHBoxLayout();
    m_titleLabel = new QLabel(tr("关节控制 (0 个关节)"), this);
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold;");
    
    QPushButton* resetButton = new QPushButton(tr("全部重置"), this);
    connect(resetButton, &QPushButton::clicked, this, &JointControlWidget::resetAll);
    
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(resetButton);
    mainLayout->addLayout(headerLayout);
    
    // 滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    m_scrollWidget = new QWidget();
    m_layout = new QVBoxLayout(m_scrollWidget);
    m_layout->setContentsMargins(5, 5, 5, 5);
    m_layout->setSpacing(10);
    m_layout->addStretch();
    
    m_scrollArea->setWidget(m_scrollWidget);
    mainLayout->addWidget(m_scrollArea);
}

void JointControlWidget::setJoints(const QVector<std::shared_ptr<URDFJoint>>& joints)
{
    clear();
    
    int index = 1;
    for (const auto& joint : joints) {
        if (!joint->isMovable()) continue;
        
        JointSlider* slider = new JointSlider(joint, index, m_scrollWidget);
        connect(slider, &JointSlider::valueChanged,
                this, [=](const QString& jointName, double value){
            QMap<QString, double> vals;
            vals.insert(jointName, value);
            emit jointValueChanged(vals);
        });
        
        m_sliders[joint->name] = slider;
        m_layout->insertWidget(m_layout->count() - 1, slider); // 在stretch之前插入
        index++;
    }
    
    updateJointCount();
}

void JointControlWidget::clear()
{
    for (auto slider : m_sliders) {
        m_layout->removeWidget(slider);
        slider->deleteLater();
    }
    m_sliders.clear();
    updateJointCount();
}

void JointControlWidget::updateJointCount()
{
    int count = m_sliders.size();
    m_titleLabel->setText(tr("关节控制 (%1 个关节)").arg(count));
}

double JointControlWidget::getJointValue(const QString& jointName) const
{
    auto it = m_sliders.find(jointName);
    if (it != m_sliders.end()) {
        return it.value()->value();
    }
    return 0;
}

QMap<QString, double> JointControlWidget::getAllJointValues() const
{
    QMap<QString, double> values;
    for (auto it = m_sliders.begin(); it != m_sliders.end(); ++it) {
        values[it.key()] = it.value()->value();
    }
    return values;
}

void JointControlWidget::setJointValue(const QString& jointName, double value)
{
    auto it = m_sliders.find(jointName);
    if (it != m_sliders.end()) {
        it.value()->setValue(value);
    }
}

void JointControlWidget::resetAll()
{
    QMap<QString, double> vals;

    for (auto slider : m_sliders) {
        slider->setValue(0);
        vals.insert(slider->jointName(), 0);
    }

    emit jointValueChanged(vals);
}

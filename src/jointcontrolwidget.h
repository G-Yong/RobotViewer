#ifndef JOINTCONTROLWIDGET_H
#define JOINTCONTROLWIDGET_H

#pragma execution_character_set("utf-8")
#include <QWidget>
#include <QMap>
#include <QSlider>
#include <QLabel>
#include <QDoubleSpinBox>
#include <memory>

#include "urdfparser.h"

class QVBoxLayout;
class QScrollArea;

/**
 * @brief 单个关节控制器
 */
class JointSlider : public QWidget
{
    Q_OBJECT
    
public:
    explicit JointSlider(std::shared_ptr<URDFJoint> joint, int index, QWidget* parent = nullptr);
    
    QString jointName() const { return m_joint->name; }
    int jointIndex() const { return m_index; }
    double value() const;
    void setValue(double value);
    
signals:
    void valueChanged(const QString& jointName, double value);
    
private slots:
    void onSliderChanged(int value);
    void onSpinBoxChanged(double value);
    
private:
    std::shared_ptr<URDFJoint> m_joint;
    int m_index;
    QSlider* m_slider = nullptr;
    QDoubleSpinBox* m_spinBox = nullptr;
    QLabel* m_nameLabel = nullptr;
    QLabel* m_valueLabel = nullptr;
    
    bool m_updating = false;
};

/**
 * @brief 关节控制面板
 * 显示所有可动关节的滑块控制器
 */
class JointControlWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit JointControlWidget(QWidget* parent = nullptr);
    ~JointControlWidget();
    
    /**
     * @brief 设置关节列表
     * @param joints 可动关节列表
     */
    void setJoints(const QVector<std::shared_ptr<URDFJoint>>& joints);
    
    /**
     * @brief 清除所有关节控制器
     */
    void clear();
    
    /**
     * @brief 获取关节值
     */
    double getJointValue(const QString& jointName) const;
    
    /**
     * @brief 获取所有关节值
     */
    QMap<QString, double> getAllJointValues() const;
    
public slots:
    /**
     * @brief 设置关节值
     */
    void setJointValue(const QString& jointName, double value);
    
    /**
     * @brief 重置所有关节
     */
    void resetAll();
    
signals:
    /**
     * @brief 关节值改变信号
     */
    void jointValueChanged(QMap<QString, double> vals);
    
private:
    void setupUI();
    void updateJointCount();
    
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollWidget = nullptr;
    QVBoxLayout* m_layout = nullptr;
    QLabel* m_titleLabel = nullptr;
    QMap<QString, JointSlider*> m_sliders;
};

#endif // JOINTCONTROLWIDGET_H

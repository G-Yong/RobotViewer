#ifndef VIEWOPTIONS_H
#define VIEWOPTIONS_H

#pragma execution_character_set("utf-8")

class RobotScene;
class SettingsManager;

/**
 * @brief 用于集中管理场景视图选项的状态与应用逻辑。
 * 将原先分散在 RobotBridge 内的布尔/数值字段收拢，便于加载、保存与批量应用。
 */
struct ViewOptionsState {
    bool showGrid = true;
    bool showAxes = true;
    bool showJointAxes = false;
    bool coloredLinks = false;
    bool zUpEnabled = false;
    bool autoScaleEnabled = true;
    bool showTrajectory = true;
    double trajectoryLifetime = 2.0;
};

class ViewOptions
{
public:
    ViewOptions() = default;

    const ViewOptionsState& state() const { return m_state; }

    bool setShowGrid(bool value, RobotScene* scene);
    bool setShowAxes(bool value, RobotScene* scene);
    bool setShowJointAxes(bool value, RobotScene* scene);
    bool setColoredLinks(bool value, RobotScene* scene);
    bool setZUpEnabled(bool value, RobotScene* scene);
    bool setAutoScaleEnabled(bool value, RobotScene* scene);
    bool setShowTrajectory(bool value, RobotScene* scene);
    bool setTrajectoryLifetime(double seconds, RobotScene* scene);

    void applyToScene(RobotScene* scene) const;

    void loadFromSettings(SettingsManager& settings, RobotScene* scene);
    void saveToSettings(SettingsManager& settings) const;

private:
    bool updateBool(bool& target, bool value);

    ViewOptionsState m_state;
};

#endif // VIEWOPTIONS_H

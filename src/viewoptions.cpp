#include "viewoptions.h"
#include "robotscene.h"
#include "settingsmanager.h"

#include <QtMath>

bool ViewOptions::setShowGrid(bool value, RobotScene* scene)
{
    if (!updateBool(m_state.showGrid, value)) return false;
    if (scene) scene->setGridVisible(value);
    return true;
}

bool ViewOptions::setShowAxes(bool value, RobotScene* scene)
{
    if (!updateBool(m_state.showAxes, value)) return false;
    if (scene) scene->setAxesVisible(value);
    return true;
}

bool ViewOptions::setShowJointAxes(bool value, RobotScene* scene)
{
    if (!updateBool(m_state.showJointAxes, value)) return false;
    if (scene) scene->setJointAxesVisible(value);
    return true;
}

bool ViewOptions::setColoredLinks(bool value, RobotScene* scene)
{
    if (!updateBool(m_state.coloredLinks, value)) return false;
    if (scene) scene->setColoredLinksEnabled(value);
    return true;
}

bool ViewOptions::setZUpEnabled(bool value, RobotScene* scene)
{
    if (!updateBool(m_state.zUpEnabled, value)) return false;
    if (scene) scene->setZUpEnabled(value);
    return true;
}

bool ViewOptions::setAutoScaleEnabled(bool value, RobotScene* scene)
{
    if (!updateBool(m_state.autoScaleEnabled, value)) return false;
    if (scene) scene->setAutoScaleEnabled(value);
    return true;
}

bool ViewOptions::setShowTrajectory(bool value, RobotScene* scene)
{
    if (!updateBool(m_state.showTrajectory, value)) return false;
    if (scene) scene->setTrajectoryVisible(value);
    return true;
}

bool ViewOptions::setTrajectoryLifetime(double seconds, RobotScene* scene)
{
    if (qFuzzyCompare(m_state.trajectoryLifetime, seconds)) return false;
    m_state.trajectoryLifetime = seconds;
    if (scene) scene->setTrajectoryLifetime(seconds);
    return true;
}

void ViewOptions::applyToScene(RobotScene* scene) const
{
    if (!scene) return;

    scene->setGridVisible(m_state.showGrid);
    scene->setAxesVisible(m_state.showAxes);
    scene->setJointAxesVisible(m_state.showJointAxes);
    scene->setColoredLinksEnabled(m_state.coloredLinks);
    scene->setZUpEnabled(m_state.zUpEnabled);
    scene->setAutoScaleEnabled(m_state.autoScaleEnabled);
    scene->setTrajectoryVisible(m_state.showTrajectory);
    scene->setTrajectoryLifetime(static_cast<float>(m_state.trajectoryLifetime));
}

void ViewOptions::loadFromSettings(SettingsManager& settings, RobotScene* scene)
{
    setShowGrid(settings.getShowGrid(), scene);
    setShowAxes(settings.getShowAxes(), scene);
    setShowJointAxes(settings.getShowJointAxes(), scene);
    setColoredLinks(settings.getColoredLinks(), scene);
    setZUpEnabled(settings.getZUpEnabled(), scene);
    setAutoScaleEnabled(settings.getAutoScaleEnabled(), scene);
    setShowTrajectory(settings.getShowTrajectory(), scene);
    setTrajectoryLifetime(settings.getTrajectoryLifetime(), scene);
}

void ViewOptions::saveToSettings(SettingsManager& settings) const
{
    settings.setShowGrid(m_state.showGrid);
    settings.setShowAxes(m_state.showAxes);
    settings.setShowJointAxes(m_state.showJointAxes);
    settings.setColoredLinks(m_state.coloredLinks);
    settings.setZUpEnabled(m_state.zUpEnabled);
    settings.setAutoScaleEnabled(m_state.autoScaleEnabled);
    settings.setShowTrajectory(m_state.showTrajectory);
    settings.setTrajectoryLifetime(m_state.trajectoryLifetime);
}

bool ViewOptions::updateBool(bool& target, bool value)
{
    if (target == value) return false;
    target = value;
    return true;
}

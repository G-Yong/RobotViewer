# 点云加载与显示功能设计

- 日期：2026-09-04
- 项目：RobotViewer（Qt 5.15.2 / Qt3D / QML）
- 状态：已确认

## 目标

在正常加载 URDF 机器人的基础上，允许用户在场景中加载并显示一个**点云**（`.ply` 文件），支持：

1. 逐点 **RGB 着色**。
2. 点云坐标系建立在所选 **link** 上，使点云与机械臂底座对齐。
3. 点云作为该 link 的子实体，**实时跟随**该 link 的位姿（关节运动时点云跟着动）。
4. 仅支持同时显示 **一个** 点云。

## 背景（已确认的事实）

- 目标文件：`C:\Users\Administrator\Desktop\dualRobot\motion\tj7\build\Desktop_Qt_5_15_2_MSVC2019_64bit_Profile\release\reachable_pc_projY.ply`
  - 格式：**ASCII PLY**（`format ascii 1.0`），约 3.5MB。
  - 元素：`element vertex 89501`，纯点云（无 `face`）。
  - 属性：`x y z (float)` + `red green blue (uchar)`。
- 项目已有 `TrajectoryEntity` 使用 `QGeometry + QBuffer + QAttribute + QGeometryRenderer` 渲染 `LineStrip` 的成熟模式，可直接扩展为 `Points`。
- `RobotBridge` 负责 QML ↔ C++ 桥接；`openURDF()` 使用原生 `QFileDialog`。
- `LinkEntity` 是带 `QTransform` 的 `QEntity`，其世界变换 = 沿运动链累乘的关节 origin 变换，并继承 `RobotEntity` 的自动缩放 `m_robotTransform`。

## 方案选择

采用 **方案 A**：C++ `PointCloudEntity` + 轻量 ASCII PLY 解析器，点云实体挂到所选 `LinkEntity` 下。

不采用 assimp（对纯点云支持不确定、颜色映射不可靠）；不采用全 QML 构建（8.9 万点塞 QVariantList 过重、跟随逻辑难写）。

## 架构

### 1. 新增 `src/pointcloudentity.h/.cpp` —— `PointCloudEntity : Qt3DCore::QEntity`

- `bool loadFromPLY(const QString& filePath, const QColor& fallbackColor = QColor(200,200,200))`
  - 解析 ASCII PLY 头部：定位 `format`、`element vertex N`，并按序读取 `property <type> <name>`。
  - 提取 `x,y,z`（float）位置与 `red,green,blue`（uchar）颜色。
  - 位置写入 `QBuffer`（float×3，交错）；颜色归一化到 0–1 写入第二个 `QBuffer`（float×3）；文件无颜色属性时用 `fallbackColor` 均匀填充。
  - 建立 `QAttribute`（`position`、`color`），`QGeometryRenderer` 使用 `Points`。
  - 自定义 `QEffect`（GL 4.3 core）：vertex shader `in vec3 position; in vec3 color; uniform mat4 mvp; uniform float pointSize;` 输出 `gl_Position = mvp * vec4(position,1.0); gl_PointSize = pointSize;`，fragment 输出 `vColor`。
  - 点大小机制：shader 的 `pointSize` uniform 由 `QParameter("pointSize", value)` 驱动；同时在实体上添加 `QPointSize` 组件以启用 `GL_PROGRAM_POINT_SIZE`（shader 写 `gl_PointSize` 时生效）。两者统一由 `setPointSize(float)` 更新。
- `void setPointSize(float)` / `void setVisible(bool)`。
- 解析范围：**仅 ASCII**。非 ASCII 头（如 `binary_little_endian`）返回错误并给出状态提示；二进制支持留作后续。

### 2. `RobotScene` 点云管理

- `bool addPointCloud(const QString& plyFile, const QString& linkName)`
  - 通过 `RobotEntity::getLinkEntity(linkName)` 获得 `LinkEntity`，将新建 `PointCloudEntity` 设为它的子节点并调用 `loadFromPLY`。
- `void removePointCloud()` / `void setPointCloudVisible(bool)` / `bool pointCloudLoaded()`。
- 信号：`pointCloudLoadedChanged()` / `pointCloudVisibleChanged()`。
- 边界：机器人未加载或 link 不存在 → 返回失败并给出错误提示；加载第二个点云前先移除旧的（仅支持一个）。加载新 URDF 时清除点云。

### 3. `RobotBridge` 暴露给 QML

- 属性：`pointCloudVisible`、`pointCloudLoaded`、`pointCloudLink`。
- `Q_INVOKABLE void openPointCloud()`：沿用 `openURDF` 的 `QFileDialog` 模式，弹出 `.ply` 选择，再用当前 `pointCloudLink` 加载。
- `Q_INVOKABLE void setPointCloudLink(const QString&)`：切换 link 时把点云重新挂到新 link 下（继续跟随）。
- `Q_INVOKABLE void removePointCloud()`。

### 4. QML UI —— `ViewOptionsPanel`

新增「点云」`SettingsGroup`：

- `GlassComboBox`（绑定 `robotBridge.linkNames`）选择 link，默认取第一个（通常是 base）。
- `GlassButton`「加载点云…」→ `robotBridge.openPointCloud()`。
- `GlassToggle`「显示点云」→ `robotBridge.pointCloudVisible`。
- `GlassButton`「清除点云」→ `robotBridge.removePointCloud()`。
- 可选：点大小 `GlassSlider` → `robotBridge` 或 `PointCloudEntity::setPointSize`。

## 数据流

加载 URDF → `linkNames` 就绪 → QML 选 link → 「加载点云」→ C++ 弹文件框 → 解析 PLY → 建 `PointCloudEntity` 挂到 `LinkEntity` → 自动对齐并随关节实时跟随。切换 link / 显隐 / 清除均即时生效。

## 错误处理

- 文件不存在、不可读 → `statusMessage` 报错并返回失败。
- 非 ASCII PLY、缺少必需属性 → 报错，不添加。
- 机器人未加载 / link 不存在 → 报错。
- 无颜色属性 → 用默认 `fallbackColor` 回退。

## 测试

- 用 `reachable_pc_projY.ply` 配 `base_link` 加载，确认与机械臂底座对齐。
- 换到一个会动的 link 上拖动关节，确认点云跟随。
- 切换 link、显隐、清除、无颜色文件回退，各验证一遍。
- 用 Qt Creator MCP build 验证编译。

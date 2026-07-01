# Changelog / 更新日志

本文件记录项目的重要变更。

格式参考 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)。

## [Unreleased]

### Added

- `DragGridWidget` exposes main configuration options through `Q_PROPERTY`.
- Added `itemMoved(int from, int to)` and `orderChanged(const QList<QWidget *> &widgets)` for easier order persistence.
- Added public API contract and release checklist documentation.
- CI now verifies install/export metadata with `cmake --install`.

## [0.2.0] - 2026-07-01

### Added

- 新增键盘拖拽支持：`Space` 拾取、方向键移动、`Enter` 放下、`Escape` 取消。
- 新增可选 `dragHandle()` / `setDragHandle()`，可限制仅通过手柄控件启动拖拽。
- 新增自动滚动边距/速度、占位符透明度/脉冲时长配置。
- 新增不完整行填满整行能力：`setFillIncompleteRowEnabled()`，旧的 `setCompactWhenSparseEnabled()` 作为兼容别名保留。
- 演示程序新增列数、拖拽开关、不完整行填满、动画时长和清空卡片控制。
- 新增基于 Qt Test 的 `DragGridLayout` 和 `DragGridWidget` 单元测试。
- 更新项目预览图，展示最新拖拽和布局效果。

### Changed

- 将 `GridDragController` 状态合并到 `DragGridWidget`，并移除不再使用的控制器类。
- `finishDrag()` 现在会将被拖拽控件平滑动画到最终单元格。
- `autoScroll()` 不再标记为 `const`。
- 将拖拽清理逻辑收敛到 `cleanupDragUi()`，避免 `finishDrag()` 与 `cancelDrag()` 重复实现。
- CI 在 macOS 上改用 Qt 6.9.3，避开 Qt 6.8.0 与最新 macOS SDK 的 `AGL.framework` 链接问题。

### Fixed

- 键盘拖拽不再抓取鼠标；使用方向键移动占位符时，视口会自动滚动。
- 拖拽手柄检测改为沿父级链查找，提高嵌套控件场景的稳定性。
- `setPlaceholderOpacity()` 增加占位符控件未构造时的保护。
- 移除 `DragGridWidget.cpp` 中未使用的 `#include <QCursor>`。
- 子控件发出 `LayoutRequest` 时，`DragGridLayout` 会使最小单元格尺寸缓存失效。
- 几何动画完成后会从 `m_geometryAnimations` 移除，避免残留条目。
- 修复不完整行填满时拖拽占位符和实际控件几何不一致导致的错位。
- 修复原位置释放时复用拖拽镜像缩放几何导致的卡片重叠。
- 修复非中心点按下拖拽时镜像锚点跳动的问题。

## [0.1.0] - 2026-06-22

### Added

- 首次开源发布。
- 新增基于 `QLayout` 的自定义 `DragGridLayout`。
- 新增支持鼠标拖拽重排的 `DragGridWidget` 容器。
- 新增示例 `CardWidget` 和演示程序。
- 新增支持 Qt 5 / Qt 6 的 CMake 构建系统。

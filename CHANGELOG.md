# Changelog / 更新日志

本文件记录项目的重要变更。

格式参考 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)。

## [Unreleased]

### Fixed

- 键盘拖拽期间不再触发边缘自动滚动：`autoScroll()` 仅在鼠标抓取有效（真实鼠标拖拽）时执行，键盘拖拽的视口滚动仍由占位符可见性保证。
- 外部直接销毁受管控件（不经 `removeWidget()` / `deleteWidget()`）时，`DragGridLayout` 自动摘除对应布局项并清理动画表项，消除布局计算中的 use-after-free。
- 拖拽中的控件被外部销毁时，`DragGridWidget` 安全终止拖拽并清理占位符、镜像与鼠标抓取状态。
- `setPlaceholderOpacity()` 现在同步更新占位符脉冲动画的关键帧，避免脉冲仍在旧透明度区间振荡。
- 几何动画所属控件被销毁后，`m_geometryAnimations` 中的残留条目会被清理。
- 拖拽容器在拖拽未结束时被销毁不再解引用已销毁的内部对象（析构期间 `destroyed` 回调直接退出）。
- 启动拖拽时若焦点在被拖拽控件上，先收走焦点再隐藏，避免 Qt 6.9+ 的 QScrollArea 因焦点转移滚动视口，导致拖拽一开始内容就跳走。
- 边缘自动滚动改用真实光标位置（`QCursor::pos()`）计算：内容滚动后不再依据过期坐标漂移，鼠标静止在边缘时可持续滚动；速度在边缘处饱和于 `autoScrollMaxSpeed`，远离边缘不会无限加速。
- 外部销毁受管控件现在会立即刷新空状态提示与几何约束（含拖拽进行中销毁非拖拽控件的情形），不再等到下一次交互。
- 拖拽启动失败（如按住的控件已被程序移除）不再把状态机卡在拖拽中，Esc 与后续拖拽均可正常工作。
- `takeWidget()` / `takeAt()` 移除控件时会断开 `destroyed` 连接，避免移除后的销毁触发无效回调。

## [0.3.0] - 2026-07-01

### Added

- `DragGridWidget` exposes main configuration options through `Q_PROPERTY`.
- Added `itemMoved(int from, int to)` and `orderUpdated(const QList<QWidget *> &widgets)` for easier order persistence.
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

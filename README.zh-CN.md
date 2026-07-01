# QtDragGrid

[![CI](https://github.com/qiuyongjun/QtDragGrid/actions/workflows/ci.yml/badge.svg)](https://github.com/qiuyongjun/QtDragGrid/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/qiuyongjun/QtDragGrid)](https://github.com/qiuyongjun/QtDragGrid/releases)

一个可复用的 Qt Widgets 拖拽网格布局组件。

QtDragGrid 基于 `QLayout` 实现了一套自定义网格布局，并提供了支持拖拽重排、自动滚动、列数可配置的容器控件，同时兼容 Qt 5 与 Qt 6。

> **English README**: [README.md](README.md)

## 功能特性

- 在网格中拖拽卡片进行重排
- 拖拽至视口边缘时自动滚动
- 可配置列数与单元格最小尺寸
- 支持等分单元格与不完整行填满整行
- 纯 Qt Widgets，无第三方依赖
- 兼容 Qt 5.15+ 与 Qt 6.x

## 效果预览

![拖拽重排效果预览](docs/preview.gif)

## 环境要求

- Qt 5.15+ 或 Qt 6.x
- CMake 3.16+
- C++17 编译器

## 构建

### 构建演示程序

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

运行演示程序：

```bash
./build/QtDragGridDemo
```

### 运行测试

```bash
ctest --test-dir build --output-on-failure
```

### 作为库使用

直接从源码引入：

```cmake
add_subdirectory(path/to/QtDragGrid)
target_link_libraries(YourApp PRIVATE QtDragGrid::QtDragGrid)
```

也可以安装后通过 CMake package 使用：

```bash
cmake -S . -B build -DQTDRAGGRID_BUILD_EXAMPLES=OFF
cmake --build build --config Release
cmake --install build --prefix /path/to/install
```

```cmake
find_package(QtDragGrid CONFIG REQUIRED)
target_link_libraries(YourApp PRIVATE QtDragGrid::QtDragGrid)
```

然后在代码中：

```cpp
#include <QtDragGrid/DragGridWidget.h>

auto *grid = new QtDragGrid::DragGridWidget(scrollArea, parent);
grid->setDragEnabled(true);
grid->setColumnCount(4);
grid->addWidget(new MyCardWidget());
```

为兼容旧项目，仍保留 `DragGrid` 目标别名和 `<DragGridWidget.h>` 旧 include 路径。

### 主要 API

公共行为契约见 [docs/api.md](docs/api.md)。

- `void setDragEnabled(bool enable)` — 启用/禁用拖拽重排。
- `void setColumnCount(int count)` — 设置列数。
- `void setMinimumCellSize(const QSize &size)` — 设置单元格最小尺寸。
- `void setEqualCellSizeEnabled(bool enable)` — 是否让所有单元格等宽。
- `void setFillIncompleteRowEnabled(bool enable)` — 不完整行是否按剩余项目数填满整行。
- `void setCompactWhenSparseEnabled(bool enable)` — 兼容旧接口，等同于 `setFillIncompleteRowEnabled`。
- `void addWidget(QWidget *widget)` / `void insertWidget(int index, QWidget *widget)` — 添加子控件。
- `void removeWidget(QWidget *widget)` / `void deleteWidget(QWidget *widget)` / `QWidget *takeWidget(int index)` — 移除子控件。
- `void clear()` — 清空并销毁所有子控件。
- `int count() const` / `QList<QWidget *> widgets() const` — 查询当前子控件。
- `void orderChanged()` — 用户通过拖拽改变顺序时发出的信号。
- `void itemMoved(int from, int to)` — 用户拖拽改变顺序后发出移动前后的索引。
- `void orderChanged(const QList<QWidget *> &widgets)` — 用户拖拽改变顺序后发出完整顺序。
- `int dragThreshold() / void setDragThreshold(int)` — 触发拖拽的按压移动阈值（像素，默认 6）。
- `qreal ghostScale() / void setGhostScale(qreal)` — 拖拽镜像缩放比例（默认 1.05）。
- `int animationDuration() / void setAnimationDuration(int)` — 布局过渡动画时长，单位毫秒（默认 200）。
- `int scrollTimerInterval() / void setScrollTimerInterval(int)` — 自动滚动定时器间隔，单位毫秒（默认 16）。
- `QWidget *dragHandle() / void setDragHandle(QWidget *)` — 可选拖拽手柄；仅在手柄区域内按下才启动拖拽。
- `int autoScrollMargin() / void setAutoScrollMargin(int)` — 触发自动滚动的视口边缘距离（默认 40）。
- `int autoScrollMaxSpeed() / void setAutoScrollMaxSpeed(int)` — 自动滚动最大速度，单位像素（默认 16）。
- `qreal placeholderOpacity() / void setPlaceholderOpacity(qreal)` — 占位符透明度（默认 0.5）。
- `int placeholderPulseDuration() / void setPlaceholderPulseDuration(int)` — 占位符脉冲动画时长，单位毫秒（默认 800）。
- `QString emptyText() / void setEmptyText(const QString &)` — 设置空状态提示文本。
- `bool emptyStateVisible() / void setEmptyStateVisible(bool)` — 显示或隐藏空状态提示。

### 键盘重排

当子控件获得焦点且已启用拖拽时：

- `Space` — 拾取当前卡片。
- `方向键` — 移动占位符位置；若占位符移出视口，视口会自动滚动。
- `Enter` — 将卡片放到占位符位置。
- `Escape` — 取消拖拽并恢复原始顺序，焦点回到被拖拽卡片。

> **注意：** 拖拽过程中调用 `setDragEnabled(false)` 会强制将卡片落到当前占位符位置。

### 拖拽手柄

默认情况下整张卡片都可拖拽。如需限制仅在某个子控件（如标题栏）上启动拖拽，请使用 `setDragHandle(widget)`。手柄控件必须是已添加到 `DragGridWidget` 的卡片的子控件，所有权仍归卡片所有。

## 项目结构

```
.
├── include/QtDragGrid/     # 公共头文件
├── src/                    # 库实现
├── examples/basic/         # 演示程序
├── tests/                  # 单元测试
├── cmake/                  # CMake package 配置模板
├── CMakeLists.txt          # 构建配置
└── .github/workflows/      # CI 配置
```

## 参与贡献

欢迎提交 Issue 和 Pull Request。请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md) 了解贡献规范。

## 行为准则

本项目遵循 [Contributor Covenant 行为准则](CODE_OF_CONDUCT.md)。

## 许可证

本项目采用 [MIT 许可证](LICENSE)。

## 作者

- GitHub: [@qiuyongjun](https://github.com/qiuyongjun)

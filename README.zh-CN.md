# QtDragGrid

[![CI](https://github.com/<your-github-username>/GridLayout/actions/workflows/ci.yml/badge.svg)](https://github.com/<your-github-username>/GridLayout/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/<your-github-username>/GridLayout)](https://github.com/<your-github-username>/GridLayout/releases)

一个可复用的 Qt Widgets 拖拽网格布局组件。

QtDragGrid 基于 `QLayout` 实现了一套自定义网格布局，并提供了支持拖拽重排、自动滚动、列数可配置的容器控件，同时兼容 Qt 5 与 Qt 6。

> **English README**: [README.md](README.md)

## 功能特性

- 在网格中拖拽卡片进行重排
- 拖拽至视口边缘时自动滚动
- 可配置列数与单元格最小尺寸
- 支持等分单元格与稀疏紧凑布局
- 纯 Qt Widgets，无第三方依赖
- 兼容 Qt 5.15+ 与 Qt 6.x

## 效果预览

<!-- TODO: 替换为展示拖拽重排效果的 GIF 或截图 -->
![Preview](docs/preview.png)

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
./build/GridLayoutDemo
```

### 作为库使用

QtDragGrid 默认会构建一个名为 `DragGrid` 的静态库，演示程序链接该库。你可以在自己的项目中这样使用：

```cmake
add_subdirectory(path/to/GridLayout)
target_link_libraries(YourApp PRIVATE DragGrid)
```

然后在代码中：

```cpp
#include <DragGridWidget.h>

DragGridWidget *grid = new DragGridWidget(scrollArea, parent);
grid->setDragEnabled(true);
grid->setColumnCount(4);
grid->addWidget(new MyCardWidget());
```

## 项目结构

```
.
├── DragGridLayout.*        # 自定义 QLayout 实现
├── DragGridWidget.*        # 拖拽重排容器控件
├── GridDragController.*    # 拖拽状态与自动滚动控制器
├── CardWidget.*            # 演示用卡片控件
├── MainWindow.* / main.cpp # 演示程序入口
├── style.qss               # 演示样式表
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

- GitHub: [@<your-github-username>](https://github.com/<your-github-username>)

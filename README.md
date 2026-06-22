# QtDragGrid

[![CI](https://github.com/<your-github-username>/GridLayout/actions/workflows/ci.yml/badge.svg)](https://github.com/<your-github-username>/GridLayout/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/<your-github-username>/GridLayout)](https://github.com/<your-github-username>/GridLayout/releases)

A reusable Qt Widgets component for draggable grid layouts.

QtDragGrid provides a custom grid layout and a container widget that supports drag-and-drop reordering, auto-scroll, and configurable column counts. It is built on top of `QLayout` and works with both Qt 5 and Qt 6.

> **中文文档**: [README.zh-CN.md](README.zh-CN.md)

## Features

- Drag and drop to reorder cards inside a grid
- Auto-scroll while dragging near viewport edges
- Configurable column count and minimum cell size
- Optional equal cell sizing and compact sparse layout
- Pure Qt Widgets, no third-party dependencies
- Compatible with Qt 5.15+ and Qt 6.x

## Preview

<!-- TODO: Replace with a GIF or screenshot that demonstrates drag-and-drop reordering -->
![Preview](docs/preview.png)

## Requirements

- Qt 5.15+ or Qt 6.x
- CMake 3.16+
- C++17 compiler

## Build

### Build the demo

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run the demo:

```bash
./build/GridLayoutDemo
```

### Use as a library

QtDragGrid is built as a static library `DragGrid` linked to the demo executable. To use it in your own project:

```cmake
add_subdirectory(path/to/GridLayout)
target_link_libraries(YourApp PRIVATE DragGrid)
```

Then in your code:

```cpp
#include <DragGridWidget.h>

DragGridWidget *grid = new DragGridWidget(scrollArea, parent);
grid->setDragEnabled(true);
grid->setColumnCount(4);
grid->addWidget(new MyCardWidget());
```

## Project Layout

```
.
├── DragGridLayout.*        # Custom QLayout implementation
├── DragGridWidget.*        # Drag-and-drop container widget
├── GridDragController.*    # Drag state and auto-scroll controller
├── CardWidget.*            # Sample card widget used in the demo
├── MainWindow.* / main.cpp # Demo application entry point
├── style.qss               # Demo stylesheet
├── CMakeLists.txt          # Build configuration
└── .github/workflows/      # CI configuration
```

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on how to submit issues and pull requests.

## Code of Conduct

This project follows the [Contributor Covenant Code of Conduct](CODE_OF_CONDUCT.md).

## License

This project is licensed under the [MIT License](LICENSE).

## Author

- GitHub: [@<your-github-username>](https://github.com/<your-github-username>)

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

A preview GIF or screenshot will be added before the official release.

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

### Run tests

```bash
ctest --test-dir build --output-on-failure
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

### API Overview

- `void setDragEnabled(bool enable)` — Enable or disable drag-and-drop reordering.
- `void setColumnCount(int count)` — Set the number of columns.
- `void setMinimumCellSize(const QSize &size)` — Set the minimum cell size.
- `void setEqualCellSizeEnabled(bool enable)` — Make all cells the same width.
- `void setCompactWhenSparseEnabled(bool enable)` — Compact columns when there are few items.
- `void addWidget(QWidget *widget)` / `void insertWidget(int index, QWidget *widget)` — Add items.
- `void removeWidget(QWidget *widget)` / `void deleteWidget(QWidget *widget)` / `QWidget *takeWidget(int index)` — Remove items.
- `void clear()` — Remove and destroy all items.
- `int count() const` / `QList<QWidget *> widgets() const` — Query current items.
- `void orderChanged()` — Signal emitted when the user reorders items by dragging.
- `int dragThreshold() / void setDragThreshold(int)` — Pixel distance before a press becomes a drag.
- `qreal ghostScale() / void setGhostScale(qreal)` — Scale factor of the drag ghost (default 1.05).
- `int animationDuration() / void setAnimationDuration(int)` — Layout transition duration in milliseconds (default 200).
- `int scrollTimerInterval() / void setScrollTimerInterval(int)` — Auto-scroll timer interval in milliseconds (default 16).
- `QWidget *dragHandle() / void setDragHandle(QWidget *)` — Optional handle widget; only presses inside the handle start a drag.
- `int autoScrollMargin() / void setAutoScrollMargin(int)` — Distance from viewport edge that triggers auto-scroll (default 40).
- `int autoScrollMaxSpeed() / void setAutoScrollMaxSpeed(int)` — Maximum auto-scroll speed in pixels (default 16).
- `qreal placeholderOpacity() / void setPlaceholderOpacity(qreal)` — Opacity of the drop placeholder (default 0.5).
- `int placeholderPulseDuration() / void setPlaceholderPulseDuration(int)` — Placeholder pulse duration in milliseconds (default 800).

### Keyboard Reordering

When a child widget has focus and `setDragEnabled(true)` is active:

- `Space` — Pick up the focused card.
- `Arrow keys` — Move the drop placeholder; the viewport scrolls automatically if the placeholder moves out of view.
- `Enter` — Drop the card at the placeholder position.
- `Escape` — Cancel the drag and restore the original order, returning focus to the dragged card.

> **Note:** Calling `setDragEnabled(false)` while a drag is in progress forces the dragged card to be dropped at the current placeholder position.

### Drag Handle

By default the whole card is draggable. To restrict dragging to a specific sub-widget (e.g. a title bar), use `setDragHandle(widget)`. The handle widget must be a descendant of the cards added to `DragGridWidget`; ownership remains with the card.

## Project Layout

```
.
├── DragGridLayout.*        # Custom QLayout implementation
├── DragGridWidget.*        # Drag-and-drop container widget
├── CardWidget.*            # Sample card widget used in the demo
├── MainWindow.* / main.cpp # Demo application entry point
├── tests/                  # Unit tests
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

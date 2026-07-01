# QtDragGrid

[![CI](https://github.com/qiuyongjun/QtDragGrid/actions/workflows/ci.yml/badge.svg)](https://github.com/qiuyongjun/QtDragGrid/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/qiuyongjun/QtDragGrid)](https://github.com/qiuyongjun/QtDragGrid/releases)

A reusable Qt Widgets component for draggable grid layouts.

QtDragGrid provides a custom grid layout and a container widget that supports drag-and-drop reordering, auto-scroll, and configurable column counts. It is built on top of `QLayout` and works with both Qt 5 and Qt 6.

> **中文文档**: [README.zh-CN.md](README.zh-CN.md)

## Features

- Drag and drop to reorder cards inside a grid
- Auto-scroll while dragging near viewport edges
- Configurable column count and minimum cell size
- Optional equal cell sizing and incomplete-row filling
- Pure Qt Widgets, no third-party dependencies
- Compatible with Qt 5.15+ and Qt 6.x

## Preview

![Drag and drop reordering preview](docs/preview.gif)

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
./build/QtDragGridDemo
```

### Run tests

```bash
ctest --test-dir build --output-on-failure
```

### Use as a library

Use QtDragGrid directly from source:

```cmake
add_subdirectory(path/to/QtDragGrid)
target_link_libraries(YourApp PRIVATE QtDragGrid::QtDragGrid)
```

Or install it and consume the exported CMake package:

```bash
cmake -S . -B build -DQTDRAGGRID_BUILD_EXAMPLES=OFF
cmake --build build --config Release
cmake --install build --prefix /path/to/install
```

```cmake
find_package(QtDragGrid CONFIG REQUIRED)
target_link_libraries(YourApp PRIVATE QtDragGrid::QtDragGrid)
```

Then in your code:

```cpp
#include <QtDragGrid/DragGridWidget.h>

auto *grid = new QtDragGrid::DragGridWidget(scrollArea, parent);
grid->setDragEnabled(true);
grid->setColumnCount(4);
grid->addWidget(new MyCardWidget());
```

The legacy `DragGrid` target and `<DragGridWidget.h>` include path are kept as compatibility aliases.

### API Overview

See [docs/api.md](docs/api.md) for the public behavior contract.

- `void setDragEnabled(bool enable)` — Enable or disable drag-and-drop reordering.
- `void setColumnCount(int count)` — Set the number of columns.
- `void setMinimumCellSize(const QSize &size)` — Set the minimum cell size.
- `void setEqualCellSizeEnabled(bool enable)` — Make all cells the same width.
- `void setFillIncompleteRowEnabled(bool enable)` — Fill incomplete rows by spreading their remaining items across the row.
- `void setCompactWhenSparseEnabled(bool enable)` — Compatibility alias for `setFillIncompleteRowEnabled`.
- `void addWidget(QWidget *widget)` / `void insertWidget(int index, QWidget *widget)` — Add items.
- `void removeWidget(QWidget *widget)` / `void deleteWidget(QWidget *widget)` / `QWidget *takeWidget(int index)` — Remove items.
- `void clear()` — Remove and destroy all items.
- `int count() const` / `QList<QWidget *> widgets() const` — Query current items.
- `void orderChanged()` — Signal emitted when the user reorders items by dragging.
- `void itemMoved(int from, int to)` — Signal emitted with the moved item's original and final index.
- `void orderChanged(const QList<QWidget *> &widgets)` — Signal emitted with the complete order after a user reorder.
- `int dragThreshold() / void setDragThreshold(int)` — Pixel distance before a press becomes a drag.
- `qreal ghostScale() / void setGhostScale(qreal)` — Scale factor of the drag ghost (default 1.05).
- `int animationDuration() / void setAnimationDuration(int)` — Layout transition duration in milliseconds (default 200).
- `int scrollTimerInterval() / void setScrollTimerInterval(int)` — Auto-scroll timer interval in milliseconds (default 16).
- `QWidget *dragHandle() / void setDragHandle(QWidget *)` — Optional handle widget; only presses inside the handle start a drag.
- `int autoScrollMargin() / void setAutoScrollMargin(int)` — Distance from viewport edge that triggers auto-scroll (default 40).
- `int autoScrollMaxSpeed() / void setAutoScrollMaxSpeed(int)` — Maximum auto-scroll speed in pixels (default 16).
- `qreal placeholderOpacity() / void setPlaceholderOpacity(qreal)` — Opacity of the drop placeholder (default 0.5).
- `int placeholderPulseDuration() / void setPlaceholderPulseDuration(int)` — Placeholder pulse duration in milliseconds (default 800).
- `QString emptyText() / void setEmptyText(const QString &)` — Text shown when the grid has no items.
- `bool emptyStateVisible() / void setEmptyStateVisible(bool)` — Show or hide the empty-state text.

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
├── include/QtDragGrid/     # Public headers
├── src/                    # Library implementation
├── examples/basic/         # Demo application
├── tests/                  # Unit tests
├── cmake/                  # CMake package config template
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

- GitHub: [@qiuyongjun](https://github.com/qiuyongjun)

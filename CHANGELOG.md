# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Keyboard drag-and-drop support (Space to pick, arrows to move, Enter to drop, Escape to cancel).
- Optional `dragHandle()` / `setDragHandle()` to restrict drag initiation to a handle widget.
- Configurable auto-scroll margin/speed and placeholder opacity/pulse duration.
- Unit tests for `DragGridLayout` and `DragGridWidget` using Qt Test.

### Changed

- Merged `GridDragController` state into `DragGridWidget` and removed the now-unused controller class.
- `finishDrag()` now animates the dragged widget smoothly back to its final cell.
- `autoScroll()` is no longer marked `const`.
- Refactored drag cleanup into a single `cleanupDragUi()` helper to avoid duplication between `finishDrag()` and `cancelDrag()`.

### Fixed

- Keyboard drag no longer grabs the mouse, and the viewport scrolls automatically when moving the placeholder with arrow keys.
- Drag handle detection now walks the parent chain for robustness.
- `setPlaceholderOpacity()` is guarded against calls before the placeholder widget is constructed.
- Removed unused `#include <QCursor>` from `DragGridWidget.cpp`.
- `DragGridLayout` now invalidates its minimum cell size cache when a child widget emits `LayoutRequest`.
- Completed geometry animations are removed from `m_geometryAnimations` to avoid stale entries.

## [0.1.0] - 2026-06-22

### Added

- Initial open source release.
- Custom `DragGridLayout` based on `QLayout`.
- `DragGridWidget` container with mouse-based drag-and-drop reordering.
- Sample `CardWidget` and a demo application.
- CMake build system with Qt 5 / Qt 6 support.

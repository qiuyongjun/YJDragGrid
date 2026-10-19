# YJDragGrid API Contract

This document describes the public behavior of `YJDragGrid::DragGridWidget`.

## Ownership

- `addWidget()` and `insertWidget()` add a widget to the grid layout. The widget remains a Qt child of the grid container.
- `removeWidget()` removes the widget from the grid and clears its parent. The caller owns the widget afterwards.
- `takeWidget()` removes and returns the widget at the requested index. The caller owns the widget afterwards.
- `deleteWidget()` removes the widget and schedules it for deletion with `deleteLater()`.
- `clear()` removes every managed widget and schedules each one for deletion.
- If a managed widget is destroyed externally (without `removeWidget()` / `deleteWidget()`), the grid automatically removes it from the layout, cleans up, and refreshes the empty state immediately. Do not rely on this for normal removal; use the ownership API above.
- Destroying the grid container while a drag is active is safe; drag UI state is reset during destruction and never touches already-destroyed internals.

## Ordering

- `widgets()` returns widgets in their current visual order.
- User-driven reorder emits all reorder signals only when the final order changes.
- `itemMoved(int from, int to)` reports the original index and final index of the moved widget.
- `orderUpdated(const QList<QWidget *> &widgets)` reports the complete order after the move.
- `orderChanged()` is the source-compatible legacy notification signal.

## Dragging

- Dragging starts only when `dragEnabled` is true.
- `dragThreshold` values below zero are treated as zero.
- `ghostScale` values below `1.0` are treated as `1.0`.
- Calling `setDragEnabled(false)` while dragging cancels the drag and restores the original order.
- Calling `deleteWidget()` or `takeWidget()` for the dragged widget cancels the drag before removal.
- Starting a drag never scrolls the viewport: if the dragged widget holds focus, focus is cleared before it is hidden, so Qt's focus-driven scroll-to-widget does not jump the content.
- Edge auto-scroll runs only during mouse drags and tracks the real cursor position (`QCursor::pos()`), so it keeps scrolling while the mouse rests at the viewport edge; the step speed saturates at `autoScrollMaxSpeed`. Keyboard drags scroll the viewport through placeholder visibility (`ensurePlaceholderVisible`).
- If the pressed widget is removed programmatically between press and drag start, the grid does not enter the dragging state; Esc and subsequent drags keep working.

## Layout

- `columnCount` is clamped to at least `1`.
- `minimumCellSize` is clamped to at least `1x1`.
- `equalCellSizeEnabled` expands cells to fill available row width.
- `fillIncompleteRowEnabled` lets incomplete rows distribute their remaining items across the row.
- `compactWhenSparseEnabled` is a compatibility alias for `fillIncompleteRowEnabled`.

## Empty State

- `emptyText` controls the label shown when the grid has no managed widgets.
- `emptyStateVisible` controls whether the empty-state label can be shown.

## Qt Properties

`DragGridWidget` exposes the main configuration options as `Q_PROPERTY`, so they can be configured through Qt's property system:

- `columnCount`
- `minimumCellSize`
- `dragEnabled`
- `dragThreshold`
- `ghostScale`
- `scrollTimerInterval`
- `animationDuration`
- `autoScrollMargin`
- `autoScrollMaxSpeed`
- `placeholderOpacity`
- `placeholderPulseDuration`
- `equalCellSizeEnabled`
- `fillIncompleteRowEnabled`
- `compactWhenSparseEnabled`
- `emptyText`
- `emptyStateVisible`

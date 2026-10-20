# YJDragGrid API Contract

This document describes the public behavior of `YJDragGrid::DragGridWidget`.

## Ownership

- `addWidget()` appends a widget at the end of the layout item list; `insertWidget(index)` inserts by `widgets()` order. The widget remains a Qt child of the grid container.
- `removeWidget()` removes the widget from the grid and clears its parent. The widget is hidden and the caller owns it afterwards.
- `takeWidget(index)` removes and returns the widget at the requested `widgets()` index. The returned widget is hidden and has no parent; call `show()` before displaying it again.
- `deleteWidget()` removes the widget and schedules it for deletion with `deleteLater()`.
- `clear()` removes every layout item and schedules each managed widget for deletion with `deleteLater()`. Non-widget items (for example spacers added through `layout()->addItem()`) are deleted with their layout item.
- `count()` returns the number of layout items, including non-widget items. `widgets()` returns the managed widgets only; its order is the index space used by `insertWidget()` and `takeWidget()`, so when spacers are present `count()` can be larger than `widgets().size()`. The empty state is driven by `widgets()`, so a grid holding nothing but a spacer still shows its empty-state label.
- If a managed widget is destroyed externally (without `removeWidget()` / `deleteWidget()`), the grid automatically removes it from the layout, cleans up, and refreshes the empty state immediately. Do not rely on this for normal removal; use the ownership API above.
- Destroying the grid container while a drag is active is safe; drag UI state is reset during destruction and never touches already-destroyed internals.

## Ordering

- `widgets()` returns widgets in their current visual order.
- User-driven reorder emits all reorder signals only when the final order changes.
- `itemMoved(int from, int to)` reports the original index and final index of the moved widget in the `widgets()` order, so it always matches the list delivered by `orderUpdated()`, even when the layout also holds non-widget items.
- `orderUpdated(const QList<QWidget *> &widgets)` reports the complete order after the move.
- `orderChanged()` is the source-compatible legacy notification signal.

## Dragging

- `dragHandle` restricts where a mouse drag can start: only the handle itself or its descendants can pick up a card. The handle must live inside a card that is already added to the grid; a handle outside every card silently disables dragging.
- The handle subtree must not consume mouse presses. Controls such as `QPushButton` accept the press before the grid sees it, so pressing them never starts a drag; use a passive widget (for example a `QLabel`) as the handle, or make interactive handle children mouse-transparent when clicking them is not required.
- Dragging starts only when `dragEnabled` is true.
- `dragThreshold` values below zero are treated as zero.
- `ghostScale` is clamped to the range `1.0`–`3.0`.
- Calling `setDragEnabled(false)` while dragging cancels the drag and restores the original order.
- Calling `deleteWidget()` or `takeWidget()` for the dragged widget cancels the drag before removal.
- Starting a drag never scrolls the viewport: if the dragged widget holds focus, focus is cleared before it is hidden, so Qt's focus-driven scroll-to-widget does not jump the content.
- Edge auto-scroll runs only during mouse drags and tracks the real cursor position (`QCursor::pos()`), so it keeps scrolling while the mouse rests at the viewport edge; the step speed saturates at `autoScrollMaxSpeed`. Keyboard drags scroll the viewport through placeholder visibility (`ensurePlaceholderVisible`).
- While edge auto-scroll moves the content, the drag ghost and the placeholder are recomputed from the real cursor position, so the landing hint stays anchored under the pointer instead of drifting with the content.
- A mouse press while a keyboard drag is active cancels the keyboard drag first. The pressed card is then evaluated normally, so the hidden card is always restored and the state machine never keeps an orphaned drag target.
- If the pressed widget is removed programmatically between press and drag start, the grid does not enter the dragging state; Esc and subsequent drags keep working.
- Widgets dropped by the grid animate from the ghost geometry to their final cell. The target geometry always comes from the layout, never from a manually computed rectangle, so a stale layout geometry cannot leave a card at the wrong size.
- A `DragGridWidget` placed inside another one is treated as a single card by the outer grid. Mouse dragging inside the inner grid does not start; keyboard handling stays with the inner grid because the outer one does not install key filters on a nested grid's subtree.

## Keyboard

- `Space` picks up the focused card; arrow keys move the placeholder; `Enter` drops it; `Escape` cancels and restores the original order.
- While `dragEnabled` is true, `Space` takes priority over the activation of focusable controls inside a card (buttons, checkboxes). Without this priority a focused close button would consume `Space` and activate instead of starting a keyboard drag.
- `Space` is left untouched for text inputs, combo boxes, sliders and scroll areas inside a card, so typing spaces keeps working. Those widgets are never turned into pick-up targets.
- Arrow keys, `Enter` and `Escape` are only intercepted while a drag is active; otherwise they keep their normal widget semantics.
- Key handling also covers widgets created after a card was added: the grid installs its filters on newly added descendants (`ChildAdded`), so a button built at runtime cannot swallow `Space` and block keyboard pick-up.
- When `dragEnabled` is false the grid does not intercept any key.

## Layout

- `columnCount` is clamped to at least `1`.
- `minimumCellSize` is clamped to at least `1x1`.
- `equalCellSizeEnabled` expands cells to fill available row width.
- `fillIncompleteRowEnabled` lets incomplete rows distribute their remaining items across the row.
- `compactWhenSparseEnabled` is a compatibility alias for `fillIncompleteRowEnabled`.

## Empty State

- `emptyText` controls the label shown when the grid has no managed widgets.
- `emptyStateVisible` controls whether the empty-state label can be shown.

## Appearance

- The placeholder is a `QFrame` with object name `GridPlaceholder`. It ships a default appearance (highlight-tinted translucent fill plus a panel frame), so drag feedback is visible without any stylesheet.
- Any `#GridPlaceholder` rule in a stylesheet replaces the default look. A semi-transparent `background-color` is painted over the built-in fill, so use an opaque color when the exact tone matters.
- The empty-state label uses object name `GridEmptyState`; the drag ghost uses `GridDragGhost`.
- The drag ghost reproduces the source card at `ghostScale` and preserves the source device pixel ratio, so it stays sharp and correctly sized on high-DPI screens.

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

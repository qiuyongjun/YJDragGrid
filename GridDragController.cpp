#include "GridDragController.h"

#include <QCursor>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QWidget>
#include <QtGlobal>

GridDragController::GridDragController(QObject *parent)
    : QObject(parent)
{
}

void GridDragController::setScrollArea(QScrollArea *scrollArea)
{
    m_scrollArea = scrollArea;
}

void GridDragController::beginDrag(QWidget *widget,
                                   const QPoint &pointOffset,
                                   const DragVisualState &visualState)
{
    if (!widget) {
        return;
    }

    m_draggedWidget = widget;
    m_dragPointOffset = pointOffset;
    m_placeholderIndex = -1;
    m_visualState = visualState;

    widget->setWindowOpacity(0.7);
    widget->raise();
    widget->resize(qMax(visualState.draggedSize.width(), 1),
                   qMax(visualState.draggedSize.height(), 1));

    widget->setProperty("drawing", true);
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
}

void GridDragController::updateDragPosition(const QPoint &widgetPos)
{
    if (!m_draggedWidget) {
        return;
    }

    m_draggedWidget->move(widgetPos);
}

void GridDragController::updatePlaceholderIndex(int index)
{
    m_placeholderIndex = index;
}

void GridDragController::endDrag()
{
    if (!m_draggedWidget) {
        return;
    }

    m_draggedWidget->setWindowOpacity(1.0);
    m_draggedWidget->resize(qMax(m_visualState.normalSize.width(), 1),
                            qMax(m_visualState.normalSize.height(), 1));

    m_draggedWidget->setProperty("drawing", false);
    m_draggedWidget->style()->unpolish(m_draggedWidget);
    m_draggedWidget->style()->polish(m_draggedWidget);

    m_draggedWidget = nullptr;
    m_dragPointOffset = QPoint();
    m_placeholderIndex = -1;
    m_visualState = DragVisualState();
}

int GridDragController::calculatePlaceholderIndex(const QPoint &widgetPos,
                                                  const QSize &cellSize,
                                                  const QPoint &contentOrigin,
                                                  int spacing,
                                                  int columnCount,
                                                  int itemCount) const
{
    if (itemCount <= 0 || cellSize.width() <= 0 || cellSize.height() <= 0) {
        return -1;
    }

    const int safeSpacing = qMax(0, spacing);
    const QPoint dropCenter = widgetPos + QPoint(cellSize.width() / 2, cellSize.height() / 2) - contentOrigin;
    const int stepWidth = qMax(1, cellSize.width() + safeSpacing);
    const int stepHeight = qMax(1, cellSize.height() + safeSpacing);
    const int safeColumnCount = qMax(columnCount, 1);
    const int row = qMax(0, dropCenter.y() / stepHeight);
    const int column = qBound(0, dropCenter.x() / stepWidth, safeColumnCount - 1);
    const int rawIndex = row * safeColumnCount + column;
    return qBound(0, rawIndex, itemCount - 1);
}

void GridDragController::autoScroll() const
{
    if (!m_draggedWidget || !m_scrollArea) {
        return;
    }

    const int margin = 40;
    const int maxSpeed = 10;

    const QPoint globalMouse = QCursor::pos();
    const QPoint mouseInViewport = m_scrollArea->viewport()->mapFromGlobal(globalMouse);

    auto *vBar = m_scrollArea->verticalScrollBar();
    auto *hBar = m_scrollArea->horizontalScrollBar();

    int dy = 0;
    int dx = 0;

    if (mouseInViewport.y() < margin) {
        dy = -((margin - mouseInViewport.y()) * maxSpeed / margin);
    } else if (mouseInViewport.y() > m_scrollArea->viewport()->height() - margin) {
        dy = ((mouseInViewport.y() - (m_scrollArea->viewport()->height() - margin)) * maxSpeed / margin);
    }

    if (mouseInViewport.x() < margin) {
        dx = -((margin - mouseInViewport.x()) * maxSpeed / margin);
    } else if (mouseInViewport.x() > m_scrollArea->viewport()->width() - margin) {
        dx = ((mouseInViewport.x() - (m_scrollArea->viewport()->width() - margin)) * maxSpeed / margin);
    }

    if (dy != 0) {
        vBar->setValue(qBound(vBar->minimum(), vBar->value() + dy, vBar->maximum()));
    }
    if (dx != 0) {
        hBar->setValue(qBound(hBar->minimum(), hBar->value() + dx, hBar->maximum()));
    }
}

bool GridDragController::isDragging() const
{
    return m_draggedWidget != nullptr;
}

QWidget *GridDragController::draggedWidget() const
{
    return m_draggedWidget;
}

QPoint GridDragController::dragPointOffset() const
{
    return m_dragPointOffset;
}

int GridDragController::placeholderIndex() const
{
    return m_placeholderIndex;
}

GridDragController::DragVisualState GridDragController::visualState() const
{
    return m_visualState;
}


#include "DragGridWidget.h"

#include "GridDragController.h"
#include "DragGridLayout.h"

#include <QCursor>
#include <QMargins>
#include <QMouseEvent>
#include <QTimer>
#include <QScrollArea>
#include <QtGlobal>

DragGridWidget::DragGridWidget(QScrollArea *scrollArea, QWidget *parent)
    : QWidget(parent)
{
    m_gridLayout = new DragGridLayout(this);
    setLayout(m_gridLayout);

    m_placeholderWidget = new QWidget(this);
    m_placeholderWidget->setObjectName("GridPlaceholder");
    m_placeholderWidget->setVisible(false);

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(16);

    m_dragController = new GridDragController(this);
    m_dragController->setScrollArea(scrollArea);
    connect(m_scrollTimer, &QTimer::timeout, this, &DragGridWidget::slot_scrollTimer_timeOut);
}

void DragGridWidget::addWidget(QWidget *widget)
{
    insertWidget(count(), widget);
}

void DragGridWidget::insertWidget(int index, QWidget *widget)
{
    if (!widget || indexOfWidget(widget) != -1) {
        return;
    }

    m_gridLayout->insertWidget(index, widget);
    widget->show();
    updateGeometry();
}

void DragGridWidget::removeWidget(QWidget *widget)
{
    QWidget *targetWidget = takeWidget(indexOfWidget(widget));
    if (!targetWidget) {
        return;
    }

    updateGeometry();
}

void DragGridWidget::deleteWidget(QWidget *widget)
{
    QWidget *targetWidget = takeWidget(indexOfWidget(widget));
    if (!targetWidget) {
        return;
    }

    targetWidget->deleteLater();
    updateGeometry();
}

QWidget *DragGridWidget::takeWidget(int index)
{
    QWidget *targetWidget = m_gridLayout->takeWidget(index);
    if (!targetWidget) {
        return nullptr;
    }

    if (targetWidget == m_dragController->draggedWidget()) {
        finishDrag();
    }

    targetWidget->hide();
    targetWidget->setParent(nullptr);
    updatePlaceholder();
    return targetWidget;
}

void DragGridWidget::clear()
{
    while (count() > 0) {
        QWidget *widget = takeWidget(0);
        if (!widget) {
            continue;
        }
        widget->deleteLater();
    }
    updateGeometry();
}

int DragGridWidget::columnCount() const
{
    return m_gridLayout->columnCount();
}

int DragGridWidget::count() const
{
    return m_gridLayout->count();
}

QList<QWidget *> DragGridWidget::widgets() const
{
    return m_gridLayout->widgets();
}

void DragGridWidget::setColumnCount(int columnCount)
{
    m_gridLayout->setColumnCount(columnCount);
    updateGeometry();
}

QSize DragGridWidget::minimumCellSize() const
{
    return m_gridLayout->minimumCellSize();
}

void DragGridWidget::setMinimumCellSize(const QSize &size)
{
    m_gridLayout->setMinimumCellSize(size);
    updateGeometry();
}

void DragGridWidget::setDragEnabled(bool enable)
{
    if (!enable && m_dragController->isDragging()) {
        finishDrag();
        m_gridLayout->activate();
    }
    m_dragEnable = enable;
}

bool DragGridWidget::dragEnabled() const
{
    return m_dragEnable;
}

void DragGridWidget::setEqualCellSizeEnabled(bool enable)
{
    m_gridLayout->setEqualCellSizeEnabled(enable);
    updateGeometry();
}

bool DragGridWidget::equalCellSizeEnabled() const
{
    return m_gridLayout->equalCellSizeEnabled();
}

void DragGridWidget::setCompactWhenSparseEnabled(bool enable)
{
    m_gridLayout->setCompactWhenSparseEnabled(enable);
    updateGeometry();
}

bool DragGridWidget::compactWhenSparseEnabled() const
{
    return m_gridLayout->compactWhenSparseEnabled();
}

void DragGridWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_dragEnable || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    for (QWidget *widget : widgets()) {
        if (!widget || !widget->geometry().contains(event->pos())) {
            continue;
        }

        m_dragController->beginDrag(widget,
                                    event->pos() - widget->pos(),
                                    {
                                        widget->size(),
                                        QSize(qMax(1, static_cast<int>(widget->width() * 1.1)),
                                              qMax(1, static_cast<int>(widget->height() * 1.1)))
                                    });
        m_dragController->updatePlaceholderIndex(indexOfWidget(widget));
        m_gridLayout->setIgnoredWidget(widget);
        m_gridLayout->setPlaceholderIndex(indexOfWidget(widget));
        m_gridLayout->activate();
        updatePlaceholder();
        grabMouse();
        m_scrollTimer->start();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void DragGridWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragEnable || !m_dragController->isDragging()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint cursorPos = mapFromGlobal(QCursor::pos());
    const QPoint widgetPos = cursorPos - m_dragController->dragPointOffset();
    m_dragController->updateDragPosition(widgetPos);

    const int placeholderIndex = placeholderIndexAt(cursorPos);
    if (placeholderIndex != m_dragController->placeholderIndex()) {
        m_dragController->updatePlaceholderIndex(placeholderIndex);
        m_gridLayout->setPlaceholderIndex(placeholderIndex);
        m_gridLayout->activate();
        updatePlaceholder();
    }

    event->accept();
}

void DragGridWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_dragController->isDragging()) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    finishDrag();
    m_gridLayout->activate();
    event->accept();
}

void DragGridWidget::slot_scrollTimer_timeOut()
{
    m_dragController->autoScroll();
}

int DragGridWidget::indexOfWidget(const QWidget *widget) const
{
    return m_gridLayout->indexOf(widget);
}

int DragGridWidget::placeholderIndexAt(const QPoint &pos) const
{
    if (count() <= 0) {
        return -1;
    }

    const QRect contentRect = layoutContentsRect();
    const QRect firstCellRect = m_gridLayout->cellRectForIndex(0, contentRect);
    QSize cellSize = firstCellRect.size();
    if (!cellSize.isValid() || cellSize.isEmpty()) {
        cellSize = m_gridLayout->minimumCellSize();
    }

    const int columns = qMax(1, m_gridLayout->effectiveColumnCount());
    const int spacing = qMax(0, m_gridLayout->spacing());
    const int stepWidth = qMax(1, cellSize.width() + spacing);
    const int stepHeight = qMax(1, cellSize.height() + spacing);
    const QPoint localPos = pos - contentRect.topLeft();
    const int row = qMax(0, localPos.y() / stepHeight);
    const int column = qBound(0, localPos.x() / stepWidth, columns - 1);

    return qBound(0, row * columns + column, count() - 1);
}

bool DragGridWidget::reorderForPlaceholder()
{
    QWidget *draggedWidget = m_dragController->draggedWidget();
    if (!draggedWidget) {
        return false;
    }

    const int from = indexOfWidget(draggedWidget);
    const int to = m_dragController->placeholderIndex();
    return m_gridLayout->moveItem(from, to);
}

void DragGridWidget::finishDrag()
{
    if (!m_dragController->isDragging()) {
        m_scrollTimer->stop();
        m_placeholderWidget->hide();
        return;
    }

    m_scrollTimer->stop();
    const bool hasOrderChanged = reorderForPlaceholder();
    m_gridLayout->setIgnoredWidget(nullptr);
    m_gridLayout->setPlaceholderIndex(-1);
    m_dragController->endDrag();
    m_placeholderWidget->hide();
    if (mouseGrabber() == this) {
        releaseMouse();
    }
    if (hasOrderChanged) {
        emit orderChanged();
    }
}

void DragGridWidget::updatePlaceholder()
{
    const int index = m_dragController->placeholderIndex();
    if (index < 0 || count() == 0) {
        m_placeholderWidget->hide();
        return;
    }

    const QRect placeholderRect = m_gridLayout->cellRectForIndex(index, layoutContentsRect());
    if (!placeholderRect.isValid()) {
        m_placeholderWidget->hide();
        return;
    }

    m_placeholderWidget->setGeometry(placeholderRect);
    m_placeholderWidget->show();
    m_placeholderWidget->lower();
}

QRect DragGridWidget::layoutContentsRect() const
{
    const QMargins margins = m_gridLayout->contentsMargins();
    return rect().adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
}



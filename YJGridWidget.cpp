#include "YJGridWidget.h"

#include "GridDragController.h"
#include "YJGridLayout.h"

#include <QCursor>
#include <QMargins>
#include <QMouseEvent>
#include <QTimer>
#include <QScrollArea>

YJGridWidget::YJGridWidget(QScrollArea *scrollArea, QWidget *parent)
    : QWidget(parent)
{
    m_gridLayout = new YJGridLayout(this);
    setLayout(m_gridLayout);

    m_placeholderWidget = new QWidget(this);
    m_placeholderWidget->setStyleSheet("background-color: rgb(170, 255, 255);");
    m_placeholderWidget->setVisible(false);

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(16);

    m_dragController = new GridDragController();
    m_dragController->setScrollArea(scrollArea);
    connect(m_scrollTimer, &QTimer::timeout, this, &YJGridWidget::slot_scrollTimer_timeOut);
}

YJGridWidget::~YJGridWidget()
{
    delete m_dragController;
}

void YJGridWidget::addWidget(QWidget *widget)
{
    insertWidget(count(), widget);
}

void YJGridWidget::insertWidget(int index, QWidget *widget)
{
    if (!widget || indexOfWidget(widget) != -1) {
        return;
    }

    m_gridLayout->insertWidget(index, widget);
    widget->show();
    updateGeometry();
}

void YJGridWidget::removeWidget(QWidget *widget)
{
    QWidget *targetWidget = takeWidget(indexOfWidget(widget));
    if (!targetWidget) {
        return;
    }

    updateGeometry();
}

void YJGridWidget::deleteWidget(QWidget *widget)
{
    QWidget *targetWidget = takeWidget(indexOfWidget(widget));
    if (!targetWidget) {
        return;
    }

    targetWidget->deleteLater();
    updateGeometry();
}

QWidget *YJGridWidget::takeWidget(int index)
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

void YJGridWidget::clear()
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

int YJGridWidget::getColumnMaxNum() const
{
    return columnCount();
}

int YJGridWidget::columnCount() const
{
    return m_gridLayout->columnCount();
}

int YJGridWidget::count() const
{
    return m_gridLayout->count();
}

QList<QWidget *> YJGridWidget::widgets() const
{
    return m_gridLayout->widgets();
}

void YJGridWidget::setColumnMaxNum(int columnMaxNum)
{
    setColumnCount(columnMaxNum);
}

void YJGridWidget::setColumnCount(int columnCount)
{
    m_gridLayout->setColumnCount(columnCount);
    updateGeometry();
}

QSize YJGridWidget::minimumCellSize() const
{
    return m_gridLayout->minimumCellSize();
}

void YJGridWidget::setMinimumCellSize(const QSize &size)
{
    m_gridLayout->setMinimumCellSize(size);
    updateGeometry();
}

int YJGridWidget::getCellMiniWidth() const
{
    return minimumCellWidth();
}

int YJGridWidget::minimumCellWidth() const
{
    return minimumCellSize().width();
}

void YJGridWidget::setCellMiniWidth(int cellMiniWidth)
{
    setMinimumCellWidth(cellMiniWidth);
}

void YJGridWidget::setMinimumCellWidth(int cellMiniWidth)
{
    QSize size = minimumCellSize();
    size.setWidth(qMax(1, cellMiniWidth));
    setMinimumCellSize(size);
}

int YJGridWidget::getCellMiniHeight() const
{
    return minimumCellHeight();
}

int YJGridWidget::minimumCellHeight() const
{
    return minimumCellSize().height();
}

void YJGridWidget::setCellMiniHeight(int cellMiniHeight)
{
    setMinimumCellHeight(cellMiniHeight);
}

void YJGridWidget::setMinimumCellHeight(int cellMiniHeight)
{
    QSize size = minimumCellSize();
    size.setHeight(qMax(1, cellMiniHeight));
    setMinimumCellSize(size);
}

void YJGridWidget::setDragEnabled(bool enable)
{
    if (!enable && m_dragController->isDragging()) {
        finishDrag();
        m_gridLayout->activate();
    }
    m_dragEnable = enable;
}

bool YJGridWidget::dragEnabled() const
{
    return m_dragEnable;
}

void YJGridWidget::setEqualCellSizeEnabled(bool enable)
{
    m_gridLayout->setEqualCellSizeEnabled(enable);
    updateGeometry();
}

bool YJGridWidget::equalCellSizeEnabled() const
{
    return m_gridLayout->equalCellSizeEnabled();
}

void YJGridWidget::setExpandedCellEnabled(bool enable)
{
    m_gridLayout->setCompactWhenSparseEnabled(enable);
    updateGeometry();
}

bool YJGridWidget::expandedCellEnabled() const
{
    return m_gridLayout->compactWhenSparseEnabled();
}

void YJGridWidget::mousePressEvent(QMouseEvent *event)
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
        m_gridLayout->setIgnoredWidget(widget);
        grabMouse();
        m_scrollTimer->start();
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void YJGridWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragEnable || !m_dragController->isDragging()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint widgetPos = mapFromGlobal(QCursor::pos()) - m_dragController->dragPointOffset();
    m_dragController->updateDragPosition(widgetPos);

    m_gridLayout->activate();
    const QRect contentRect = layoutContentsRect();
    const QRect firstCellRect = m_gridLayout->cellRectForIndex(0, contentRect);
    QSize cellSize = firstCellRect.size();
    if (!cellSize.isValid() || cellSize.isEmpty()) {
        cellSize = m_gridLayout->minimumCellSize();
    }
    const int placeholderIndex = m_dragController->calculatePlaceholderIndex(widgetPos,
                                                                             cellSize,
                                                                             contentRect.topLeft(),
                                                                             m_gridLayout->spacing(),
                                                                             m_gridLayout->effectiveColumnCount(),
                                                                             m_gridLayout->count());
    if (placeholderIndex != m_dragController->placeholderIndex()) {
        m_dragController->updatePlaceholderIndex(placeholderIndex);
        if (reorderForPlaceholder()) {
            emit orderChanged();
            m_gridLayout->activate();
        }
        updatePlaceholder();
    }

    event->accept();
}

void YJGridWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_dragController->isDragging()) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    finishDrag();
    m_gridLayout->activate();
    event->accept();
}

void YJGridWidget::slot_scrollTimer_timeOut()
{
    m_dragController->autoScroll();
}

int YJGridWidget::indexOfWidget(const QWidget *widget) const
{
    return m_gridLayout->indexOf(const_cast<QWidget *>(widget));
}

bool YJGridWidget::reorderForPlaceholder()
{
    QWidget *draggedWidget = m_dragController->draggedWidget();
    if (!draggedWidget) {
        return false;
    }

    const int from = indexOfWidget(draggedWidget);
    const int to = m_dragController->placeholderIndex();
    return m_gridLayout->moveItem(from, to);
}

void YJGridWidget::finishDrag()
{
    if (!m_dragController->isDragging()) {
        m_scrollTimer->stop();
        m_placeholderWidget->hide();
        return;
    }

    m_scrollTimer->stop();
    m_gridLayout->setIgnoredWidget(nullptr);
    m_dragController->endDrag();
    m_placeholderWidget->hide();
    if (mouseGrabber() == this) {
        releaseMouse();
    }
}

void YJGridWidget::updatePlaceholder()
{
    const int index = m_dragController->placeholderIndex();
    if (index < 0 || count() == 0) {
        m_placeholderWidget->hide();
        return;
    }

    m_placeholderWidget->setGeometry(m_gridLayout->cellRectForIndex(index, layoutContentsRect()));
    m_placeholderWidget->show();
    m_placeholderWidget->lower();
}

QRect YJGridWidget::layoutContentsRect() const
{
    const QMargins margins = m_gridLayout->contentsMargins();
    return rect().adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
}

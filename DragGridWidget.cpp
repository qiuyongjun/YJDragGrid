#include "DragGridWidget.h"

#include "GridDragController.h"
#include "DragGridLayout.h"

#include <QCursor>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QMargins>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QScrollBar>
#include <QScrollArea>
#include <QTimer>
#include <QtGlobal>

DragGridWidget::DragGridWidget(QScrollArea *scrollArea, QWidget *parent)
    : QWidget(parent)
    , m_scrollArea(scrollArea)
{
    m_gridLayout = new DragGridLayout(this);
    setLayout(m_gridLayout);

    m_placeholderWidget = new QWidget(this);
    m_placeholderWidget->setObjectName("GridPlaceholder");
    m_placeholderWidget->setVisible(false);

    auto *placeholderOpacity = new QGraphicsOpacityEffect(m_placeholderWidget);
    placeholderOpacity->setOpacity(0.5);
    m_placeholderWidget->setGraphicsEffect(placeholderOpacity);

    m_placeholderPulseAnimation = new QPropertyAnimation(placeholderOpacity, "opacity", this);
    m_placeholderPulseAnimation->setDuration(800);
    m_placeholderPulseAnimation->setStartValue(0.3);
    m_placeholderPulseAnimation->setKeyValueAt(0.5, 0.7);
    m_placeholderPulseAnimation->setEndValue(0.3);
    m_placeholderPulseAnimation->setLoopCount(-1);

    m_emptyStateLabel = new QLabel(tr("暂无卡片，点击上方按钮添加"), this);
    m_emptyStateLabel->setObjectName("GridEmptyState");
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->hide();

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(16);

    m_dragController = new GridDragController(this);
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
    updateEmptyState();
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
    updateEmptyState();
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
    updateEmptyState();
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
    if (!enable && m_dragState == DragState::Dragging) {
        finishDrag();
        m_gridLayout->activate();
    }
    m_dragState = DragState::Idle;
    m_pressWidget = nullptr;
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

    // 使用 childAt 替代线性遍历
    QWidget *hitWidget = childAt(event->pos());
    if (!hitWidget) {
        QWidget::mousePressEvent(event);
        return;
    }

    // 向上查找被布局管理的 widget
    QWidget *targetWidget = nullptr;
    for (QWidget *w : widgets()) {
        if (w == hitWidget || w->isAncestorOf(hitWidget)) {
            targetWidget = w;
            break;
        }
    }

    if (!targetWidget) {
        QWidget::mousePressEvent(event);
        return;
    }

    m_dragState = DragState::Pressed;
    m_pressWidget = targetWidget;
    m_pressPos = event->pos();
    m_pressOffset = event->pos() - targetWidget->pos();
    event->accept();
}

void DragGridWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragState == DragState::Pressed) {
        if ((event->pos() - m_pressPos).manhattanLength() > kDragThreshold) {
            startDragOperation(m_pressWidget, m_pressOffset);
            m_dragState = DragState::Dragging;
        } else {
            return;
        }
    }

    if (m_dragState != DragState::Dragging) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    // 使用事件位置替代 QCursor::pos()
    const QPoint cursorPos = event->pos();
    updateDragGhostPosition(cursorPos);

    const int placeholderIndex = m_gridLayout->targetIndexAt(cursorPos);
    if (placeholderIndex >= 0 && placeholderIndex != m_gridLayout->placeholderIndex()) {
        m_gridLayout->setPlaceholderIndex(placeholderIndex);
        m_gridLayout->activate();
        updatePlaceholder();
    }

    event->accept();
}

void DragGridWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragState == DragState::Dragging) {
        finishDrag();
        m_gridLayout->activate();
        event->accept();
        return;
    }

    if (m_dragState == DragState::Pressed) {
        m_dragState = DragState::Idle;
        m_pressWidget = nullptr;
    }

    QWidget::mouseReleaseEvent(event);
}

void DragGridWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateEmptyState();
}

void DragGridWidget::slot_scrollTimer_timeOut()
{
    autoScroll();
}

int DragGridWidget::indexOfWidget(const QWidget *widget) const
{
    return m_gridLayout->indexOf(widget);
}

void DragGridWidget::startDragOperation(QWidget *widget, const QPoint &offset)
{
    if (!widget) {
        return;
    }

    const int originalIndex = indexOfWidget(widget);
    const QSize normalSize = widget->size();
    const QSize draggedSize(qMax(1, static_cast<int>(widget->width() * kGhostScale)),
                            qMax(1, static_cast<int>(widget->height() * kGhostScale)));

    m_dragController->beginDrag(widget, offset,
                                GridDragController::DragVisualState{normalSize, draggedSize});

    createDragGhost(widget);
    widget->hide();

    m_gridLayout->setIgnoredWidget(widget);
    m_gridLayout->setPlaceholderIndex(originalIndex);
    m_gridLayout->activate();
    updatePlaceholder();

    if (m_placeholderPulseAnimation) {
        m_placeholderPulseAnimation->start();
    }

    grabMouse();
    m_scrollTimer->start();
}

bool DragGridWidget::reorderForPlaceholder()
{
    QWidget *draggedWidget = m_dragController->draggedWidget();
    if (!draggedWidget) {
        return false;
    }

    const int from = indexOfWidget(draggedWidget);
    const int to = m_gridLayout->placeholderIndex();
    return m_gridLayout->moveItem(from, to);
}

void DragGridWidget::finishDrag()
{
    if (m_dragState != DragState::Dragging) {
        m_scrollTimer->stop();
        m_placeholderWidget->hide();
        if (m_placeholderPulseAnimation) {
            m_placeholderPulseAnimation->stop();
        }
        return;
    }

    m_scrollTimer->stop();

    QWidget *draggedWidget = m_dragController->draggedWidget();
    const int finalPlaceholderIndex = m_gridLayout->placeholderIndex();

    const bool hasOrderChanged = reorderForPlaceholder();
    m_gridLayout->setIgnoredWidget(nullptr);
    m_gridLayout->setPlaceholderIndex(-1);
    m_dragController->endDrag();

    clearDragGhost();

    if (draggedWidget && finalPlaceholderIndex >= 0) {
        const QRect targetRect = m_gridLayout->placeholderRectAt(finalPlaceholderIndex);
        if (targetRect.isValid()) {
            draggedWidget->setGeometry(targetRect);
        }
        draggedWidget->show();
    }

    m_placeholderWidget->hide();
    if (m_placeholderPulseAnimation) {
        m_placeholderPulseAnimation->stop();
    }

    m_dragState = DragState::Idle;
    m_pressWidget = nullptr;

    if (mouseGrabber() == this) {
        releaseMouse();
    }
    if (hasOrderChanged) {
        emit orderChanged();
    }
}

void DragGridWidget::updatePlaceholder()
{
    const int index = m_gridLayout->placeholderIndex();
    if (index < 0 || count() == 0) {
        m_placeholderWidget->hide();
        return;
    }

    const QRect placeholderRect = m_gridLayout->placeholderRectAt(index);
    if (!placeholderRect.isValid()) {
        m_placeholderWidget->hide();
        return;
    }

    m_placeholderWidget->setGeometry(placeholderRect);
    m_placeholderWidget->show();
    m_placeholderWidget->lower();
}

void DragGridWidget::createDragGhost(QWidget *source)
{
    if (!source) {
        return;
    }

    clearDragGhost();

    const QPixmap sourcePixmap = source->grab();
    const QSize scaledSize(qMax(1, static_cast<int>(source->width() * kGhostScale)),
                           qMax(1, static_cast<int>(source->height() * kGhostScale)));
    const QPixmap scaledPixmap = sourcePixmap.scaled(scaledSize,
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation);

    // 生成半透明镜像（子控件用 QPainter 控制 alpha，比 setWindowOpacity 更可靠）。
    QPixmap ghostPixmap(scaledSize);
    ghostPixmap.fill(Qt::transparent);
    {
        QPainter painter(&ghostPixmap);
        painter.setOpacity(0.85);
        painter.drawPixmap(0, 0, scaledPixmap);
    }

    auto *ghost = new QLabel(this);
    ghost->setObjectName("GridDragGhost");
    ghost->setPixmap(ghostPixmap);
    ghost->resize(ghostPixmap.size());
    ghost->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *shadowEffect = new QGraphicsDropShadowEffect(ghost);
    shadowEffect->setBlurRadius(20);
    shadowEffect->setColor(QColor(0, 0, 0, 120));
    shadowEffect->setOffset(4, 4);
    ghost->setGraphicsEffect(shadowEffect);

    // 居中覆盖在原控件位置，避免初次跳动。
    const QPoint centerOffset((ghost->width() - source->width()) / 2,
                              (ghost->height() - source->height()) / 2);
    ghost->move(source->pos() - centerOffset);
    ghost->raise();
    ghost->show();

    m_dragGhostWidget = ghost;
}

void DragGridWidget::updateDragGhostPosition(const QPoint &cursorPos)
{
    if (!m_dragGhostWidget) {
        return;
    }

    const QPoint offset = m_dragController->dragPointOffset();
    const QPoint scaledOffset(qRound(offset.x() * kGhostScale),
                              qRound(offset.y() * kGhostScale));
    m_dragGhostWidget->move(cursorPos - scaledOffset);
}

void DragGridWidget::clearDragGhost()
{
    if (!m_dragGhostWidget) {
        return;
    }

    m_dragGhostWidget->hide();
    m_dragGhostWidget->deleteLater();
    m_dragGhostWidget = nullptr;
}

void DragGridWidget::updateEmptyState()
{
    const bool empty = (count() == 0);
    m_emptyStateLabel->setVisible(empty);
    if (empty) {
        m_emptyStateLabel->setGeometry(rect().adjusted(20, 20, -20, -20));
    }
}

void DragGridWidget::autoScroll() const
{
    if (m_dragState != DragState::Dragging || !m_scrollArea) {
        return;
    }

    const int margin = 40;
    const int maxSpeed = 16;
    const int marginSquared = margin * margin;

    const QPoint globalMouse = QCursor::pos();
    const QPoint mouseInViewport = m_scrollArea->viewport()->mapFromGlobal(globalMouse);

    auto *vBar = m_scrollArea->verticalScrollBar();
    auto *hBar = m_scrollArea->horizontalScrollBar();

    int dy = 0;
    int dx = 0;

    const int viewportHeight = m_scrollArea->viewport()->height();
    const int viewportWidth = m_scrollArea->viewport()->width();

    if (mouseInViewport.y() < margin) {
        const int distance = margin - mouseInViewport.y();
        dy = -qMax(1, maxSpeed * distance * distance / marginSquared);
    } else if (mouseInViewport.y() > viewportHeight - margin) {
        const int distance = mouseInViewport.y() - (viewportHeight - margin);
        dy = qMax(1, maxSpeed * distance * distance / marginSquared);
    }

    if (mouseInViewport.x() < margin) {
        const int distance = margin - mouseInViewport.x();
        dx = -qMax(1, maxSpeed * distance * distance / marginSquared);
    } else if (mouseInViewport.x() > viewportWidth - margin) {
        const int distance = mouseInViewport.x() - (viewportWidth - margin);
        dx = qMax(1, maxSpeed * distance * distance / marginSquared);
    }

    if (dy != 0) {
        vBar->setValue(qBound(vBar->minimum(), vBar->value() + dy, vBar->maximum()));
    }
    if (dx != 0) {
        hBar->setValue(qBound(hBar->minimum(), hBar->value() + dx, hBar->maximum()));
    }
}

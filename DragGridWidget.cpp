#include "DragGridWidget.h"

#include "DragGridLayout.h"

#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
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
    setFocusPolicy(Qt::StrongFocus);

    m_gridLayout = new DragGridLayout(this);
    m_gridLayout->setAnimationDuration(m_animationDuration);
    setLayout(m_gridLayout);

    m_placeholderWidget = new QWidget(this);
    m_placeholderWidget->setObjectName("GridPlaceholder");
    m_placeholderWidget->setVisible(false);

    auto *placeholderOpacity = new QGraphicsOpacityEffect(m_placeholderWidget);
    placeholderOpacity->setOpacity(m_placeholderOpacity);
    m_placeholderWidget->setGraphicsEffect(placeholderOpacity);

    m_placeholderPulseAnimation = new QPropertyAnimation(placeholderOpacity, "opacity", this);
    m_placeholderPulseAnimation->setDuration(m_placeholderPulseDuration);
    m_placeholderPulseAnimation->setStartValue(qMax(0.0, m_placeholderOpacity * 0.6));
    m_placeholderPulseAnimation->setKeyValueAt(0.5, qMin(1.0, m_placeholderOpacity * 1.4));
    m_placeholderPulseAnimation->setEndValue(qMax(0.0, m_placeholderOpacity * 0.6));
    m_placeholderPulseAnimation->setLoopCount(-1);

    m_emptyStateLabel = new QLabel(tr("暂无卡片，点击上方按钮添加"), this);
    m_emptyStateLabel->setObjectName("GridEmptyState");
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->hide();

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(m_scrollTimerInterval);

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
    // 将控件从布局中移除并解除父子关系；调用者获得 widget 所有权，需自行管理生命周期。
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
    QWidget *targetWidget = m_gridLayout->widgets().value(index, nullptr);
    if (!targetWidget) {
        return nullptr;
    }

    // 若目标控件正在拖拽，先完成拖拽落位，避免 takeAt 后 reorderForPlaceholder 找不到该控件。
    if (targetWidget == m_draggedWidget) {
        finishDrag();
        index = m_gridLayout->indexOf(targetWidget);
    }

    targetWidget = m_gridLayout->takeWidget(index);
    if (!targetWidget) {
        return nullptr;
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
        if (finishDrag()) {
            m_gridLayout->activate();
        }
    }
    m_dragState = DragState::Idle;
    m_pressWidget = nullptr;
    m_dragEnable = enable;
}

bool DragGridWidget::dragEnabled() const
{
    return m_dragEnable;
}

int DragGridWidget::dragThreshold() const
{
    return m_dragThreshold;
}

void DragGridWidget::setDragThreshold(int threshold)
{
    m_dragThreshold = qMax(0, threshold);
}

qreal DragGridWidget::ghostScale() const
{
    return m_ghostScale;
}

void DragGridWidget::setGhostScale(qreal scale)
{
    m_ghostScale = qMax(1.0, scale);
}

QWidget *DragGridWidget::dragHandle() const
{
    return m_dragHandle;
}

void DragGridWidget::setDragHandle(QWidget *handle)
{
    m_dragHandle = handle;
}

int DragGridWidget::scrollTimerInterval() const
{
    return m_scrollTimerInterval;
}

void DragGridWidget::setScrollTimerInterval(int ms)
{
    m_scrollTimerInterval = qMax(1, ms);
    if (m_scrollTimer) {
        m_scrollTimer->setInterval(m_scrollTimerInterval);
    }
}

int DragGridWidget::animationDuration() const
{
    return m_animationDuration;
}

void DragGridWidget::setAnimationDuration(int ms)
{
    m_animationDuration = qMax(0, ms);
    if (m_gridLayout) {
        m_gridLayout->setAnimationDuration(m_animationDuration);
    }
}

int DragGridWidget::autoScrollMargin() const
{
    return m_autoScrollMargin;
}

void DragGridWidget::setAutoScrollMargin(int margin)
{
    m_autoScrollMargin = qMax(0, margin);
}

int DragGridWidget::autoScrollMaxSpeed() const
{
    return m_autoScrollMaxSpeed;
}

void DragGridWidget::setAutoScrollMaxSpeed(int speed)
{
    m_autoScrollMaxSpeed = qMax(0, speed);
}

qreal DragGridWidget::placeholderOpacity() const
{
    return m_placeholderOpacity;
}

void DragGridWidget::setPlaceholderOpacity(qreal opacity)
{
    m_placeholderOpacity = qBound(0.0, opacity, 1.0);
    if (!m_placeholderWidget) {
        return;
    }

    auto *opacityEffect = qobject_cast<QGraphicsOpacityEffect *>(m_placeholderWidget->graphicsEffect());
    if (opacityEffect) {
        opacityEffect->setOpacity(m_placeholderOpacity);
    }
}

int DragGridWidget::placeholderPulseDuration() const
{
    return m_placeholderPulseDuration;
}

void DragGridWidget::setPlaceholderPulseDuration(int ms)
{
    m_placeholderPulseDuration = qMax(0, ms);
    if (m_placeholderPulseAnimation) {
        m_placeholderPulseAnimation->setDuration(m_placeholderPulseDuration);
    }
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

    // 若设置了 drag handle，仅允许在手柄区域按下时启动拖拽。
    if (m_dragHandle && !isUnderDragHandle(hitWidget)) {
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
        if ((event->pos() - m_pressPos).manhattanLength() > m_dragThreshold) {
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
    m_lastMousePos = cursorPos;
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
    if (event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

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

void DragGridWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_dragEnable) {
        QWidget::keyPressEvent(event);
        return;
    }

    switch (event->key()) {
    case Qt::Key_Space:
        if (m_dragState == DragState::Idle) {
            startKeyboardDrag();
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
        return;

    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
        if (m_dragState == DragState::Dragging) {
            movePlaceholderByKey(event->key());
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
        return;

    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_dragState == DragState::Dragging) {
            finishDrag();
            m_gridLayout->activate();
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
        return;

    case Qt::Key_Escape:
        if (m_dragState == DragState::Dragging) {
            cancelDrag();
            event->accept();
        } else if (m_dragState == DragState::Pressed) {
            m_dragState = DragState::Idle;
            m_pressWidget = nullptr;
            event->accept();
        } else {
            QWidget::keyPressEvent(event);
        }
        return;

    default:
        QWidget::keyPressEvent(event);
    }
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

void DragGridWidget::startDragOperation(QWidget *widget, const QPoint &offset, bool grabMouse)
{
    if (!widget) {
        return;
    }

    const int originalIndex = indexOfWidget(widget);
    m_keyboardDragStartIndex = originalIndex;

    m_draggedWidget = widget;
    m_dragPointOffset = offset;

    createDragGhost(widget);
    widget->hide();

    m_gridLayout->setIgnoredWidget(widget);
    m_gridLayout->setPlaceholderIndex(originalIndex);
    m_gridLayout->activate();
    updatePlaceholder();

    if (m_placeholderPulseAnimation) {
        m_placeholderPulseAnimation->start();
    }

    if (grabMouse) {
        QWidget::grabMouse();
    }
    m_scrollTimer->start();
}

void DragGridWidget::startKeyboardDrag()
{
    QWidget *focus = focusWidget();
    if (!focus) {
        return;
    }

    QWidget *targetWidget = nullptr;
    for (QWidget *w : widgets()) {
        if (w == focus || w->isAncestorOf(focus)) {
            targetWidget = w;
            break;
        }
    }

    if (!targetWidget) {
        return;
    }

    startDragOperation(targetWidget, QPoint(targetWidget->width() / 2, targetWidget->height() / 2), false);
    m_dragState = DragState::Dragging;
}

bool DragGridWidget::isUnderDragHandle(QWidget *widget) const
{
    if (!m_dragHandle || !widget) {
        return false;
    }

    QWidget *handle = m_dragHandle.data();
    for (QWidget *w = widget; w; w = w->parentWidget()) {
        if (w == handle) {
            return true;
        }
    }
    return false;
}

void DragGridWidget::movePlaceholderByKey(int key)
{
    const int columns = m_gridLayout->effectiveColumnCount();
    if (columns <= 0) {
        return;
    }

    const int itemCount = count();
    if (itemCount <= 1) {
        return;
    }

    int current = m_gridLayout->placeholderIndex();
    if (current < 0) {
        current = 0;
    }

    int next = current;
    switch (key) {
    case Qt::Key_Left:
        next = current - 1;
        break;
    case Qt::Key_Right:
        next = current + 1;
        break;
    case Qt::Key_Up:
        next = current - columns;
        break;
    case Qt::Key_Down:
        next = current + columns;
        break;
    }

    next = qBound(0, next, itemCount - 1);
    if (next != current) {
        m_gridLayout->setPlaceholderIndex(next);
        m_gridLayout->activate();
        updatePlaceholder();
        ensurePlaceholderVisible();
    }
}

void DragGridWidget::cleanupDragUi()
{
    m_scrollTimer->stop();
    clearDragGhost();
    m_placeholderWidget->hide();
    if (m_placeholderPulseAnimation) {
        m_placeholderPulseAnimation->stop();
    }

    if (mouseGrabber() == this) {
        releaseMouse();
    }

    m_dragState = DragState::Idle;
    m_pressWidget = nullptr;
    m_draggedWidget = nullptr;
    m_dragPointOffset = QPoint();
    m_keyboardDragStartIndex = -1;
}

void DragGridWidget::cancelDrag()
{
    if (m_dragState != DragState::Dragging || !m_draggedWidget) {
        return;
    }

    // 恢复到拖拽前的顺序，再结束拖拽。
    if (m_keyboardDragStartIndex >= 0) {
        const int from = indexOfWidget(m_draggedWidget);
        m_gridLayout->moveItem(from, m_keyboardDragStartIndex);
    }

    m_gridLayout->setIgnoredWidget(nullptr);
    m_gridLayout->setPlaceholderIndex(-1);

    QWidget *draggedWidget = m_draggedWidget;

    cleanupDragUi();

    if (draggedWidget) {
        draggedWidget->show();
        draggedWidget->setFocus();
    }
}

bool DragGridWidget::reorderForPlaceholder()
{
    if (!m_draggedWidget) {
        return false;
    }

    const int from = indexOfWidget(m_draggedWidget);
    const int to = m_gridLayout->placeholderIndex();
    return m_gridLayout->moveItem(from, to);
}

bool DragGridWidget::finishDrag()
{
    if (m_dragState != DragState::Dragging) {
        cleanupDragUi();
        return false;
    }

    QWidget *draggedWidget = m_draggedWidget;
    const int finalPlaceholderIndex = m_gridLayout->placeholderIndex();

    const bool hasOrderChanged = reorderForPlaceholder();
    m_gridLayout->setIgnoredWidget(nullptr);
    m_gridLayout->setPlaceholderIndex(-1);

    // 先计算目标位置，再清除幽灵，保证落位动画可以从幽灵当前位置平滑过渡。
    QRect targetRect;
    if (draggedWidget && finalPlaceholderIndex >= 0) {
        targetRect = m_gridLayout->placeholderRectAt(finalPlaceholderIndex);
    }

    QRect ghostRect;
    if (m_dragGhostWidget) {
        ghostRect = m_dragGhostWidget->geometry();
    }

    cleanupDragUi();

    if (draggedWidget) {
        if (targetRect.isValid()) {
            // 如果幽灵可见，从幽灵位置动画归位；否则直接落位。
            if (ghostRect.isValid()) {
                draggedWidget->setGeometry(ghostRect);
                draggedWidget->show();

                auto *settleAnimation = new QPropertyAnimation(draggedWidget, "geometry", draggedWidget);
                settleAnimation->setDuration(m_animationDuration);
                settleAnimation->setEasingCurve(QEasingCurve::OutCubic);
                settleAnimation->setStartValue(ghostRect);
                settleAnimation->setEndValue(targetRect);
                connect(settleAnimation, &QPropertyAnimation::finished, draggedWidget, [settleAnimation]() {
                    settleAnimation->deleteLater();
                });
                settleAnimation->start();
            } else {
                draggedWidget->setGeometry(targetRect);
                draggedWidget->show();
            }
        } else {
            draggedWidget->show();
        }
        draggedWidget->setFocus();
    }

    if (hasOrderChanged) {
        emit orderChanged();
    }
    return true;
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
    const QSize scaledSize(qMax(1, static_cast<int>(source->width() * m_ghostScale)),
                           qMax(1, static_cast<int>(source->height() * m_ghostScale)));
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

    const QPoint scaledOffset(qRound(m_dragPointOffset.x() * m_ghostScale),
                              qRound(m_dragPointOffset.y() * m_ghostScale));
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

void DragGridWidget::ensurePlaceholderVisible()
{
    if (!m_scrollArea) {
        return;
    }

    const int index = m_gridLayout->placeholderIndex();
    if (index < 0) {
        return;
    }

    const QRect rect = m_gridLayout->placeholderRectAt(index);
    if (!rect.isValid()) {
        return;
    }

    m_scrollArea->ensureVisible(rect.center().x(), rect.center().y(),
                                rect.width() / 2 + 10, rect.height() / 2 + 10);
}

void DragGridWidget::autoScroll()
{
    if (m_dragState != DragState::Dragging || !m_scrollArea || !m_scrollArea->viewport()) {
        return;
    }

    const int margin = m_autoScrollMargin;
    const int maxSpeed = m_autoScrollMaxSpeed;
    const int marginSquared = qMax(1, margin * margin);

    const QPoint mouseInViewport = m_scrollArea->viewport()->mapFrom(this, m_lastMousePos);

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

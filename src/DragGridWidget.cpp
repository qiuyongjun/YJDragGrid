#include <YJDragGrid/DragGridWidget.h>

#include <YJDragGrid/DragGridLayout.h>

#include <QCursor>
#include <QApplication>
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

namespace YJDragGrid {

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

    m_emptyStateLabel = new QLabel(m_emptyText, this);
    m_emptyStateLabel->setObjectName("GridEmptyState");
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->hide();

    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(m_scrollTimerInterval);

    connect(m_scrollTimer, &QTimer::timeout, this, &DragGridWidget::slot_scrollTimer_timeOut);

    updateEmptyState();
}

DragGridWidget::~DragGridWidget()
{
    // 基类析构会按创建顺序销毁子控件：网格布局、定时器等先于卡片死亡，
    // 而卡片的 destroyed 处理器仍会被触发。置位标记让处理器直接退出，
    // 避免在析构期触碰已销毁的内部对象。
    m_destructing = true;
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

    // 布局先连接 destroyed（其回调先摘除布局项）；本类回调后连接、后执行，
    // 执行时 count() 已不含被销毁控件，空状态刷新才能得到准确结果。
    m_gridLayout->insertWidget(index, widget);

    // 控件被外部直接销毁时安全终止拖拽，避免 m_draggedWidget / m_pressWidget 悬垂。
    connect(widget, &QObject::destroyed, this, [this](QObject *obj) {
        handleWidgetDestroyed(obj);
    });

    widget->show();
    updateEmptyState();
    updateGeometry();
}

void DragGridWidget::handleWidgetDestroyed(QObject *obj)
{
    if (m_destructing) {
        return;
    }

    if (m_dragState == DragState::Idle) {
        // 布局侧回调已先摘除布局项；此处补齐容器侧的空状态刷新。
        updateEmptyState();
        updateGeometry();
        return;
    }

    if (obj != static_cast<QObject *>(m_draggedWidget)
        && obj != static_cast<QObject *>(m_pressWidget)) {
        // 拖拽/按压目标之外的其他受管控件被外部销毁：布局侧已摘除布局项，
        // 这里补齐空状态与几何刷新即可，不终止进行中的拖拽。
        updateEmptyState();
        updateGeometry();
        return;
    }

    // cleanupDragUi() 不触碰被拖拽控件，可安全在控件析构中调用；
    // 忽略标记和占位符索引需先复位，让布局回到无占位状态。
    if (m_dragState == DragState::Dragging) {
        m_gridLayout->setIgnoredWidget(nullptr);
        m_gridLayout->setPlaceholderIndex(-1);
    }
    cleanupDragUi();
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

    // 删除拖拽中的控件时按取消处理，避免把临时占位顺序提交为业务顺序。
    if (targetWidget == m_draggedWidget) {
        cancelDrag();
        index = m_gridLayout->indexOf(targetWidget);
    }

    targetWidget = m_gridLayout->takeWidget(index);
    if (!targetWidget) {
        return nullptr;
    }

    // 控件已脱离网格，断开 destroyed 回调，避免移除后的销毁触发无关的状态刷新。
    disconnect(targetWidget, &QObject::destroyed, this, nullptr);
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
        cancelDrag();
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

    // 脉冲关键帧在构造时按初始透明度固定，改透明度必须同步，否则脉冲仍在旧区间振荡。
    if (m_placeholderPulseAnimation) {
        m_placeholderPulseAnimation->setStartValue(qMax(0.0, m_placeholderOpacity * 0.6));
        m_placeholderPulseAnimation->setKeyValueAt(0.5, qMin(1.0, m_placeholderOpacity * 1.4));
        m_placeholderPulseAnimation->setEndValue(qMax(0.0, m_placeholderOpacity * 0.6));
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

void DragGridWidget::setFillIncompleteRowEnabled(bool enable)
{
    m_gridLayout->setFillIncompleteRowEnabled(enable);
    updateGeometry();
}

bool DragGridWidget::fillIncompleteRowEnabled() const
{
    return m_gridLayout->fillIncompleteRowEnabled();
}

void DragGridWidget::setCompactWhenSparseEnabled(bool enable)
{
    setFillIncompleteRowEnabled(enable);
}

bool DragGridWidget::compactWhenSparseEnabled() const
{
    return fillIncompleteRowEnabled();
}

QString DragGridWidget::emptyText() const
{
    return m_emptyText;
}

void DragGridWidget::setEmptyText(const QString &text)
{
    if (m_emptyText == text) {
        return;
    }
    m_emptyText = text;
    m_emptyStateLabel->setText(m_emptyText);
    updateEmptyState();
}

bool DragGridWidget::emptyStateVisible() const
{
    return m_emptyStateVisible;
}

void DragGridWidget::setEmptyStateVisible(bool visible)
{
    if (m_emptyStateVisible == visible) {
        return;
    }
    m_emptyStateVisible = visible;
    updateEmptyState();
}

void DragGridWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_dragEnable || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

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

    // 命中的通常是卡片内部子控件，需要映射回布局管理的卡片。
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
            // 启动失败（如按住的控件已被程序移除）时不得进入 Dragging，
            // 否则状态机卡死：cancelDrag 会因 m_draggedWidget 为空而直接返回。
            if (!startDragOperation(m_pressWidget, m_pressOffset)) {
                m_dragState = DragState::Idle;
                m_pressWidget = nullptr;
                QWidget::mouseMoveEvent(event);
                return;
            }
            m_dragState = DragState::Dragging;
        } else {
            return;
        }
    }

    if (m_dragState != DragState::Dragging) {
        QWidget::mouseMoveEvent(event);
        return;
    }

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

bool DragGridWidget::startDragOperation(QWidget *widget, const QPoint &offset, bool grabMouse)
{
    if (!widget) {
        return false;
    }

    const int originalIndex = indexOfWidget(widget);
    if (originalIndex < 0) {
        return false;
    }
    m_keyboardDragStartIndex = originalIndex;

    m_draggedWidget = widget;
    m_dragPointOffset = offset;
    m_dragGhostCursorOffset = QPoint();

    createDragGhost(widget);

    // 焦点若在被拖拽控件（或其子控件）上，先收走焦点再隐藏：
    // 否则焦点会转移到下一个控件，QScrollArea 会随之滚动视口（Qt 6.9+ 行为），
    // 表现为拖拽一开始内容就跳走。
    QWidget *focusedWidget = QApplication::focusWidget();
    if (focusedWidget && (focusedWidget == widget || widget->isAncestorOf(focusedWidget))) {
        focusedWidget->clearFocus();
    }
    widget->hide();

    m_gridLayout->setIgnoredWidget(widget);
    m_gridLayout->setPlaceholderIndex(originalIndex);
    m_gridLayout->activate();
    updatePlaceholder();

    if (m_placeholderPulseAnimation) {
        m_placeholderPulseAnimation->start();
    }

    if (grabMouse) {
        // 边缘自动滚动依赖真实鼠标位置，只在鼠标拖拽（已抓取鼠标）时启动定时器；
        // 键盘拖拽的视口滚动由 ensurePlaceholderVisible() 负责。
        QWidget::grabMouse();
        m_scrollTimer->start();
    }
    return true;
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

    if (!startDragOperation(targetWidget, QPoint(targetWidget->width() / 2, targetWidget->height() / 2), false)) {
        return;
    }
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
    m_dragGhostCursorOffset = QPoint();
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

bool DragGridWidget::reorderForPlaceholder(int *from, int *to)
{
    if (!m_draggedWidget) {
        return false;
    }

    const int sourceIndex = indexOfWidget(m_draggedWidget);
    const int targetIndex = m_gridLayout->placeholderIndex();
    if (from) {
        *from = sourceIndex;
    }
    if (to) {
        *to = targetIndex;
    }
    return m_gridLayout->moveItem(sourceIndex, targetIndex);
}

bool DragGridWidget::finishDrag()
{
    if (m_dragState != DragState::Dragging) {
        cleanupDragUi();
        return false;
    }

    QWidget *draggedWidget = m_draggedWidget;
    const int finalPlaceholderIndex = m_gridLayout->placeholderIndex();

    int movedFrom = -1;
    int movedTo = -1;
    const bool hasOrderChanged = reorderForPlaceholder(&movedFrom, &movedTo);
    m_gridLayout->setIgnoredWidget(nullptr);
    m_gridLayout->setPlaceholderIndex(-1);

    QRect targetRect;
    if (draggedWidget && finalPlaceholderIndex >= 0) {
        targetRect = m_gridLayout->placeholderRectAt(finalPlaceholderIndex);
    }

    if (!hasOrderChanged) {
        cleanupDragUi();
        if (draggedWidget) {
            if (targetRect.isValid()) {
                draggedWidget->setGeometry(targetRect);
            }
            draggedWidget->show();
            draggedWidget->setFocus();
        }
        return true;
    }

    // 顺序变化时才从幽灵位置做落位动画；原位释放不能复用 ghost 的缩放几何。
    QRect ghostRect;
    if (m_dragGhostWidget) {
        ghostRect = m_dragGhostWidget->geometry();
    }

    cleanupDragUi();

    if (draggedWidget) {
        if (targetRect.isValid()) {
            // 如果幽灵可见，从幽灵位置动画归位；否则直接落位。
            if (ghostRect.isValid() && m_animationDuration > 0) {
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
        emit itemMoved(movedFrom, movedTo);
        emit orderUpdated(widgets());
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

    const QPoint centerOffset((ghost->width() - source->width()) / 2,
                              (ghost->height() - source->height()) / 2);
    m_dragGhostCursorOffset = m_dragPointOffset + centerOffset;

    // 创建和移动使用同一个鼠标锚点，避免非中心按下时镜像突然偏移。
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

    m_dragGhostWidget->move(cursorPos - m_dragGhostCursorOffset);
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
    const bool show = (count() == 0) && m_emptyStateVisible;
    m_emptyStateLabel->setVisible(show);
    if (show) {
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
    // 内容滚动会改变网格在屏幕上的位置，缓存的网格坐标换算出的"鼠标位置"会随
    // 内容漂移；必须读取真实光标位置，鼠标静止在边缘时才能持续滚动。
    if (m_dragState != DragState::Dragging || !m_scrollArea || !m_scrollArea->viewport()
        || mouseGrabber() != this) {
        return;
    }

    const int margin = m_autoScrollMargin;
    const int maxSpeed = m_autoScrollMaxSpeed;
    if (maxSpeed <= 0) {
        // 速度为 0（或负值）时按"不自动滚动"处理，避免 edgeSpeed 的
        // qBound 上界小于下界导致速度退化为常量 1。
        return;
    }
    const int marginSquared = qMax(1, margin * margin);

    const QPoint mouseInViewport = m_scrollArea->viewport()->mapFromGlobal(QCursor::pos());

    auto *vBar = m_scrollArea->verticalScrollBar();
    auto *hBar = m_scrollArea->horizontalScrollBar();

    int dy = 0;
    int dx = 0;

    const int viewportHeight = m_scrollArea->viewport()->height();
    const int viewportWidth = m_scrollArea->viewport()->width();

    // 越靠近边缘滚动越快，平方曲线能减少刚进入边缘区域时的突兀跳动；
    // 速度在 margin 处饱和于 maxSpeed，远离边缘不会无限加速，也避免乘法溢出。
    const auto edgeSpeed = [maxSpeed, marginSquared](qint64 distance) -> int {
        const qint64 speed = maxSpeed * distance * distance / marginSquared;
        return int(qBound<qint64>(qint64(1), speed, qint64(maxSpeed)));
    };

    if (mouseInViewport.y() < margin) {
        dy = -edgeSpeed(margin - mouseInViewport.y());
    } else if (mouseInViewport.y() > viewportHeight - margin) {
        dy = edgeSpeed(mouseInViewport.y() - (viewportHeight - margin));
    }

    if (mouseInViewport.x() < margin) {
        dx = -edgeSpeed(margin - mouseInViewport.x());
    } else if (mouseInViewport.x() > viewportWidth - margin) {
        dx = edgeSpeed(mouseInViewport.x() - (viewportWidth - margin));
    }

    if (dy != 0) {
        vBar->setValue(qBound(vBar->minimum(), vBar->value() + dy, vBar->maximum()));
    }
    if (dx != 0) {
        hBar->setValue(qBound(hBar->minimum(), hBar->value() + dx, hBar->maximum()));
    }
}

} // namespace YJDragGrid

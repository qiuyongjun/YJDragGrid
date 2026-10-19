#include <YJDragGrid/DragGridWidget.h>

#include <YJDragGrid/DragGridLayout.h>

#include <QCursor>
#include <QAbstractScrollArea>
#include <QAbstractSlider>
#include <QApplication>
#include <QChildEvent>
#include <QComboBox>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QKeyEvent>
#include <QLabel>
#include <QLayoutItem>
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

namespace {

// 文本输入、下拉、滑块与滚动区域对 Space 有独立语义，拖拽拾取不能抢占，
// 否则卡片内的输入框将无法输入空格。
bool spaceBelongsToFocusWidget(const QObject *watched)
{
    const auto *widget = qobject_cast<const QWidget *>(watched);
    if (!widget) {
        return true;
    }

    return widget->testAttribute(Qt::WA_InputMethodEnabled)
            || qobject_cast<const QComboBox *>(widget) != nullptr
            || qobject_cast<const QAbstractSlider *>(widget) != nullptr
            || qobject_cast<const QAbstractScrollArea *>(widget) != nullptr;
}

// 判断控件是否属于另一个（嵌套的）拖拽网格。嵌套网格自行管理按键，
// 外层不得把过滤器装到它的子树上，否则会抢走内层的 Space 拾取。
bool belongsToNestedGrid(const QWidget *widget, const DragGridWidget *owner)
{
    for (const QWidget *current = widget; current; current = current->parentWidget()) {
        if (const auto *grid = qobject_cast<const DragGridWidget *>(current)) {
            return grid != owner;
        }
    }
    return false;
}

} // namespace

DragGridWidget::DragGridWidget(QScrollArea *scrollArea, QWidget *parent)
    : QWidget(parent)
    , m_scrollArea(scrollArea)
{
    // orderUpdated 携带 QList<QWidget *>，队列连接与跨线程调用前需要注册。
    qRegisterMetaType<QList<QWidget *>>("QList<QWidget *>");

    setFocusPolicy(Qt::StrongFocus);

    m_gridLayout = new DragGridLayout(this);
    m_gridLayout->setAnimationDuration(m_animationDuration);
    setLayout(m_gridLayout);

    // 用 QFrame 承载占位符：它自带 frame 绘制路径，未提供 QSS 时也能看见落点；
    // 调用方仍可通过 #GridPlaceholder 样式表完全覆盖默认外观。
    auto *placeholder = new QFrame(this);
    placeholder->setObjectName("GridPlaceholder");
    placeholder->setFrameShape(QFrame::StyledPanel);
    placeholder->setLineWidth(2);
    placeholder->setAttribute(Qt::WA_StyledBackground, true);
    QPalette placeholderPalette = placeholder->palette();
    QColor placeholderFill = placeholderPalette.color(QPalette::Highlight);
    placeholderFill.setAlpha(48);
    placeholderPalette.setColor(QPalette::Window, placeholderFill);
    placeholder->setPalette(placeholderPalette);
    placeholder->setAutoFillBackground(true);
    placeholder->setVisible(false);
    m_placeholderWidget = placeholder;

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

    // 卡片内部的按钮等控件会先截获按键，必须统一转交给网格的拖拽状态机。
    installDragKeyFilters(widget);

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
        // 这里补齐空状态、几何与占位符刷新即可，不终止进行中的拖拽。
        // 占位符几何不刷新会一直画在删除前的旧矩形，直到下一次鼠标移动。
        updateEmptyState();
        updateGeometry();
        m_gridLayout->activate();
        updatePlaceholder();
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
    }

    // widgets() 会跳过非 widget 布局项（例如调用方加入的 spacer），它返回的索引
    // 不能直接当作布局索引使用，必须按控件重新换算，否则会摘错对象。
    const int layoutIndex = m_gridLayout->indexOf(targetWidget);
    if (layoutIndex < 0) {
        return nullptr;
    }

    targetWidget = m_gridLayout->takeWidget(layoutIndex);
    if (!targetWidget) {
        return nullptr;
    }

    // 控件已脱离网格，断开回调并卸载按键过滤器，避免移除后的操作触发无关状态。
    detachWidget(targetWidget);
    updatePlaceholder();
    updateEmptyState();
    return targetWidget;
}

void DragGridWidget::detachWidget(QWidget *widget)
{
    if (!widget) {
        return;
    }

    removeDragKeyFilters(widget);
    disconnect(widget, &QObject::destroyed, this, nullptr);
    widget->hide();
    widget->setParent(nullptr);
}

void DragGridWidget::clear()
{
    // 拖拽目标也在待清列表里，先按取消处理，避免把临时占位顺序写回业务顺序。
    if (m_dragState == DragState::Dragging) {
        cancelDrag();
    }
    m_dragState = DragState::Idle;
    m_pressWidget = nullptr;

    // 用 takeAt 而不是 count()+takeWidget()：布局中可能存在非 widget 项
    //（例如调用方通过 layout()->addItem() 加入的 spacer），此时 count() 永远
    // 大于 0，而 widgets() 取不到任何控件，循环会卡死。
    QLayoutItem *item = nullptr;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        QWidget *widget = item->widget();
        delete item;
        if (!widget) {
            continue;
        }
        detachWidget(widget);
        widget->deleteLater();
    }

    updatePlaceholder();
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
    // 上限用于约束镜像位图的内存占用：缩放比例按面积增长。
    m_ghostScale = qBound(1.0, scale, 3.0);
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
        // 拖拽进行中把时长改成 0 同样会触发忙转，运行中的动画直接停掉。
        if (m_placeholderPulseDuration <= 0
            && m_placeholderPulseAnimation->state() == QAbstractAnimation::Running) {
            m_placeholderPulseAnimation->stop();
        } else {
            m_placeholderPulseAnimation->setDuration(m_placeholderPulseDuration);
        }
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

    // 键盘拖拽进行中不得被鼠标按下覆盖状态：否则 m_draggedWidget 成为孤儿，
    // 被隐藏的卡片无人恢复，Escape/Enter 也因状态不是 Dragging 而失效。
    // 取消当前拖拽后按正常流程重新判定目标，允许用户直接用鼠标接着拖。
    if (m_dragState != DragState::Idle) {
        cancelDrag();
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
    updateDragFeedback(cursorPos);

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
        // show() 可能已经消费掉 dirty 标记，而落位动画需要一次以幽灵位置为
        // 起点的布局计算，这里强制重新布局。
        m_gridLayout->invalidate();
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
            if (m_dragState == DragState::Dragging) {
                event->accept();
            } else {
                // 没找到可拖拽的卡片时不吞按键，交还原控件（如按钮激活）。
                QWidget::keyPressEvent(event);
            }
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
            m_gridLayout->invalidate();
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

bool DragGridWidget::eventFilter(QObject *watched, QEvent *event)
{
    // 卡片加入网格后才创建的子控件不会经过 insertWidget()，必须在这里补装过滤器。
    // 否则运行期构建的按钮会吞掉 Space，键盘拾取对该卡片失效。
    if (event->type() == QEvent::ChildAdded) {
        auto *childEvent = static_cast<QChildEvent *>(event);
        if (auto *childWidget = qobject_cast<QWidget *>(childEvent->child())) {
            installDragKeyFilters(childWidget);
        }
        return QWidget::eventFilter(watched, event);
    }

    if (m_dragEnable) {
        if (event->type() == QEvent::KeyRelease && m_dragState == DragState::Dragging) {
            auto *releaseEvent = static_cast<QKeyEvent *>(event);
            if (releaseEvent->key() == Qt::Key_Space) {
                // 与拾取时被吞掉的 KeyPress 配对，避免按钮收到无按下状态的释放。
                return true;
            }
        }

        if (event->type() == QEvent::KeyPress) {
            auto *keyEvent = static_cast<QKeyEvent *>(event);
            const int key = keyEvent->key();
            // 拖拽进行中，导航与提交键必须由网格独占；空闲时只接管 Space（拾取）。
            // 其余按键保持控件原有语义，避免影响卡片内的输入框、按钮等。
            const bool dragNavigationKey =
                    m_dragState == DragState::Dragging
                    && (key == Qt::Key_Left || key == Qt::Key_Right
                        || key == Qt::Key_Up || key == Qt::Key_Down
                        || key == Qt::Key_Return || key == Qt::Key_Enter
                        || key == Qt::Key_Escape);

            if (dragNavigationKey
                || (key == Qt::Key_Space && !spaceBelongsToFocusWidget(watched))) {
                keyPressEvent(keyEvent);
                if (keyEvent->isAccepted()) {
                    return true;
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void DragGridWidget::installDragKeyFilters(QWidget *root)
{
    if (!root) {
        return;
    }

    // 若整棵子树属于嵌套网格，交给内层自行处理。
    if (belongsToNestedGrid(root, this)) {
        return;
    }

    // 卡片通常在构造期就建好全部子控件；对已存在的后代统一安装即可。
    root->installEventFilter(this);
    const QList<QWidget *> descendants = root->findChildren<QWidget *>();
    for (QWidget *child : descendants) {
        if (belongsToNestedGrid(child, this)) {
            continue;
        }
        child->installEventFilter(this);
    }
}

void DragGridWidget::removeDragKeyFilters(QWidget *root)
{
    if (!root) {
        return;
    }

    root->removeEventFilter(this);
    const QList<QWidget *> descendants = root->findChildren<QWidget *>();
    for (QWidget *child : descendants) {
        child->removeEventFilter(this);
    }
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

    // duration=0 的无限循环动画会立即完成并无限重启，占满事件循环 tick；
    // 0 时长表示"不要脉冲"，直接跳过启动。
    if (m_placeholderPulseAnimation && m_placeholderPulseDuration > 0) {
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
    // 必须用应用级当前焦点：QWidget::focusWidget() 返回的是"最后一个获得过焦点的子控件"，
    // 焦点已经移出网格时它会给出过期结果，导致按 Space 拾取到错误的卡片。
    QWidget *focus = QApplication::focusWidget();
    if (!focus || (focus != this && !isAncestorOf(focus))) {
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

    // show() 可能同步激活布局并清掉 dirty，这里再推一次，保证占位槽被回收。
    m_gridLayout->invalidate();
    m_gridLayout->activate();
}

bool DragGridWidget::reorderForPlaceholder()
{
    if (!m_draggedWidget) {
        return false;
    }

    return m_gridLayout->moveItem(indexOfWidget(m_draggedWidget), m_gridLayout->placeholderIndex());
}

bool DragGridWidget::finishDrag()
{
    if (m_dragState != DragState::Dragging) {
        cleanupDragUi();
        return false;
    }

    QWidget *draggedWidget = m_draggedWidget;

    // 对外报告的索引以 widgets() 的控件序为准；布局序可能含非 widget 项，
    // 直接暴露会让调用方与 orderUpdated() 给出的顺序对不上。
    const int movedFrom = widgets().indexOf(draggedWidget);
    const bool hasOrderChanged = reorderForPlaceholder();
    const int movedTo = widgets().indexOf(draggedWidget);
    m_gridLayout->setIgnoredWidget(nullptr);
    m_gridLayout->setPlaceholderIndex(-1);

    // 只记录幽灵位置作为落位动画的起点；终点交给布局计算。
    // 此处不能用 layout 的 geometry 手工推算目标矩形：布局可能尚未刷新，
    // 算出的矩形与 activate() 的真实结果不一致，落位会先跳到错误尺寸再纠正。
    QRect ghostRect;
    if (m_dragGhostWidget) {
        ghostRect = m_dragGhostWidget->geometry();
    }

    cleanupDragUi();

    if (draggedWidget) {
        // show() 会同步激活布局，必须在它之后再写入起点，否则起点会被覆盖。
        draggedWidget->show();
        if (hasOrderChanged && ghostRect.isValid() && m_animationDuration > 0) {
            draggedWidget->setGeometry(ghostRect);
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

    // grab() 返回的是 DPR-aware 位图：像素尺寸 = 逻辑尺寸 * devicePixelRatio。
    // scaled() 按设备像素处理，所以先归一到 DPR 1.0 再缩放，最后按原 DPR 交还，
    // 否则 150%/200% 缩放下镜像的逻辑尺寸会被缩小一半、锚点也随之偏移。
    QPixmap sourcePixmap = source->grab();
    const qreal sourceDpr = sourcePixmap.devicePixelRatio() > 0.0
            ? sourcePixmap.devicePixelRatio() : 1.0;
    sourcePixmap.setDevicePixelRatio(1.0);

    const QSize scaledPixels(qMax(1, qRound(sourcePixmap.width() * m_ghostScale)),
                             qMax(1, qRound(sourcePixmap.height() * m_ghostScale)));
    const QPixmap scaledPixmap = sourcePixmap.scaled(scaledPixels,
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation);

    // 生成半透明镜像（子控件用 QPainter 控制 alpha，比 setWindowOpacity 更可靠）。
    QPixmap ghostPixmap(scaledPixmap.size());
    ghostPixmap.fill(Qt::transparent);
    {
        QPainter painter(&ghostPixmap);
        painter.setOpacity(0.85);
        painter.drawPixmap(0, 0, scaledPixmap);
    }
    // 物理像素不变，逻辑尺寸回到 source * ghostScale。
    ghostPixmap.setDevicePixelRatio(sourceDpr);

    auto *ghost = new QLabel(this);
    ghost->setObjectName("GridDragGhost");
    ghost->setPixmap(ghostPixmap);
    ghost->resize(qRound(ghostPixmap.width() / sourceDpr),
                  qRound(ghostPixmap.height() / sourceDpr));
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

void DragGridWidget::updateDragFeedback(const QPoint &cursorPos)
{
    updateDragGhostPosition(cursorPos);

    const int placeholderIndex = m_gridLayout->targetIndexAt(cursorPos);
    if (placeholderIndex >= 0 && placeholderIndex != m_gridLayout->placeholderIndex()) {
        m_gridLayout->setPlaceholderIndex(placeholderIndex);
        m_gridLayout->activate();
        updatePlaceholder();
    }
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
    // 空状态判断基于"是否有受管控件"，而不是布局项数：调用方通过 layout()->addItem()
    // 加入的 spacer 会让 count() 大于 0，但网格里一张卡片也没有。
    const bool show = widgets().isEmpty() && m_emptyStateVisible;
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

    const int maxSpeed = m_autoScrollMaxSpeed;
    if (maxSpeed <= 0) {
        // 速度为 0（或负值）时按"不自动滚动"处理，避免 edgeSpeed 的
        // qBound 上界小于下界导致速度退化为常量 1。
        return;
    }

    const QPoint mouseInViewport = m_scrollArea->viewport()->mapFromGlobal(QCursor::pos());

    auto *vBar = m_scrollArea->verticalScrollBar();
    auto *hBar = m_scrollArea->horizontalScrollBar();

    int dy = 0;
    int dx = 0;

    const int viewportHeight = m_scrollArea->viewport()->height();
    const int viewportWidth = m_scrollArea->viewport()->width();

    // 边缘区不得超过视口：否则水平/垂直两个方向会同时命中，且 margin*margin
    // 在 int 下会符号溢出（UB）。先收窄到视口尺寸，再按 qint64 求平方。
    const int margin = qMin(m_autoScrollMargin, qMax(1, qMin(viewportWidth, viewportHeight)));
    const qint64 marginSquared = qMax<qint64>(1, qint64(margin) * qint64(margin));

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

    bool contentScrolled = false;
    if (dy != 0) {
        const int before = vBar->value();
        vBar->setValue(qBound(vBar->minimum(), before + dy, vBar->maximum()));
        contentScrolled = contentScrolled || (vBar->value() != before);
    }
    if (dx != 0) {
        const int before = hBar->value();
        hBar->setValue(qBound(hBar->minimum(), before + dx, hBar->maximum()));
        contentScrolled = contentScrolled || (hBar->value() != before);
    }

    if (contentScrolled) {
        // 内容滚动后，同一个屏幕位置对应的容器坐标已经改变：镜像与占位符必须
        // 立即按真实光标重算，否则落点提示会随内容一起从光标处漂开。
        updateDragFeedback(mapFromGlobal(QCursor::pos()));
    }
}

} // namespace YJDragGrid

#include <YJDragGrid/DragGridLayout.h>

#include <QEvent>
#include <QLayoutItem>
#include <QMargins>
#include <QPropertyAnimation>
#include <QStyle>
#include <QWidget>
#include <QWidgetItem>
#include <QtGlobal>

namespace YJDragGrid {

DragGridLayout::DragGridLayout(QWidget *parent)
    : QLayout(parent)
{
}

DragGridLayout::~DragGridLayout()
{
    QLayoutItem *item = nullptr;
    while ((item = takeAt(0)) != nullptr) {
        delete item;
    }
}

bool DragGridLayout::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        auto *widget = qobject_cast<QWidget *>(watched);
        if (widget && indexOf(widget) >= 0) {
            // 子控件内部布局变化后，其 minimumSize 可能已改变。
            invalidate();
        }
    }
    return QLayout::eventFilter(watched, event);
}

void DragGridLayout::addItem(QLayoutItem *item)
{
    if (!item) {
        return;
    }

    m_items.append(Item{item, {}});
    invalidate();
}

void DragGridLayout::addWidget(QWidget *widget, Qt::Alignment alignment)
{
    insertWidget(m_items.size(), widget, alignment);
}

void DragGridLayout::insertWidget(int index, QWidget *widget, Qt::Alignment alignment)
{
    if (!widget || indexOf(widget) != -1) {
        return;
    }

    addChildWidget(widget);
    widget->installEventFilter(this);
    // 外部直接销毁受管控件时自动摘除布局项，避免布局持有悬垂指针。
    connect(widget, &QObject::destroyed, this, [this](QObject *obj) {
        removeDestroyedWidget(qobject_cast<QWidget *>(obj));
    });
    auto *item = new QWidgetItem(widget);
    m_items.insert(clampedInsertIndex(index), Item{item, alignment});
    invalidate();
}

QLayoutItem *DragGridLayout::itemAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }

    return m_items[index].layoutItem;
}

QLayoutItem *DragGridLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }

    Item item = m_items.takeAt(index);
    const bool removedIgnoredWidget = m_ignoredWidget && item.layoutItem
            && item.layoutItem->widget() == m_ignoredWidget;

    if (item.layoutItem && item.layoutItem->widget()) {
        QWidget *widget = item.layoutItem->widget();
        widget->removeEventFilter(this);
        // 控件已脱离布局，断开 destroyed 回调，避免移除后的销毁触发无效清理。
        disconnect(widget, &QObject::destroyed, this, nullptr);
        stopAnimationForWidget(widget);
    }

    // 占位符索引是视觉槽位，项目减少后可能越界；被忽略项被移除时占位含义也消失。
    if (removedIgnoredWidget) {
        m_ignoredWidget = nullptr;
        m_placeholderIndex = -1;
    } else if (m_placeholderIndex >= m_items.size()) {
        m_placeholderIndex = m_items.isEmpty() ? -1 : m_items.size() - 1;
    }

    invalidate();
    return item.layoutItem;
}

int DragGridLayout::count() const
{
    return m_items.size();
}

void DragGridLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);

    if (m_items.isEmpty()) {
        return;
    }

    const QMargins margins = contentsMargins();
    const QRect contentRect = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    const int columns = effectiveColumnCount();
    const QSize cellSize = effectiveCellSize(contentRect);
    const bool reservePlaceholder = hasActivePlaceholder();
    const int slotCount = visualItemCount();
    int visualIndex = 0;

    for (int index = 0; index < m_items.size(); ++index) {
        QLayoutItem *item = m_items[index].layoutItem;
        if (!item) {
            continue;
        }

        QWidget *widget = item->widget();
        if (widget == m_ignoredWidget) {
            continue;
        }

        // 只有拖拽忽略项存在时才预留占位槽，避免结束拖拽后残留索引挤开布局。
        if (reservePlaceholder && visualIndex == m_placeholderIndex) {
            ++visualIndex;
        }

        QRect cell = cellRect(visualIndex, contentRect, columns, cellSize, slotCount);
        ++visualIndex;
        QRect itemRect = cell;
        if (m_items[index].alignment) {
            itemRect.setSize(expandedSizeForItem(m_items[index], cell.size()));
            itemRect = QStyle::alignedRect(Qt::LeftToRight,
                                           m_items[index].alignment,
                                           itemRect.size(),
                                           cell);
        }

        setWidgetGeometryAnimated(widget, itemRect);
    }
}

QSize DragGridLayout::sizeHint() const
{
    return minimumSize();
}

QSize DragGridLayout::minimumSize() const
{
    if (m_items.isEmpty()) {
        const QMargins margins = contentsMargins();
        return QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    }

    const QSize cellSize = minimumCellSizeForItems();
    const int columns = minimumColumnCount();
    const int rows = rowCount();
    const int layoutSpacing = qMax(0, spacing());
    const int spacingX = layoutSpacing * qMax(0, columns - 1);
    const int spacingY = layoutSpacing * qMax(0, rows - 1);
    const QMargins margins = contentsMargins();

    return QSize(columns * cellSize.width() + spacingX + margins.left() + margins.right(),
                 rows * cellSize.height() + spacingY + margins.top() + margins.bottom());
}

bool DragGridLayout::hasHeightForWidth() const
{
    return true;
}

int DragGridLayout::heightForWidth(int width) const
{
    if (m_items.isEmpty()) {
        const QMargins margins = contentsMargins();
        return margins.top() + margins.bottom();
    }

    const QMargins margins = contentsMargins();
    const QRect contentRect(0, 0, qMax(0, width - margins.left() - margins.right()), 0);
    const QSize cellSize = effectiveCellSize(contentRect);
    const int rows = rowCount();
    return rows * cellSize.height() + qMax(0, rows - 1) * qMax(0, spacing()) + margins.top() + margins.bottom();
}

Qt::Orientations DragGridLayout::expandingDirections() const
{
    return Qt::Horizontal;
}

int DragGridLayout::columnCount() const
{
    return m_columnCount;
}

void DragGridLayout::setColumnCount(int columnCount)
{
    const int safeColumnCount = qMax(1, columnCount);
    if (m_columnCount == safeColumnCount) {
        return;
    }

    m_columnCount = safeColumnCount;
    invalidate();
}

QSize DragGridLayout::minimumCellSize() const
{
    return m_minimumCellSize;
}

void DragGridLayout::setMinimumCellSize(const QSize &size)
{
    const QSize safeSize(qMax(1, size.width()), qMax(1, size.height()));
    if (m_minimumCellSize == safeSize) {
        return;
    }

    m_minimumCellSize = safeSize;
    invalidate();
}

bool DragGridLayout::equalCellSizeEnabled() const
{
    return m_equalCellSizeEnabled;
}

void DragGridLayout::setEqualCellSizeEnabled(bool enable)
{
    if (m_equalCellSizeEnabled == enable) {
        return;
    }

    m_equalCellSizeEnabled = enable;
    invalidate();
}

bool DragGridLayout::compactWhenSparseEnabled() const
{
    return fillIncompleteRowEnabled();
}

void DragGridLayout::setCompactWhenSparseEnabled(bool enable)
{
    setFillIncompleteRowEnabled(enable);
}

bool DragGridLayout::fillIncompleteRowEnabled() const
{
    return m_fillIncompleteRowEnabled;
}

void DragGridLayout::setFillIncompleteRowEnabled(bool enable)
{
    if (m_fillIncompleteRowEnabled == enable) {
        return;
    }

    m_fillIncompleteRowEnabled = enable;
    invalidate();
}

QList<QWidget *> DragGridLayout::widgets() const
{
    QList<QWidget *> result;
    result.reserve(m_items.size());
    for (const Item &item : m_items) {
        if (item.layoutItem && item.layoutItem->widget()) {
            result.append(item.layoutItem->widget());
        }
    }
    return result;
}

int DragGridLayout::indexOf(const QWidget *widget) const
{
    if (!widget) {
        return -1;
    }

    for (int index = 0; index < m_items.size(); ++index) {
        if (m_items[index].layoutItem && m_items[index].layoutItem->widget() == widget) {
            return index;
        }
    }
    return -1;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
int DragGridLayout::indexOf(QWidget *widget) const
{
    return indexOf(static_cast<const QWidget *>(widget));
}
#endif

QWidget *DragGridLayout::takeWidget(int index)
{
    QLayoutItem *item = takeAt(index);
    if (!item) {
        return nullptr;
    }

    QWidget *widget = item->widget();
    delete item;
    return widget;
}

bool DragGridLayout::moveItem(int from, int to)
{
    if (from < 0 || from >= m_items.size()) {
        return false;
    }

    const int targetIndex = qBound(0, to, m_items.size() - 1);
    if (from == targetIndex) {
        return false;
    }

    Item item = m_items.takeAt(from);
    m_items.insert(targetIndex, item);
    invalidate();
    return true;
}

QRect DragGridLayout::cellRectForIndex(int index, const QRect &contentRect) const
{
    if (index < 0 || m_items.isEmpty()) {
        return QRect();
    }

    const int columns = effectiveColumnCount();
    if (columns <= 0) {
        return QRect();
    }

    return cellRect(index, contentRect, columns, effectiveCellSize(contentRect), visualItemCount());
}

int DragGridLayout::effectiveColumnCount() const
{
    if (m_items.isEmpty()) {
        return 0;
    }

    return qMax(1, m_columnCount);
}

QWidget *DragGridLayout::ignoredWidget() const
{
    return m_ignoredWidget;
}

void DragGridLayout::setIgnoredWidget(QWidget *widget)
{
    if (m_ignoredWidget == widget) {
        return;
    }

    m_ignoredWidget = widget;
    invalidate();
}

int DragGridLayout::animationDuration() const
{
    return m_animationDuration;
}

void DragGridLayout::setAnimationDuration(int ms)
{
    m_animationDuration = qMax(0, ms);
}

int DragGridLayout::placeholderIndex() const
{
    return m_placeholderIndex;
}

void DragGridLayout::setPlaceholderIndex(int index)
{
    const int safeIndex = qBound(-1, index, m_items.size() - 1);
    if (m_placeholderIndex == safeIndex) {
        return;
    }

    m_placeholderIndex = safeIndex;
    invalidate();
}

QSize DragGridLayout::effectiveCellSize(const QRect &contentRect) const
{
    QSize cellSize = minimumCellSizeForItems();
    const int columns = effectiveColumnCount();
    if (!m_equalCellSizeEnabled || columns <= 0) {
        return cellSize;
    }

    const int availableWidth = qMax(0, contentRect.width());
    const int spacingWidth = qMax(0, spacing()) * qMax(0, columns - 1);
    const int computedWidth = (availableWidth - spacingWidth) / columns;
    cellSize.setWidth(qMax(cellSize.width(), computedWidth));
    return cellSize;
}

QSize DragGridLayout::minimumCellSizeForItems() const
{
    // 最小单元格尺寸需要同时满足配置值和所有子控件的 minimumSize。
    // 不缓存：子控件可在运行期改最小尺寸，而 LayoutRequest 只发给父控件，
    // 布局无法可靠收到通知，缓存一旦过期会让卡片被压到低于自身最小尺寸。
    QSize cellSize = m_minimumCellSize;
    for (const Item &item : m_items) {
        if (item.layoutItem) {
            cellSize = cellSize.expandedTo(item.layoutItem->minimumSize());
        }
    }
    return cellSize;
}

int DragGridLayout::minimumColumnCount() const
{
    if (m_items.isEmpty()) {
        return 0;
    }

    return qMax(1, qMin(m_columnCount, m_items.size()));
}

int DragGridLayout::rowCount() const
{
    if (m_items.isEmpty()) {
        return 0;
    }

    const int columns = effectiveColumnCount();
    if (columns <= 0) {
        return 0;
    }

    return (m_items.size() + columns - 1) / columns;
}

bool DragGridLayout::hasActivePlaceholder() const
{
    // 忽略项必须仍在布局中，且索引落在有效槽位内，否则预留槽会与真实项目错位。
    return m_ignoredWidget && indexOf(m_ignoredWidget) >= 0
            && m_placeholderIndex >= 0 && m_placeholderIndex < m_items.size();
}

int DragGridLayout::visualItemCount() const
{
    return m_items.size();
}

QSize DragGridLayout::expandedSizeForItem(const Item &item, const QSize &cellSize) const
{
    if (!item.layoutItem) {
        return cellSize;
    }

    QSize itemSize = item.layoutItem->sizeHint().boundedTo(cellSize);
    itemSize = itemSize.expandedTo(item.layoutItem->minimumSize());
    return itemSize.boundedTo(cellSize);
}

int DragGridLayout::clampedInsertIndex(int index) const
{
    return qBound(0, index, m_items.size());
}

QRect DragGridLayout::cellRect(int index, const QRect &contentRect, int columns,
                               const QSize &cellSize, int itemCount) const
{
    if (index < 0 || columns <= 0 || cellSize.isEmpty() || itemCount <= 0) {
        return QRect();
    }

    const int safeIndex = qBound(0, index, itemCount - 1);
    const int row = safeIndex / columns;
    const int rowStart = row * columns;
    const int itemsInRow = qMin(columns, itemCount - rowStart);
    const int col = safeIndex - rowStart;
    const int layoutSpacing = qMax(0, spacing());
    const bool fillRow = m_fillIncompleteRowEnabled && itemsInRow > 0 && itemsInRow < columns;

    int x = contentRect.x() + col * (cellSize.width() + layoutSpacing);
    int rowCellWidth = cellSize.width();
    if (fillRow) {
        const int availableWidth = qMax(0, contentRect.width()) - layoutSpacing * (itemsInRow - 1);
        const int baseWidth = availableWidth / itemsInRow;
        const int remainder = availableWidth % itemsInRow;
        if (baseWidth >= cellSize.width()) {
            rowCellWidth = baseWidth + (col < remainder ? 1 : 0);
            x = contentRect.x() + col * (baseWidth + layoutSpacing) + qMin(col, remainder);
        }
    }

    const int y = contentRect.y() + row * (cellSize.height() + layoutSpacing);
    return QRect(x, y, rowCellWidth, cellSize.height());
}

void DragGridLayout::setWidgetGeometryAnimated(QWidget *widget, const QRect &target)
{
    if (!widget || widget->geometry() == target) {
        return;
    }

    if (m_animationDuration <= 0) {
        stopAnimationForWidget(widget);
        widget->setGeometry(target);
        return;
    }

    // 首次布局或隐藏控件时直接定位，避免初始加载动画。
    if (!widget->isVisible() || widget->geometry().isEmpty()) {
        widget->setGeometry(target);
        return;
    }

    QPropertyAnimation *animation = m_geometryAnimations.value(widget).data();
    if (!animation) {
        animation = new QPropertyAnimation(widget, "geometry", widget);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        m_geometryAnimations[widget] = animation;
        // 结束后必须释放动画对象：只摘除表项会让它以控件为父对象长期留存，
        // 每次重排都会新增一批，长时间运行持续增长。
        const QPointer<QPropertyAnimation> guard(animation);
        connect(animation, &QPropertyAnimation::finished, this, [this, widget, guard]() {
            m_geometryAnimations.remove(widget);
            if (guard) {
                guard->deleteLater();
            }
        });
    }
    animation->setDuration(m_animationDuration);

    animation->stop();
    animation->setStartValue(widget->geometry());
    animation->setEndValue(target);
    animation->start();
}

void DragGridLayout::stopAnimationForWidget(QWidget *widget)
{
    if (!widget) {
        return;
    }

    QPointer<QPropertyAnimation> animation = m_geometryAnimations.take(widget);
    if (animation) {
        animation->stop();
        delete animation.data();
    }
}

void DragGridLayout::removeDestroyedWidget(QWidget *widget)
{
    if (!widget) {
        return;
    }

    // 此时控件处于析构中：只删除布局项、清理动画表项与忽略标记，
    // 不得调用控件任何成员函数。
    for (int index = 0; index < m_items.size(); ++index) {
        const Item &item = m_items.at(index);
        if (item.layoutItem && item.layoutItem->widget() == widget) {
            delete item.layoutItem;
            m_items.removeAt(index);
            break;
        }
    }

    // 几何动画以控件为父对象，会随控件一起销毁；这里只需清掉表项。
    m_geometryAnimations.remove(widget);

    if (m_ignoredWidget == widget) {
        // 被忽略项即占位槽的"洞"，控件没了占位含义也随之消失。
        m_ignoredWidget = nullptr;
        m_placeholderIndex = -1;
    } else if (m_placeholderIndex >= m_items.size()) {
        // 其他控件被销毁时项目数减少，占位索引必须收回到有效槽位。
        m_placeholderIndex = m_items.isEmpty() ? -1 : m_items.size() - 1;
    }

    invalidate();
}

int DragGridLayout::targetIndexAt(const QPoint &pos) const
{
    if (m_items.isEmpty()) {
        return -1;
    }

    const QMargins margins = contentsMargins();
    const QRect contentRect = geometry().adjusted(margins.left(), margins.top(),
                                                   -margins.right(), -margins.bottom());
    if (!contentRect.isValid()) {
        return -1;
    }

    const int columns = effectiveColumnCount();
    if (columns <= 0) {
        return -1;
    }

    const QSize cellSize = effectiveCellSize(contentRect);
    if (cellSize.isEmpty()) {
        return -1;
    }

    const int itemCount = visualItemCount();
    if (pos.y() < contentRect.top()) {
        return 0;
    }

    const int layoutSpacing = qMax(0, spacing());
    const int stepHeight = qMax(1, cellSize.height() + layoutSpacing);
    const int rows = qMax(1, (itemCount + columns - 1) / columns);
    const int row = qBound(0, (pos.y() - contentRect.top()) / stepHeight, rows - 1);

    return targetIndexInRow(pos, row, contentRect, columns, cellSize, itemCount);
}

int DragGridLayout::targetIndexInRow(const QPoint &pos, int row, const QRect &contentRect,
                                     int columns, const QSize &cellSize, int itemCount) const
{
    const int rowStart = row * columns;
    if (rowStart >= itemCount) {
        return itemCount - 1;
    }

    const int rowEnd = qMin(itemCount - 1, rowStart + columns - 1);
    for (int index = rowStart; index <= rowEnd; ++index) {
        const QRect cell = cellRect(index, contentRect, columns, cellSize, itemCount);
        if (!cell.isValid()) {
            continue;
        }

        if (pos.x() < cell.left()) {
            return index;
        }
        if (pos.x() <= cell.right()) {
            if (columns == 1) {
                return (pos.y() - cell.top()) * 2 >= cell.height()
                        ? qMin(index + 1, itemCount - 1)
                        : index;
            }
            return (pos.x() - cell.left()) * 2 >= cell.width()
                    ? qMin(index + 1, itemCount - 1)
                    : index;
        }
    }

    return qMin(rowEnd + 1, itemCount - 1);
}

QRect DragGridLayout::placeholderRectAt(int placeholderIndex) const
{
    const QMargins margins = contentsMargins();
    const QRect contentRect = geometry().adjusted(margins.left(), margins.top(),
                                                  -margins.right(), -margins.bottom());
    return cellRectForIndex(placeholderIndex, contentRect);
}

} // namespace YJDragGrid


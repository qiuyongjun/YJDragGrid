#include "YJGridLayout.h"

#include <QLayoutItem>
#include <QMargins>
#include <QStyle>
#include <QWidget>
#include <QWidgetItem>
#include <QtGlobal>

YJGridLayout::YJGridLayout(QWidget *parent)
    : QLayout(parent)
{
}

YJGridLayout::~YJGridLayout()
{
    QLayoutItem *item = nullptr;
    while ((item = takeAt(0)) != nullptr) {
        delete item;
    }
}

void YJGridLayout::addItem(QLayoutItem *item)
{
    if (!item) {
        return;
    }

    m_items.append({item, {}});
    invalidate();
}

void YJGridLayout::addWidget(QWidget *widget, Qt::Alignment alignment)
{
    insertWidget(m_items.size(), widget, alignment);
}

void YJGridLayout::insertWidget(int index, QWidget *widget, Qt::Alignment alignment)
{
    if (!widget || indexOf(widget) != -1) {
        return;
    }

    addChildWidget(widget);
    auto *item = new QWidgetItem(widget);
    m_items.insert(clampedInsertIndex(index), {item, alignment});
    invalidate();
}

QLayoutItem *YJGridLayout::itemAt(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }

    return m_items[index].layoutItem;
}

QLayoutItem *YJGridLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }

    Item item = m_items.takeAt(index);
    invalidate();
    return item.layoutItem;
}

int YJGridLayout::count() const
{
    return m_items.size();
}

void YJGridLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);

    if (m_items.isEmpty()) {
        return;
    }

    const QMargins margins = contentsMargins();
    const QRect contentRect = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    const QSize cellSize = effectiveCellSize(contentRect);

    for (int index = 0; index < m_items.size(); ++index) {
        QLayoutItem *item = m_items[index].layoutItem;
        if (!item) {
            continue;
        }
        if (item->widget() == m_ignoredWidget) {
            continue;
        }

        QRect cellRect = cellRectForIndex(index, contentRect);
        QRect itemRect = cellRect;
        if (m_items[index].alignment) {
            itemRect.setSize(expandedSizeForItem(m_items[index], cellSize));
            itemRect = QStyle::alignedRect(Qt::LeftToRight,
                                           m_items[index].alignment,
                                           itemRect.size(),
                                           cellRect);
        }

        item->setGeometry(itemRect);
    }
}

QSize YJGridLayout::sizeHint() const
{
    return minimumSize();
}

QSize YJGridLayout::minimumSize() const
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

bool YJGridLayout::hasHeightForWidth() const
{
    return true;
}

int YJGridLayout::heightForWidth(int width) const
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

Qt::Orientations YJGridLayout::expandingDirections() const
{
    return Qt::Horizontal;
}

int YJGridLayout::columnCount() const
{
    return m_columnCount;
}

void YJGridLayout::setColumnCount(int columnCount)
{
    const int safeColumnCount = qMax(1, columnCount);
    if (m_columnCount == safeColumnCount) {
        return;
    }

    m_columnCount = safeColumnCount;
    invalidate();
}

QSize YJGridLayout::minimumCellSize() const
{
    return m_minimumCellSize;
}

void YJGridLayout::setMinimumCellSize(const QSize &size)
{
    const QSize safeSize(qMax(1, size.width()), qMax(1, size.height()));
    if (m_minimumCellSize == safeSize) {
        return;
    }

    m_minimumCellSize = safeSize;
    invalidate();
}

bool YJGridLayout::equalCellSizeEnabled() const
{
    return m_equalCellSizeEnabled;
}

void YJGridLayout::setEqualCellSizeEnabled(bool enable)
{
    if (m_equalCellSizeEnabled == enable) {
        return;
    }

    m_equalCellSizeEnabled = enable;
    invalidate();
}

bool YJGridLayout::compactWhenSparseEnabled() const
{
    return m_compactWhenSparseEnabled;
}

void YJGridLayout::setCompactWhenSparseEnabled(bool enable)
{
    if (m_compactWhenSparseEnabled == enable) {
        return;
    }

    m_compactWhenSparseEnabled = enable;
    invalidate();
}

QList<QWidget *> YJGridLayout::widgets() const
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

int YJGridLayout::indexOf(QWidget *widget) const
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

QWidget *YJGridLayout::takeWidget(int index)
{
    QLayoutItem *item = takeAt(index);
    if (!item) {
        return nullptr;
    }

    QWidget *widget = item->widget();
    delete item;
    return widget;
}

bool YJGridLayout::moveItem(int from, int to)
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

QRect YJGridLayout::cellRectForIndex(int index, const QRect &contentRect) const
{
    if (index < 0 || m_items.isEmpty()) {
        return QRect();
    }

    const int columns = effectiveColumnCount();
    if (columns <= 0) {
        return QRect();
    }

    const QSize cellSize = effectiveCellSize(contentRect);
    const int col = index % columns;
    const int row = index / columns;
    const int layoutSpacing = qMax(0, spacing());
    const int x = contentRect.x() + col * (cellSize.width() + layoutSpacing);
    const int y = contentRect.y() + row * (cellSize.height() + layoutSpacing);
    return QRect(x, y, cellSize.width(), cellSize.height());
}

int YJGridLayout::effectiveColumnCount() const
{
    if (m_items.isEmpty()) {
        return 0;
    }

    if (m_compactWhenSparseEnabled) {
        return qMax(1, qMin(m_columnCount, m_items.size()));
    }

    return qMax(1, m_columnCount);
}

QWidget *YJGridLayout::ignoredWidget() const
{
    return m_ignoredWidget;
}

void YJGridLayout::setIgnoredWidget(QWidget *widget)
{
    if (m_ignoredWidget == widget) {
        return;
    }

    m_ignoredWidget = widget;
    invalidate();
}

QSize YJGridLayout::effectiveCellSize(const QRect &contentRect) const
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

QSize YJGridLayout::minimumCellSizeForItems() const
{
    QSize cellSize = m_minimumCellSize;
    for (const Item &item : m_items) {
        if (item.layoutItem) {
            cellSize = cellSize.expandedTo(item.layoutItem->minimumSize());
        }
    }
    return cellSize;
}

int YJGridLayout::minimumColumnCount() const
{
    if (m_items.isEmpty()) {
        return 0;
    }

    return qMax(1, qMin(m_columnCount, m_items.size()));
}

int YJGridLayout::rowCount() const
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

QSize YJGridLayout::expandedSizeForItem(const Item &item, const QSize &cellSize) const
{
    if (!item.layoutItem) {
        return cellSize;
    }

    QSize itemSize = item.layoutItem->sizeHint().boundedTo(cellSize);
    itemSize = itemSize.expandedTo(item.layoutItem->minimumSize());
    return itemSize.boundedTo(cellSize);
}

int YJGridLayout::clampedInsertIndex(int index) const
{
    return qBound(0, index, m_items.size());
}

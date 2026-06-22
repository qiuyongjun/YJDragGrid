#ifndef DRAGGRIDLAYOUT_H
#define DRAGGRIDLAYOUT_H

#include <QList>
#include <QLayout>
#include <QSize>

class QWidget;

class DragGridLayout : public QLayout
{
    Q_DISABLE_COPY(DragGridLayout)

public:
    explicit DragGridLayout(QWidget *parent = nullptr);
    ~DragGridLayout() override;

    // 添加布局项，所有权转移给布局。
    void addItem(QLayoutItem *item) override;
    void addWidget(QWidget *widget, Qt::Alignment alignment = {});
    void insertWidget(int index, QWidget *widget, Qt::Alignment alignment = {});

    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    int count() const override;

    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    Qt::Orientations expandingDirections() const override;

    int columnCount() const;
    void setColumnCount(int columnCount);

    QSize minimumCellSize() const;
    void setMinimumCellSize(const QSize &size);

    bool equalCellSizeEnabled() const;
    void setEqualCellSizeEnabled(bool enable);

    bool compactWhenSparseEnabled() const;
    void setCompactWhenSparseEnabled(bool enable);
    int effectiveColumnCount() const;

    QWidget *ignoredWidget() const;
    void setIgnoredWidget(QWidget *widget);

    int placeholderIndex() const;
    void setPlaceholderIndex(int index);

    QList<QWidget *> widgets() const;
    int indexOf(const QWidget *widget) const;
    QWidget *takeWidget(int index);
    bool moveItem(int from, int to);
    QRect cellRectForIndex(int index, const QRect &contentsRect) const;

private:
    struct Item {
        QLayoutItem *layoutItem = nullptr;
        Qt::Alignment alignment;
    };

    void invalidate() override;

    QSize effectiveCellSize(const QRect &contentRect) const;
    QSize minimumCellSizeForItems() const;
    int minimumColumnCount() const;
    int rowCount() const;
    QSize expandedSizeForItem(const Item &item, const QSize &cellSize) const;
    int clampedInsertIndex(int index) const;

    QRect cellRect(int index, const QRect &contentRect, int columns, const QSize &cellSize) const;

private:
    QList<Item> m_items;
    int m_columnCount = 4;
    QSize m_minimumCellSize = QSize(400, 300);
    bool m_equalCellSizeEnabled = true;
    bool m_compactWhenSparseEnabled = false;
    QWidget *m_ignoredWidget = nullptr;
    int m_placeholderIndex = -1;

    // 缓存：invalidate 时失效，惰性计算
    mutable QSize m_cachedMinCellSize;
    mutable bool m_minCellSizeDirty = true;
};

#endif // DRAGGRIDLAYOUT_H

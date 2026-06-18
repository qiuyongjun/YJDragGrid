#ifndef YJGRIDLAYOUT_H
#define YJGRIDLAYOUT_H

#include <QList>
#include <QLayout>
#include <QSize>

class QWidget;

class YJGridLayout : public QLayout
{
public:
    explicit YJGridLayout(QWidget *parent = nullptr);
    ~YJGridLayout() override;

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

    QList<QWidget *> widgets() const;
    int indexOf(QWidget *widget) const;
    QWidget *takeWidget(int index);
    bool moveItem(int from, int to);
    QRect cellRectForIndex(int index, const QRect &contentsRect) const;

private:
    struct Item {
        QLayoutItem *layoutItem = nullptr;
        Qt::Alignment alignment;
    };

    QSize effectiveCellSize(const QRect &contentRect) const;
    QSize minimumCellSizeForItems() const;
    int minimumColumnCount() const;
    int rowCount() const;
    QSize expandedSizeForItem(const Item &item, const QSize &cellSize) const;
    int clampedInsertIndex(int index) const;

private:
    QList<Item> m_items;
    int m_columnCount = 4;
    QSize m_minimumCellSize = QSize(400, 300);
    bool m_equalCellSizeEnabled = true;
    bool m_compactWhenSparseEnabled = false;
    QWidget *m_ignoredWidget = nullptr;
};

#endif // YJGRIDLAYOUT_H

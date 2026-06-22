#ifndef DRAGGRIDWIDGET_H
#define DRAGGRIDWIDGET_H

#include <QList>
#include <QSize>
#include <QWidget>

class QMouseEvent;
class QScrollArea;
class QTimer;
class DragGridLayout;

class GridDragController;

class DragGridWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DragGridWidget(QScrollArea *scrollArea = nullptr, QWidget *parent = nullptr);
    ~DragGridWidget() override = default;

    // 添加控件并纳入网格布局管理。
    void addWidget(QWidget *widget);
    // 按指定顺序插入控件。
    void insertWidget(int index, QWidget *widget);
    // 从网格移除控件，不销毁对象。
    void removeWidget(QWidget *widget);
    // 从网格移除控件并延迟销毁。
    void deleteWidget(QWidget *widget);
    // 按索引取出控件，不销毁对象。
    QWidget *takeWidget(int index);
    // 清空并销毁所有控件。
    void clear();

    int columnCount() const;
    void setColumnCount(int columnCount);
    int count() const;
    QList<QWidget *> widgets() const;

    QSize minimumCellSize() const;
    void setMinimumCellSize(const QSize &size);

    void setDragEnabled(bool enable);
    bool dragEnabled() const;

    void setEqualCellSizeEnabled(bool enable);
    bool equalCellSizeEnabled() const;
    void setCompactWhenSparseEnabled(bool enable);
    bool compactWhenSparseEnabled() const;

signals:
    void orderChanged();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void slot_scrollTimer_timeOut();

private:
    int indexOfWidget(const QWidget *widget) const;
    int placeholderIndexAt(const QPoint &pos) const;
    bool reorderForPlaceholder();
    void finishDrag();
    void updatePlaceholder();
    QRect layoutContentsRect() const;

private:
    DragGridLayout *m_gridLayout = nullptr;
    QWidget *m_placeholderWidget = nullptr;
    QTimer *m_scrollTimer = nullptr;
    GridDragController *m_dragController = nullptr;

    bool m_dragEnable = false;
};

#endif // DRAGGRIDWIDGET_H

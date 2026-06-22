#ifndef DRAGGRIDWIDGET_H
#define DRAGGRIDWIDGET_H

#include <QList>
#include <QSize>
#include <QWidget>

class QLabel;
class QMouseEvent;
class QPropertyAnimation;
class QResizeEvent;
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
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void slot_scrollTimer_timeOut();

private:
    enum class DragState { Idle, Pressed, Dragging };

    int indexOfWidget(const QWidget *widget) const;
    bool reorderForPlaceholder();
    void startDragOperation(QWidget *widget, const QPoint &offset);
    void finishDrag();
    void updatePlaceholder();
    void autoScroll() const;

    void createDragGhost(QWidget *source);
    void updateDragGhostPosition(const QPoint &cursorPos);
    void clearDragGhost();

    void updateEmptyState();

private:
    DragGridLayout *m_gridLayout = nullptr;
    QWidget *m_placeholderWidget = nullptr;
    QTimer *m_scrollTimer = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    GridDragController *m_dragController = nullptr;

    DragState m_dragState = DragState::Idle;
    QWidget *m_pressWidget = nullptr;
    QPoint m_pressPos;
    QPoint m_pressOffset;

    QWidget *m_dragGhostWidget = nullptr;
    QLabel  *m_emptyStateLabel = nullptr;
    QPropertyAnimation *m_placeholderPulseAnimation = nullptr;

    bool m_dragEnable = false;
    static constexpr int kDragThreshold = 6;
    static constexpr qreal kGhostScale = 1.05;
};

#endif // DRAGGRIDWIDGET_H

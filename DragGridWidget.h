#ifndef DRAGGRIDWIDGET_H
#define DRAGGRIDWIDGET_H

#include <QList>
#include <QPointer>
#include <QSize>
#include <QWidget>

class QLabel;
class QKeyEvent;
class QMouseEvent;
class QPropertyAnimation;
class QResizeEvent;
class QScrollArea;
class QTimer;
class DragGridLayout;

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

    int dragThreshold() const;
    void setDragThreshold(int threshold);

    qreal ghostScale() const;
    void setGhostScale(qreal scale);

    QWidget *dragHandle() const;
    void setDragHandle(QWidget *handle);

    int scrollTimerInterval() const;
    void setScrollTimerInterval(int ms);

    int animationDuration() const;
    void setAnimationDuration(int ms);

    int autoScrollMargin() const;
    void setAutoScrollMargin(int margin);

    int autoScrollMaxSpeed() const;
    void setAutoScrollMaxSpeed(int speed);

    qreal placeholderOpacity() const;
    void setPlaceholderOpacity(qreal opacity);

    int placeholderPulseDuration() const;
    void setPlaceholderPulseDuration(int ms);

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
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void slot_scrollTimer_timeOut();

private:
    enum class DragState { Idle, Pressed, Dragging };

    int indexOfWidget(const QWidget *widget) const;
    bool reorderForPlaceholder();
    void startDragOperation(QWidget *widget, const QPoint &offset, bool grabMouse = true);
    bool finishDrag();
    void cancelDrag();
    void cleanupDragUi();
    void updatePlaceholder();
    void autoScroll();
    void ensurePlaceholderVisible();

    void startKeyboardDrag();
    void movePlaceholderByKey(int key);

    bool isUnderDragHandle(QWidget *widget) const;

    void createDragGhost(QWidget *source);
    void updateDragGhostPosition(const QPoint &cursorPos);
    void clearDragGhost();

    void updateEmptyState();

private:
    DragGridLayout *m_gridLayout = nullptr;
    QWidget *m_placeholderWidget = nullptr;
    QTimer *m_scrollTimer = nullptr;
    QScrollArea *m_scrollArea = nullptr;

    DragState m_dragState = DragState::Idle;
    QWidget *m_pressWidget = nullptr;
    QWidget *m_draggedWidget = nullptr;
    QPoint m_pressPos;
    QPoint m_pressOffset;
    QPoint m_dragPointOffset;
    QPoint m_lastMousePos;
    int m_keyboardDragStartIndex = -1;

    QWidget *m_dragGhostWidget = nullptr;
    QLabel  *m_emptyStateLabel = nullptr;
    QPropertyAnimation *m_placeholderPulseAnimation = nullptr;

    bool m_dragEnable = false;
    int m_dragThreshold = 6;
    qreal m_ghostScale = 1.05;
    QPointer<QWidget> m_dragHandle;
    int m_scrollTimerInterval = 16;
    int m_animationDuration = 200;

    int m_autoScrollMargin = 40;
    int m_autoScrollMaxSpeed = 16;
    qreal m_placeholderOpacity = 0.5;
    int m_placeholderPulseDuration = 800;
};

#endif // DRAGGRIDWIDGET_H

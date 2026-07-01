#ifndef DRAGGRIDWIDGET_H
#define DRAGGRIDWIDGET_H

#include <QList>
#include <QPointer>
#include <QSize>
#include <QWidget>

#include <QtDragGrid/QtDragGridGlobal.h>

class QLabel;
class QKeyEvent;
class QMouseEvent;
class QPropertyAnimation;
class QResizeEvent;
class QScrollArea;
class QTimer;

namespace QtDragGrid {

class DragGridLayout;

class QTDRAGGRID_EXPORT DragGridWidget : public QWidget
{
    Q_OBJECT

public:
    // 创建拖拽网格容器；传入滚动区域后可启用拖拽边缘自动滚动。
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

    // 返回当前布局列数。
    int columnCount() const;
    // 设置布局列数，非法值会被限制为至少 1 列。
    void setColumnCount(int columnCount);
    // 返回网格中受布局管理的控件数量。
    int count() const;
    // 按当前顺序返回所有子控件。
    QList<QWidget *> widgets() const;

    // 返回单元格最小尺寸。
    QSize minimumCellSize() const;
    // 设置单元格最小尺寸，避免控件被压缩到不可用大小。
    void setMinimumCellSize(const QSize &size);

    // 启用或禁用拖拽；禁用时会取消正在进行的拖拽。
    void setDragEnabled(bool enable);
    // 返回是否允许拖拽重排。
    bool dragEnabled() const;

    // 返回触发拖拽所需的鼠标移动阈值。
    int dragThreshold() const;
    // 设置拖拽触发阈值，负数会按 0 处理。
    void setDragThreshold(int threshold);

    // 返回拖拽镜像缩放比例。
    qreal ghostScale() const;
    // 设置拖拽镜像缩放比例，最小为 1.0。
    void setGhostScale(qreal scale);

    // 返回当前拖拽手柄控件。
    QWidget *dragHandle() const;
    // 设置拖拽手柄；仅手柄自身或其子控件可启动拖拽。
    void setDragHandle(QWidget *handle);

    // 返回自动滚动定时器间隔。
    int scrollTimerInterval() const;
    // 设置自动滚动定时器间隔，单位毫秒。
    void setScrollTimerInterval(int ms);

    // 返回拖拽落位和布局过渡动画时长。
    int animationDuration() const;
    // 设置拖拽落位和布局过渡动画时长，单位毫秒。
    void setAnimationDuration(int ms);

    // 返回触发自动滚动的视口边缘距离。
    int autoScrollMargin() const;
    // 设置触发自动滚动的视口边缘距离。
    void setAutoScrollMargin(int margin);

    // 返回自动滚动最大步进速度。
    int autoScrollMaxSpeed() const;
    // 设置自动滚动最大步进速度，单位像素。
    void setAutoScrollMaxSpeed(int speed);

    // 返回拖拽占位符透明度。
    qreal placeholderOpacity() const;
    // 设置拖拽占位符透明度，取值会限制在 0.0 到 1.0。
    void setPlaceholderOpacity(qreal opacity);

    // 返回占位符脉冲动画周期。
    int placeholderPulseDuration() const;
    // 设置占位符脉冲动画周期，单位毫秒。
    void setPlaceholderPulseDuration(int ms);

    // 设置是否让所有单元格使用统一宽度。
    void setEqualCellSizeEnabled(bool enable);
    // 返回是否启用统一单元格宽度。
    bool equalCellSizeEnabled() const;
    // 设置不完整行是否按剩余项目数填满整行。
    void setFillIncompleteRowEnabled(bool enable);
    // 返回是否启用不完整行填满整行。
    bool fillIncompleteRowEnabled() const;
    // 设置不完整行是否填满整行，兼容旧接口。
    void setCompactWhenSparseEnabled(bool enable);
    // 返回是否启用不完整行填满整行，兼容旧接口。
    bool compactWhenSparseEnabled() const;

    // 返回空状态提示文本。
    QString emptyText() const;
    // 设置网格为空时显示的提示文本。
    void setEmptyText(const QString &text);

    // 返回空状态提示是否可见。
    bool emptyStateVisible() const;
    // 设置网格为空时是否显示提示。
    void setEmptyStateVisible(bool visible);

signals:
    // 用户通过拖拽改变控件顺序后发出。
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
    QPointer<QScrollArea> m_scrollArea;

    DragState m_dragState = DragState::Idle;
    QWidget *m_pressWidget = nullptr;
    QWidget *m_draggedWidget = nullptr;
    QPoint m_pressPos;
    QPoint m_pressOffset;
    QPoint m_dragPointOffset;
    QPoint m_dragGhostCursorOffset;
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

    QString m_emptyText = QStringLiteral("No items.");
    bool m_emptyStateVisible = true;
};

} // namespace QtDragGrid

#endif // DRAGGRIDWIDGET_H

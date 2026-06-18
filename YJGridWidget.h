#ifndef YJGRIDWIDGET_H
#define YJGRIDWIDGET_H

#include <QList>
#include <QSize>
#include <QWidget>

class GridDragController;
class QMouseEvent;
class QScrollArea;
class QTimer;
class YJGridLayout;

class YJGridWidget : public QWidget
{
    Q_OBJECT

public:
    explicit YJGridWidget(QScrollArea *scrollArea = nullptr, QWidget *parent = nullptr);
    ~YJGridWidget() override;

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

    // 兼容旧接口，建议使用 columnCount()。
    int getColumnMaxNum() const;
    int columnCount() const;
    int count() const;
    QList<QWidget *> widgets() const;
    // 兼容旧接口，建议使用 setColumnCount()。
    void setColumnMaxNum(int columnMaxNum);
    void setColumnCount(int columnCount);

    QSize minimumCellSize() const;
    void setMinimumCellSize(const QSize &size);

    // 兼容旧接口，建议使用 minimumCellSize()。
    int getCellMiniWidth() const;
    int minimumCellWidth() const;
    void setCellMiniWidth(int cellMiniWidth);
    void setMinimumCellWidth(int cellMiniWidth);

    // 兼容旧接口，建议使用 minimumCellSize()。
    int getCellMiniHeight() const;
    int minimumCellHeight() const;
    void setCellMiniHeight(int cellMiniHeight);
    void setMinimumCellHeight(int cellMiniHeight);

    void setDragEnabled(bool enable);
    bool dragEnabled() const;

    void setEqualCellSizeEnabled(bool enable);
    bool equalCellSizeEnabled() const;
    // 兼容旧接口。
    void setExpandedCellEnabled(bool enable);
    bool expandedCellEnabled() const;

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
    bool reorderForPlaceholder();
    void finishDrag();
    void updatePlaceholder();
    QRect layoutContentsRect() const;

private:
    YJGridLayout *m_gridLayout = nullptr;
    QWidget *m_placeholderWidget = nullptr;
    QTimer *m_scrollTimer = nullptr;
    GridDragController *m_dragController = nullptr;

    bool m_dragEnable = false;
};

#endif // YJGRIDWIDGET_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

namespace QtDragGrid {
class DragGridWidget;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 创建拖拽网格演示窗口。
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 添加一张演示卡片。
    void on_pushButton_addView_clicked();
    // 清空所有演示卡片，展示空状态。
    void on_pushButton_clearView_clicked();

    // 响应卡片关闭请求并从网格中删除卡片。
    void slot_viewRemove();
    // 拖拽顺序变化后刷新示例编号。
    void slot_orderChanged();

private:
    // 初始化示例控制面板并同步默认配置。
    void setupDemoControls();
    // 批量创建初始卡片，便于打开示例后立即观察布局效果。
    void populateInitialCards(int count);
    // 创建并添加一张演示卡片。
    void addDemoCard();
    // 根据当前顺序刷新示例标题，便于肉眼验证拖拽和插入结果。
    void refreshWidgetLabels();

    Ui::MainWindow *ui;

    QtDragGrid::DragGridWidget *m_customDraggableGrid = nullptr;
};
#endif // MAINWINDOW_H

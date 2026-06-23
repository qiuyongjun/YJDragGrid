#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class DragGridWidget;

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

    // 响应卡片关闭请求并从网格中删除卡片。
    void slot_viewRemove();
    // 拖拽顺序变化后刷新示例编号。
    void slot_orderChanged();

private:
    // 根据当前顺序刷新示例标题，便于肉眼验证拖拽和插入结果。
    void refreshWidgetLabels();

    Ui::MainWindow *ui;

    DragGridWidget * m_customDraggableGrid;
};
#endif // MAINWINDOW_H

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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_addView_clicked();

    void slot_viewRemove();
    void slot_orderChanged();

private:
    // 根据当前顺序刷新示例标题，便于肉眼验证拖拽和插入结果。
    void refreshWidgetLabels();

    Ui::MainWindow *ui;

    DragGridWidget * m_customDraggableGrid;
};
#endif // MAINWINDOW_H

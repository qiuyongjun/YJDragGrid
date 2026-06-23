#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QWidget>

namespace Ui {
class CardWidget;
}

class CardWidget : public QWidget
{
    Q_OBJECT

public:
    // 创建演示卡片控件。
    explicit CardWidget(QWidget *parent = nullptr);
    ~CardWidget();

    // 设置卡片中间显示的文本。
    void setLabel(const QString &text);

signals:
    // 用户点击关闭按钮时发出，由外层容器决定如何移除卡片。
    void signal_closeWidget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::CardWidget *ui;
};

#endif // CARDWIDGET_H

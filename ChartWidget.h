#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>

namespace Ui {
class ChartWidget;
}

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(QWidget *parent = nullptr);
    ~ChartWidget();

public:
    void setLabel(const QString &text);
    // 兼容旧接口，新代码请使用 setLabel。
    void setLable(const QString &text);
signals:
    void signal_closeWidget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::ChartWidget *ui;
};

#endif // CHARTWIDGET_H

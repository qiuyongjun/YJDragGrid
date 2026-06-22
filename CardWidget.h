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
    explicit CardWidget(QWidget *parent = nullptr);
    ~CardWidget();

    void setLabel(const QString &text);

signals:
    void signal_closeWidget();

private slots:
    void on_pushButton_clicked();

private:
    Ui::CardWidget *ui;
};

#endif // CARDWIDGET_H

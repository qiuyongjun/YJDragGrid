#include "ChartWidget.h"
#include "ui_ChartWidget.h"

ChartWidget::ChartWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ChartWidget)
{
    ui->setupUi(this);
}

ChartWidget::~ChartWidget()
{
    delete ui;
}

void ChartWidget::setLabel(const QString &text)
{
    ui->label->setText(text);
}

void ChartWidget::setLable(const QString &text)
{
    setLabel(text);
}

void ChartWidget::on_pushButton_clicked()
{
    emit signal_closeWidget();
    //deleteLater();
}




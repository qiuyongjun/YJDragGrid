#include "CardWidget.h"
#include "ui_CardWidget.h"

CardWidget::CardWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CardWidget)
{
    ui->setupUi(this);

    setAttribute(Qt::WA_StyledBackground, true);
}

CardWidget::~CardWidget()
{
    delete ui;
}

void CardWidget::setLabel(const QString &text)
{
    ui->label->setText(text);
}

void CardWidget::on_pushButton_clicked()
{
    emit signal_closeWidget();
}

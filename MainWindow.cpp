#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "CardWidget.h"
#include "DragGridWidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_customDraggableGrid = new DragGridWidget(ui->scrollArea_2,this);
    m_customDraggableGrid->setDragEnabled(true);
    m_customDraggableGrid->setColumnCount(4);
    connect(m_customDraggableGrid, &DragGridWidget::orderChanged, this, &MainWindow::slot_orderChanged);
    ui->gridLayout->addWidget(m_customDraggableGrid);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_addView_clicked()
{
    CardWidget *cardWidget = new CardWidget(this);
    connect(cardWidget, &CardWidget::signal_closeWidget, this, &MainWindow::slot_viewRemove);

    m_customDraggableGrid->addWidget(cardWidget);
    refreshWidgetLabels();
}

void MainWindow::slot_viewRemove()
{
    auto *cardWidget = qobject_cast<CardWidget*>(sender());
    if (!cardWidget || !m_customDraggableGrid) {
        return;
    }

    m_customDraggableGrid->deleteWidget(cardWidget);
    refreshWidgetLabels();
}

void MainWindow::slot_orderChanged()
{
    refreshWidgetLabels();
}

void MainWindow::refreshWidgetLabels()
{
    if (!m_customDraggableGrid) {
        return;
    }

    const QList<QWidget *> orderedWidgets = m_customDraggableGrid->widgets();
    for (int index = 0; index < orderedWidgets.size(); ++index) {
        auto *cardWidget = qobject_cast<CardWidget *>(orderedWidgets[index]);
        if (!cardWidget) {
            continue;
        }

        cardWidget->setLabel(QString("%1 / %2")
                                  .arg(index + 1)
                                  .arg(m_customDraggableGrid->count()));
    }
}


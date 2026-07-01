#include <QtDragGrid/DragGridWidget.h>

#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QtDragGrid::DragGridWidget grid;
    grid.setColumnCount(2);
    return grid.columnCount() == 2 ? 0 : 1;
}

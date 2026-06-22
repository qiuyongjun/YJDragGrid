#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>

/**
 * @brief 从可执行文件所在目录加载外部 QSS 样式表。
 * @param app 当前 QApplication 实例。
 */
static void loadStyleSheet(QApplication *app)
{
    const QString qssPath = QDir(QCoreApplication::applicationDirPath()).filePath("style.qss");
    QFile file(qssPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open stylesheet:" << qssPath;
        return;
    }

    app->setStyleSheet(QString::fromUtf8(file.readAll()));
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    loadStyleSheet(&a);

    MainWindow w;
    w.show();
    return a.exec();
}

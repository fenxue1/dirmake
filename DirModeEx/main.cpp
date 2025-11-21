/**
 * @file main.cpp
 * @brief 应用入口（Application entry point）
 *
 * 使用示例：命令行启动后创建并显示主窗口。
 */
#include "mainwindow.h"

#include <QApplication>

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QItemSelectionModel>


#include <QDirModel>
#include <QTreeView>
#include <QListView>
#include <QTableView>
#include <QSplitter>
#include <QObject>
int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

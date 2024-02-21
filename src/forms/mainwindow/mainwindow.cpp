#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDebug>

extern "C" {
#include <libavcodec/avcodec.h>
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    auto avversion = av_version_info();
    qDebug()<<avversion;
}

MainWindow::~MainWindow()
{
    delete ui;
}

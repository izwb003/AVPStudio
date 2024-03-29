/*
 * Copyright (C) 2024 Steven Song (izwb003)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include "aboutwindow/aboutwindow.h"
#include "pagewelcome.h"
#include "pagecreate.h"
#include "pageedit.h"
#include "pageprocess.h"
#include "pagecompleted.h"

#include "settings.h"

#include <QDebug>
#include <QDesktopServices>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(windowFlags()& ~Qt::WindowMaximizeButtonHint);
    this->setFixedSize(this->width(), this->height());
    this->setWindowTitle("AVPStudio - Welcome");

    PageWelcome *pageWelcome = new PageWelcome(this);
    ui->stackedWidget->addWidget(pageWelcome);
    PageCreate *pageCreate = new PageCreate(this);
    ui->stackedWidget->addWidget(pageCreate);
    PageEdit *pageEdit = new PageEdit(this);
    ui->stackedWidget->addWidget(pageEdit);
    PageProcess *pageProcess = new PageProcess(this);
    ui->stackedWidget->addWidget(pageProcess);
    PageCompleted *pageCompleted = new PageCompleted(this);
    ui->stackedWidget->addWidget(pageCompleted);
    ui->stackedWidget->setCurrentIndex(0);

    connect(pageWelcome, SIGNAL(createContent()), this, SLOT(do_createContent()));
    connect(pageWelcome, SIGNAL(createContent()), pageCreate, SLOT(do_init()));
    connect(pageCreate, SIGNAL(editContent()), this, SLOT(do_editContent()));
    connect(pageCreate, SIGNAL(editContent()), pageEdit, SLOT(do_init()));
    connect(pageEdit, SIGNAL(toProcess()), this, SLOT(do_toProcess()));
    connect(pageProcess, SIGNAL(reInit()), this, SLOT(on_actionNewContent_triggered()));
    connect(pageCompleted, SIGNAL(reInit()), this, SLOT(on_actionNewContent_triggered()));
    connect(this, SIGNAL(toProcess()), pageProcess, SLOT(do_proc()));
    connect(this, SIGNAL(openFile(QString)), pageCreate, SLOT(on_labelDragText_linkActivated(QString)));

    checkUpdate();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::checkUpdate()
{
    updateChecker = new QNetworkAccessManager(this);
    connect(updateChecker, SIGNAL(finished(QNetworkReply*)), this, SLOT(do_checkUpdateFinished(QNetworkReply*)));

    updateChecker->get(QNetworkRequest(QUrl("https://api.github.com/repos/izwb003/AVPStudio/releases/latest")));
}

void MainWindow::do_createContent()
{
    this->setWindowTitle("AVPStudio - Create (" + settings.getSizeString() + ")");
    ui->stackedWidget->setCurrentIndex(1);
    ui->actionOpenFile->setEnabled(true);
}

void MainWindow::do_editContent()
{
    ui->stackedWidget->setCurrentIndex(2);
}

void MainWindow::do_toProcess()
{
    ui->stackedWidget->setCurrentIndex(3);
    this->setWindowTitle("AVPStudio - Processing");
    ui->actionNewContent->setEnabled(false);
    ui->actionOpenFile->setEnabled(false);
    emit toProcess();
}

void MainWindow::do_toCompleted(bool isError, QString errorStr)
{
    PageCompleted *pageCompleted = qobject_cast<PageCompleted*>(ui->stackedWidget->widget(4));
    pageCompleted->setStatus(isError, errorStr);
    ui->stackedWidget->setCurrentIndex(4);
}

bool compareVersion(QString newVersionStr)
{
    static const QRegularExpression newVersionRegex("v(\\d+)\\.(\\d+)\\.(\\d+)");
    QRegularExpressionMatch newVersionMatch = newVersionRegex.match(newVersionStr);
    if(newVersionMatch.hasMatch())
    {
        int newMajor = newVersionMatch.captured(1).toInt();
        int newMinor = newVersionMatch.captured(2).toInt();
        int newPatch = newVersionMatch.captured(3).toInt();

        if(newMajor > PROJECT_VERSION_MAJOR)
            return true;
        else if(newMajor == PROJECT_VERSION_MAJOR)
        {
            if(newMinor > PROJECT_VERSION_MINOR)
                return true;
            else if(newMinor == PROJECT_VERSION_MINOR)
                return newPatch > PROJECT_VERSION_PATCH;
        }
    }
    return false;
}

void MainWindow::do_checkUpdateFinished(QNetworkReply *reply)
{
    if(reply->error() == QNetworkReply::NoError)
    {
        QString newVersionStr;
        QString newVersionUrl;

        QByteArray data = reply->readAll();

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(!jsonDoc.isObject())
            return;
        QJsonObject jsonObj = jsonDoc.object();

        QStringList jsonKeys = jsonObj.keys();
        for(QString jsonKey : jsonKeys)
        {
            if(jsonKey == "tag_name")
                newVersionStr = jsonObj.value(jsonKey).toString();
            if(jsonKey == "html_url")
                newVersionUrl = jsonObj.value(jsonKey).toString();
        }

        if(compareVersion(newVersionStr))
        {
            if(QMessageBox::question(this, tr("版本更新"), tr("有新版本的AVPStudio可用。要下载吗？")) == QMessageBox::Yes)
                QDesktopServices::openUrl(QUrl(newVersionUrl));
        }
    }
}

void MainWindow::on_actionAbout_triggered()
{
    AboutWindow *aboutWindow = new AboutWindow();
    aboutWindow->setAttribute(Qt::WA_DeleteOnClose);
    aboutWindow->show();
}


void MainWindow::on_actionExit_triggered()
{
    qApp->quit();
}


void MainWindow::on_actionNewContent_triggered()
{
    setWindowTitle("AVPStudio - Welcome");
    ui->stackedWidget->setCurrentIndex(0);
    ui->actionNewContent->setEnabled(true);
    ui->actionOpenFile->setEnabled(false);
}


void MainWindow::on_actionOpenFile_triggered()
{
    emit openFile("");
}


void MainWindow::on_actionWavGenerator_triggered()
{
    QProcess *wavGeneratorProcess = new QProcess(this);
    wavGeneratorProcess -> startDetached("wavgenerator");
    delete wavGeneratorProcess;
}

void MainWindow::on_actionImageOrganizer_triggered()
{
    QProcess *imageOrganizerProcess = new QProcess(this);
    imageOrganizerProcess -> startDetached("imageorganizer");
    delete imageOrganizerProcess;
}


void MainWindow::on_actionMXLPlayer_triggered()
{
    QProcess *mxlPlayerProcess = new QProcess(this);
    mxlPlayerProcess -> startDetached("mxlplayer");
    delete mxlPlayerProcess;
}


void MainWindow::on_actionOpenSource_triggered()
{
    QDesktopServices::openUrl(QUrl(QString("https://github.com/izwb003/AVPStudio")));
}


void MainWindow::on_actionCheckUpdate_triggered()
{
    QDesktopServices::openUrl(QUrl(QString("https://github.com/izwb003/AVPStudio/releases")));
}


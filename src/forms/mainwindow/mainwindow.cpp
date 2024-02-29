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

#include "settings.h"

#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("AVPStudio - Welcome");

    PageWelcome *pageWelcome = new PageWelcome;
    ui->stackedWidget->addWidget(pageWelcome);
    PageCreate *pageCreate = new PageCreate;
    ui->stackedWidget->addWidget(pageCreate);
    PageEdit *pageEdit = new PageEdit;
    ui->stackedWidget->addWidget(pageEdit);
    ui->stackedWidget->setCurrentIndex(0);

    connect(pageWelcome, SIGNAL(createContent()), this, SLOT(do_createContent()));
    connect(pageWelcome, SIGNAL(createContent()), pageCreate, SLOT(do_init()));
    connect(pageCreate, SIGNAL(editContent()), this, SLOT(do_editContent()));
    connect(pageCreate, SIGNAL(editContent()), pageEdit, SLOT(do_init()));
    connect(this, SIGNAL(openFile(QString)), pageCreate, SLOT(on_labelDragText_linkActivated(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
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
    ui->stackedWidget->setCurrentIndex(0);
    ui->actionOpenFile->setEnabled(false);
}


void MainWindow::on_actionOpenFile_triggered()
{
    emit openFile("");
}


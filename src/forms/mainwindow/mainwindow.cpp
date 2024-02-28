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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::do_createContent()
{
    this->setWindowTitle("AVPStudio - Create (" + settings.getSizeString() + ")");
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::do_editContent()
{
    ui->stackedWidget->setCurrentIndex(2);
}

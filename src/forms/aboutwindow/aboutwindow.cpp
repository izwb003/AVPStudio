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
#include "aboutwindow.h"
#include "ui_aboutwindow.h"

AboutWindow::AboutWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AboutWindow)
{
    ui->setupUi(this);
    this->setWindowFlags(windowFlags()& ~Qt::WindowMinMaxButtonsHint);
    this->setFixedSize(this->width(), this->height());

    ui->labelVersion->setText(tr("版本") + QString::number(PROJECT_VERSION_MAJOR) + "." + QString::number(PROJECT_VERSION_MINOR) + "." + QString::number(PROJECT_VERSION_PATCH) + tr("构建于") + __DATE__);
}

AboutWindow::~AboutWindow()
{
    delete ui;
}

void AboutWindow::on_pushButtonOK_clicked()
{
    this->close();
}


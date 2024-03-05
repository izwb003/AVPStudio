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
#include "pagecompleted.h"
#include "ui_pagecompleted.h"

PageCompleted::PageCompleted(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageCompleted)
{
    ui->setupUi(this);
}

PageCompleted::~PageCompleted()
{
    delete ui;
}

void PageCompleted::setStatus(bool isError, QString errorStr)
{
    if(isError)
    {
        ui->labelIcon->setPixmap(QPixmap(":/images/images/error.png"));
        ui->labelText->setText(errorStr);
        this->window()->setWindowTitle("AVPStudio - Error");
    }
    else
    {
        ui->labelIcon->setPixmap(QPixmap(":/images/images/completed.png"));
        ui->labelText->setText(tr("放映内容导出成功。"));
        this->window()->setWindowTitle("AVPStudio - Completed");
    }
}

void PageCompleted::on_pushButtonOK_clicked()
{
    emit reInit();
}


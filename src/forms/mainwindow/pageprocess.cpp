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
#include "pageprocess.h"
#include "ui_pageprocess.h"

#include "settings.h"

PageProcess::PageProcess(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageProcess)
{
    ui->setupUi(this);
}

PageProcess::~PageProcess()
{
    delete ui;
}

void PageProcess::do_proc()
{
    doProcessThread = new TDoProcess(this);

    connect(doProcessThread, SIGNAL(setProgressMax(int64_t)), this, SLOT(do_setProgressMax(int64_t)));
    connect(doProcessThread, SIGNAL(setProgress(int64_t)), this, SLOT(do_setProgress(int64_t)));
    connect(doProcessThread, SIGNAL(setLabel(QString)), this, SLOT(do_setLabel(QString)));

    doProcessThread->start();
}

void PageProcess::do_setProgressMax(int64_t num)
{
    ui->progressBar->setMaximum(num);
}

void PageProcess::do_setProgress(int64_t num)
{
    ui->progressBar->setValue(num);
}

void PageProcess::do_setLabel(QString str)
{
    ui->label->setText(str);
}

void PageProcess::on_pushButtonCancel_clicked()
{
    doProcessThread->exit(-1);
    while(true)
    {
        if(!doProcessThread->isRunning())
        {
            delete doProcessThread;
            doProcessThread = NULL;
            break;
        }
    }
    emit reInit();
}


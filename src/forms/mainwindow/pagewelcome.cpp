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
#include "pagewelcome.h"
#include "ui_pagewelcome.h"

#include "settings.h"

PageWelcome::PageWelcome(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageWelcome)
{
    ui->setupUi(this);
}

PageWelcome::~PageWelcome()
{
    delete ui;
}

void PageWelcome::on_pushButtonExit_clicked()
{
    qApp->quit();
}


void PageWelcome::on_radioButtonSmall_clicked(bool checked)
{
    ui->labelInfo->setText(tr("当前设定：走廊尺寸：SMALL / 实际大小：5.5Mx2.1M / 画面分辨率：2830x1080"));
    QPixmap pixSmall(":/images/images/AVP_Small.jpg");
    ui->labelPreview->setPixmap(pixSmall);
    settings.size = AVP::kAVPSmallSize;
}


void PageWelcome::on_radioButtonMedium_clicked(bool checked)
{
    ui->labelInfo->setText(tr("当前设定：走廊尺寸：MEDIUM / 实际大小：9Mx2.1M / 画面分辨率：4633x1080"));
    QPixmap pixMedium(":/images/images/AVP_Medium.jpg");
    ui->labelPreview->setPixmap(pixMedium);
    settings.size = AVP::kAVPMediumSize;
}


void PageWelcome::on_radioButtonLarge_clicked(bool checked)
{
    ui->labelInfo->setText(tr("当前设定：走廊尺寸：LARGE / 实际大小：12Mx2.1M / 画面分辨率：6167x1080"));
    QPixmap pixLarge(":/images/images/AVP_Large.jpg");
    ui->labelPreview->setPixmap(pixLarge);
    settings.size = AVP::kAVPLargeSize;
}


void PageWelcome::on_pushButtonNext_clicked()
{
    emit createContent();
}


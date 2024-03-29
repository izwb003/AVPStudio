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

#include <QDebug>
#include <QXmlStreamReader>

PageWelcome::PageWelcome(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageWelcome)
{
    ui->setupUi(this);

    ui->treeWidgetCinemas->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    fillCinemaList();
}

PageWelcome::~PageWelcome()
{
    delete ui;
}

void PageWelcome::fillCinemaList()
{
    QFile cinemaListFile("cinemalist.xml");
    if(!cinemaListFile.open(QIODevice::ReadOnly))
        return;

    QXmlStreamReader reader(&cinemaListFile);
    while(!reader.atEnd())
    {
        reader.readNext();

        if(reader.isStartElement() && reader.name().compare("cinemalist") == 0)
        {
            if(reader.attributes().value("version").compare("1") != 0)
                return;
            ui->labelUpdateTime->setText(tr("数据更新于") + reader.attributes().value("updatetime").toString());
        }

        if(reader.isStartElement() && reader.name().compare("location") == 0)
        {
            QTreeWidgetItem *locationItem = new QTreeWidgetItem();
            locationItem->setText(0, reader.attributes().value("name").toString());
            locationItem->setFlags(Qt::ItemIsEnabled);
            ui->treeWidgetCinemas->addTopLevelItem(locationItem);

            while(!reader.atEnd())
            {
                reader.readNext();

                if(reader.isStartElement() && reader.name().compare("province") == 0)
                {
                    QTreeWidgetItem *provinceItem = new QTreeWidgetItem();
                    provinceItem->setText(0, reader.attributes().value("name").toString());
                    provinceItem->setFlags(Qt::ItemIsEnabled);
                    locationItem->addChild(provinceItem);

                    while(!reader.atEnd())
                    {
                        reader.readNext();

                        if(reader.isStartElement() && reader.name().compare("cinema") == 0)
                        {
                            QTreeWidgetItem *cinemaItem = new QTreeWidgetItem();
                            cinemaItem->setText(0, reader.attributes().value("name").toString());
                            reader.readNextStartElement();
                            if(reader.isStartElement() && reader.name().compare("avpsize") == 0)
                                cinemaItem->setText(1, reader.readElementText());
                            provinceItem->addChild(cinemaItem);
                        }

                        if(reader.isEndElement() && reader.name().compare("province") == 0)
                            break;
                    }
                }

                if(reader.isEndElement() && reader.name().compare("location") == 0)
                    break;
            }
        }

        if(reader.isEndElement() && reader.name().compare("cinemalist") == 0)
            break;
    }

    ui->treeWidgetCinemas->expandAll();
}

void PageWelcome::on_pushButtonExit_clicked()
{
    qApp->quit();
}

void PageWelcome::on_pushButtonNext_clicked()
{
    emit createContent();
}


void PageWelcome::on_treeWidgetCinemas_itemActivated(QTreeWidgetItem *item, int column)
{
    if(item->text(1) == "Small")
        ui->radioButtonSmall->setChecked(true);
    else if(item->text(1) == "Medium")
        ui->radioButtonMedium->setChecked(true);
    else if(item->text(1) == "Large")
        ui->radioButtonLarge->setChecked(true);
}


void PageWelcome::on_radioButtonSmall_toggled(bool checked)
{
    if(checked)
    {
        settings.size = AVP::kAVPSmallSize;
        ui->labelInfo->setText(tr("当前设定：走廊尺寸：") + settings.getSizeString() + tr(" / 实际大小：") + settings.getRealSize() + tr(" / 画面分辨率：") + settings.getSizeResolution());
    }
}


void PageWelcome::on_radioButtonMedium_toggled(bool checked)
{
    if(checked)
    {
        settings.size = AVP::kAVPMediumSize;
        ui->labelInfo->setText(tr("当前设定：走廊尺寸：") + settings.getSizeString() + tr(" / 实际大小：") + settings.getRealSize() + tr(" / 画面分辨率：") + settings.getSizeResolution());
    }
}


void PageWelcome::on_radioButtonLarge_toggled(bool checked)
{
    if(checked)
    {
        settings.size = AVP::kAVPLargeSize;
        ui->labelInfo->setText(tr("当前设定：走廊尺寸：") + settings.getSizeString() + tr(" / 实际大小：") + settings.getRealSize() + tr(" / 画面分辨率：") + settings.getSizeResolution());
    }
}


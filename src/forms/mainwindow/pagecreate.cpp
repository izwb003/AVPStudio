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
#include "pagecreate.h"
#include "ui_pagecreate.h"

#include "settings.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>

void PageCreate::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
        if(event->mimeData()->urls().size() == 1)
        {
            ui->labelDragImg->setStyleSheet("border: 5px dashed blue; border-radius: 10px;");
            ui->labelDragText->setText(tr("放下文件以添加..."));
        }
        else
        {
            ui->labelDragImg->setStyleSheet("border: 5px dashed red; border-radius: 10px;");
            ui->labelDragText->setText(tr("只支持拖放一个文件"));
        }
    }
    else
        event->ignore();
}

void PageCreate::dragLeaveEvent(QDragLeaveEvent *event)
{
    rewriteLabelDragText();
}

void PageCreate::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        if(event->mimeData()->urls().size() != 1)
        {
            rewriteLabelDragText();
            return;
        }
        else
        {
            QString fileName = event->mimeData()->urls().at(0).toLocalFile();
            QFileInfo fileInfo(fileName);
            if(fileInfo.isFile())
            {
                QStringList supportedFormat = {"mp4", "mpg", "avi", "mkv", "mov", "flv"};
                bool isFormatSupported = false;
                for(QString format : supportedFormat)
                {
                    if(fileInfo.suffix().compare(format) == 0)
                    {
                        isFormatSupported = true;
                        break;
                    }
                }
                if(!isFormatSupported)
                {
                    QMessageBox::critical(this, tr("错误"), tr("不支持的文件格式。"));
                    rewriteLabelDragText();
                    return;
                }
                settings.inputVideoPath = fileName;
                settings.inputVideoInfo = fileInfo;
                emit editContent();
                rewriteLabelDragText();
            }
            else
                rewriteLabelDragText();
        }
    }
}

PageCreate::PageCreate(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageCreate)
{
    ui->setupUi(this);
}

PageCreate::~PageCreate()
{
    delete ui;
}

void PageCreate::do_init()
{
    ui->labelSizeHint->setText(QString(tr("当前走廊尺寸大小%1建议素材分辨率%2")).arg(settings.getSizeString()).arg(settings.getSizeResolution()));
}

void PageCreate::on_labelDragText_linkActivated(const QString &link)
{
    settings.inputVideoPath = QFileDialog::getOpenFileName(this, tr("选择素材文件..."), QDir::homePath(), tr("视频文件 (*.mp4 *.mpg *.avi *.mkv *.mov *.flv)"));
    settings.inputVideoInfo.setFile(settings.inputVideoPath);
    if(!settings.inputVideoPath.isEmpty())
        emit editContent();
}

void PageCreate::rewriteLabelDragText()
{
    ui->labelDragImg->setStyleSheet("border: 0px; border-radius: 0px;");
    ui->labelDragText->setText(tr("<html><head/><body><p>拖放素材文件到此以添加内容或<a href=\"%1\"><span style=\" text-decoration: underline; color:#808080;\">浏览</span></a>...</p></body></html>\n"));
}

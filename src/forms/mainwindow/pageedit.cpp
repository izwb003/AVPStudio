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
#include "pageedit.h"
#include "ui_pageedit.h"

#include "settings.h"

#include <QAudioOutput>
#include <QMessageBox>

PageEdit::PageEdit(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageEdit)
{
    ui->setupUi(this);

    ui->widgetVideoPreview->setAspectRatioMode(Qt::IgnoreAspectRatio);
    player = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    player->setVideoOutput(ui->widgetVideoPreview);

    connect(player, &QMediaPlayer::positionChanged, this, &PageEdit::do_positionChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &PageEdit::do_durationChanged);
    connect(player, &QMediaPlayer::playbackStateChanged, this, &PageEdit::do_playStateChanged);
    connect(player, &QMediaPlayer::errorOccurred, this, &PageEdit::do_playErrorOccured);
}

PageEdit::~PageEdit()
{
    delete ui;
}

void PageEdit::do_init()
{
    ui->labelFileName->setText(settings.inputVideoInfo.fileName());

    switch(settings.size)
    {
    case AVP::kAVPSmallSize:
        ui->widgetVideoPreview->setMinimumSize(524, 200);
        ui->widgetVideoPreview->setMaximumSize(524, 200);
        break;
    case AVP::kAVPMediumSize:
        ui->widgetVideoPreview->setMinimumSize(643, 150);
        ui->widgetVideoPreview->setMaximumSize(643, 150);
        break;
    case AVP::kAVPLargeSize:
        ui->widgetVideoPreview->setMinimumSize(685, 120);
        ui->widgetVideoPreview->setMaximumSize(685, 120);
        break;
    }

    player->setSource(QUrl::fromLocalFile(settings.inputVideoPath));
}

void PageEdit::do_positionChanged(qint64 position)
{
    if(ui->horizontalSliderPosition->isSliderDown())
        return;
    ui->horizontalSliderPosition->setSliderPosition(position);
    int secs = position / 1000;
    int mins = secs / 60;
    secs = secs % 60;
    positionTime = QString("%1:%2").arg(mins, 2, 10, QLatin1Char('0')).arg(secs, 2, 10, QLatin1Char('0'));
    ui->labelDuration->setText(positionTime + "/" + durationTime);
}

void PageEdit::do_durationChanged(qint64 duration)
{
    ui->horizontalSliderPosition->setMaximum(duration);
    int secs = duration / 1000;
    int mins = secs / 60;
    secs = secs % 60;
    durationTime = QString("%1:%2").arg(mins, 2, 10, QLatin1Char('0')).arg(secs, 2, 10, QLatin1Char('0'));
    ui->labelDuration->setText(positionTime + "/" + durationTime);
}

void PageEdit::do_playStateChanged(QMediaPlayer::PlaybackState state)
{
    bool isPlaying = (state == QMediaPlayer::PlayingState);
    ui->toolButtonPausePlay->setChecked(isPlaying);
    on_toolButtonPausePlay_clicked(isPlaying);
}

void PageEdit::do_playErrorOccured(QMediaPlayer::Error error, const QString &errorString)
{
    QMessageBox::critical(this, tr("加载素材出错"), errorString);
}

void PageEdit::on_toolButtonPausePlay_clicked(bool checked)
{
    if(checked)
    {
        QIcon icon(":/images/images/pause.png");
        ui->toolButtonPausePlay->setIcon(icon);
        player->play();
    }
    else
    {
        QIcon icon(":/images/images/play.png");
        ui->toolButtonPausePlay->setIcon(icon);
        player->pause();
    }
}


void PageEdit::on_verticalSliderVolume_valueChanged(int value)
{
    player->audioOutput()->setVolume(value/100.0);
}


void PageEdit::on_horizontalSliderPosition_valueChanged(int value)
{
    if(ui->horizontalSliderPosition->isSliderDown())
        player->setPosition(value);
}


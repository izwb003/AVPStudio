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
#include <QFileDialog>
#include <QMessageBox>

PageEdit::PageEdit(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageEdit)
{
    ui->setupUi(this);

    player = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    player->setVideoOutput(ui->widgetVideoPreview);

    ui->comboBoxFrameRate->addItem("23.976fps", QVariant::fromValue(av_make_q(24000, 1001)));
    ui->comboBoxFrameRate->addItem("24fps", QVariant::fromValue(av_make_q(24, 1)));
    ui->comboBoxFrameRate->addItem("25fps", QVariant::fromValue(av_make_q(25, 1)));
    ui->comboBoxFrameRate->addItem("30fps", QVariant::fromValue(av_make_q(30, 1)));

    AVP::ColorSettings color470;
    color470.outputColorPrimary = AVCOL_PRI_BT470M;
    color470.outputVideoColorTrac = AVCOL_TRC_GAMMA22;
    ui->comboBoxVideoColor->addItem("BT.470", QVariant::fromValue(color470));

    AVP::ColorSettings color709;
    color709.outputColorPrimary = AVCOL_PRI_BT709;
    color709.outputVideoColorTrac = AVCOL_TRC_BT709;
    ui->comboBoxVideoColor->addItem("BT.709", QVariant::fromValue(color709));

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


void PageEdit::on_checkBoxPadding_clicked(bool checked)
{
    if(checked)
        ui->widgetVideoPreview->setAspectRatioMode(Qt::IgnoreAspectRatio);
    else
        ui->widgetVideoPreview->setAspectRatioMode(Qt::KeepAspectRatio);
}


void PageEdit::on_pushButtonOutput_clicked()
{
    settings.outputVideoBitRate = ui->doubleSpinBoxVideoBitRate->value();
    settings.outputFrameRate = ui->comboBoxFrameRate->currentData(Qt::UserRole).value<AVRational>();
    settings.outputColor = ui->comboBoxVideoColor->currentData(Qt::UserRole).value<AVP::ColorSettings>();
    settings.outputFileName = ui->lineEditFileName->text();
    settings.useDolbyNaming = ui->checkBoxDolbyNaming->isChecked();
    settings.scalePicture = ui->checkBoxPadding->isChecked();
    settings.outputVolume = ui->verticalSliderVolume->value();

    settings.outputFilePath = QFileDialog::getExistingDirectory(this, tr("选择保存位置..."), QDir::currentPath());
}


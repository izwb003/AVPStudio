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
#include "playcontrol.h"
#include "ui_playcontrol.h"

#include <QMessageBox>
#include <QToolTip>

PlayControl::PlayControl(QWidget *parent, QString mxlPath, QString wavPath, AVP::AVPSize size)
    : QWidget(parent)
    , ui(new Ui::PlayControl)
{
    this -> mxlPath = mxlPath;
    this -> wavPath = wavPath;
    this -> size = size;
    this -> mainPage = qobject_cast<MainWindow*>(parent);

    ui->setupUi(this);

    this->setGeometry((qApp->primaryScreen()->size().width() / 2) - (this->width() / 2), (qApp->primaryScreen()->size().height() / 6) * 5 - (this->height() / 2), this->width(), this->height());

    setMouseTracking(true);
    ui->labelIcon->installEventFilter(this);

    videoPlayer = new TPlayVideo(this, mxlPath, wavPath, size);
    connect(videoPlayer, SIGNAL(showError(QString)), this, SLOT(do_showError(QString)));
    connect(videoPlayer, SIGNAL(setPositionBarMax(int,double)), this, SLOT(do_setPositionBarMax(int,double)));
    connect(videoPlayer, SIGNAL(setPosition(int)), this, SLOT(do_setPosition(int)));
    connect(videoPlayer, SIGNAL(sdlQuit()), this, SLOT(on_toolButtonBack_clicked()));
    connect(this, SIGNAL(play()), videoPlayer, SLOT(do_play()));
    connect(this, SIGNAL(pause()), videoPlayer, SLOT(do_pause()));
    connect(this, SIGNAL(updatePosition(int)), videoPlayer, SLOT(do_updatePosition(int)));

    if(videoPlayer->init())
        return;

    videoPlayer->start();
}

PlayControl::~PlayControl()
{
    delete ui;
}

void PlayControl::do_showError(QString errorMsg)
{
    QMessageBox::critical(this, tr("错误"), errorMsg);
}

void PlayControl::do_setPositionBarMax(int val, double timebase)
{
    ui->horizontalSliderPosition->setMaximum(val);
    this->timebase = timebase;
    QTime totalTime(0, 0, 0);
    qDebug()<<timebase;
    totalTime = totalTime.addSecs((int)((double)val * timebase));
    ui->labelTotalTime->setText(totalTime.toString("mm:ss"));
}

void PlayControl::do_setPosition(int val)
{
    if(!isPositionBarDragging)
        ui->horizontalSliderPosition->setValue(val);
    QTime time(0, 0, 0);
    time = time.addSecs((int)((double)val * timebase));
    ui->labelPlayTime->setText(time.toString("mm:ss"));
}

void PlayControl::mousePressEvent(QMouseEvent *event)
{
    if(isDragging)
        lastMousePos = event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
}

void PlayControl::mouseMoveEvent(QMouseEvent *event)
{
    if(isDragging)
        move(event->globalPosition().toPoint() - lastMousePos);
    event->accept();
}

void PlayControl::mouseReleaseEvent(QMouseEvent *event)
{
    if(isDragging)
        isDragging = false;
    event->accept();
}

bool PlayControl::eventFilter(QObject *object, QEvent *event)
{
    if(object == ui->labelIcon)
    {
        if(event->type() == QEvent::MouseButtonPress)
            startDragging();
        else if(event->type() == QEvent::MouseButtonRelease)
            stopDragging();
        return false;
    }
    return QWidget::eventFilter(object, event);
}


void PlayControl::on_toolButtonBack_clicked()
{
    qApp->quit();
}


void PlayControl::on_toolButtonMute_clicked(bool checked)
{
    if(checked)
    {
        ui->toolButtonMute->setIcon(QIcon(":/images/images/mute.png"));
        muteVolume = ui->horizontalSliderVolume->value();
        ui->horizontalSliderVolume->setValue(0);
    }
    else
    {
        ui->toolButtonMute->setIcon(QIcon(":/images/images/volume.png"));
        ui->horizontalSliderVolume->setValue(muteVolume);
    }
}


void PlayControl::on_horizontalSliderVolume_valueChanged(int value)
{
    QPoint globalPos = ui->horizontalSliderVolume->mapToGlobal(QPoint(0, 0));
    QToolTip::showText(QPoint(globalPos.x() + ui->horizontalSliderVolume->width() / 2, globalPos.y()), QString::number(ui->horizontalSliderVolume->value()));
}


void PlayControl::on_toolButtonPlayPause_clicked(bool checked)
{
    if(checked)
    {
        ui->toolButtonPlayPause->setIcon(QIcon(":/images/images/pause.png"));
        emit play();
    }
    else
    {
        ui->toolButtonPlayPause->setIcon(QIcon(":/images/images/play.png"));
        emit pause();
    }
}


void PlayControl::on_horizontalSliderPosition_sliderReleased()
{
    emit updatePosition(ui->horizontalSliderPosition->value());
    isPositionBarDragging = false;
}


void PlayControl::on_horizontalSliderPosition_sliderPressed()
{
    isPositionBarDragging = true;
}


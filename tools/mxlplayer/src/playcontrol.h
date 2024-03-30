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
#ifndef PLAYCONTROL_H
#define PLAYCONTROL_H

#include "mainwindow.h"
#include "playvideo.h"
#include "avpsettings.h"

#include <QWidget>
#include <QMouseEvent>
#include <QTime>

namespace Ui {
class PlayControl;
}

class PlayControl : public QWidget
{
    Q_OBJECT

public:
    explicit PlayControl(QWidget *parent = nullptr, QString mxlPath = "", QString wavPath = "", AVP::AVPSize size = AVP::kAVPMediumSize);
    ~PlayControl();

signals:
    void updatePosition(int val);

    void play();

    void pause();

    void setVolume(int val);

private:
    Ui::PlayControl *ui;

    MainWindow *mainPage;

    QString mxlPath = "";
    QString wavPath = "";
    AVP::AVPSize size = AVP::kAVPMediumSize;

    bool isDragging = false;
    QPoint lastMousePos;

    double timebase = 0;

    int muteVolume = 0;

    TPlayVideo *videoPlayer = NULL;

private slots:
    inline void startDragging()  {isDragging = true;}
    inline void stopDragging()   {isDragging = false;}

    void do_showError(QString errorMsg);

    void do_setPositionBarMax(int val, double timebase);

    void do_setPosition(int val);

    void on_toolButtonBack_clicked();

    void on_toolButtonMute_clicked(bool checked);

    void on_horizontalSliderVolume_valueChanged(int value);

    void on_toolButtonPlayPause_clicked(bool checked);

    void on_horizontalSliderPosition_sliderReleased();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
};

#endif // PLAYCONTROL_H

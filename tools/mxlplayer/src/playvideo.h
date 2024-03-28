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
#ifndef TPLAYVIDEO_H
#define TPLAYVIDEO_H

#define SDL_MAIN_HANDLED

#include "avpsettings.h"

#include <QThread>

class TPlayVideo : public QThread
{
    Q_OBJECT
public:
    explicit TPlayVideo(QObject *parent = nullptr, QString mxlPath = "", QString wavPath = "", AVP::AVPSize size = AVP::kAVPMediumSize);

    int init();

    void cleanup();

    void notifyQuit();

signals:
    void showError(QString errorMsg);

    void setPositionBarMax(int val, double timebase);

    void setPosition(int val);

    void sdlQuit();

private:
    QString mxlPath = "";
    QString wavPath = "";
    AVP::AVPSize size = AVP::kAVPMediumSize;
    int AVPWidth = 4632;
    int AVPHeight = 1080;

    int newPosition = 0;

private slots:
    void do_updatePosition(int val);

    void do_play();

    void do_pause();

protected:
    void run();
};

#endif // TPLAYVIDEO_H

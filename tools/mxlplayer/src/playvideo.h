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

#include <SDL.h>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswscale/swscale.h>
}

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

    AVFormatContext *videoFmtCxt = NULL;
    int videoStreamID = 0;

    const AVCodec *videoDecoder = NULL;
    AVCodecContext *videoDecoderCxt = NULL;

    AVFilterGraph *videoFilterGraph = NULL;
    AVFilterInOut *videoFilterInput = NULL;
    AVFilterInOut *videoFilterOutput = NULL;

    const AVFilter *videoFilterSrc = NULL;
    AVFilterContext *videoFilterSrcCxt = NULL;
    const AVFilter *videoFilterSink = NULL;
    AVFilterContext *videoFilterSinkCxt = NULL;

    SwsContext *scalerCxt = NULL;

    AVPacket *packet = NULL;
    AVFrame *frameIn = NULL;
    AVFrame *frameFiltered = NULL;
    AVFrame *frameScaled = NULL;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    SDL_Thread *thread = NULL;
    SDL_Event event;

    int newPosition = 0;

    static int SDLRefresher(void* opaque);

private slots:
    void do_updatePosition(int val);

    void do_play();

    void do_pause();

protected:
    void run();
};

#endif // TPLAYVIDEO_H

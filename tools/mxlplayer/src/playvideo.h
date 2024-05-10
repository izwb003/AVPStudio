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
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

class TPlayVideo : public QThread
{
    Q_OBJECT
public:
    explicit TPlayVideo(QObject *parent = nullptr, QString mxlPath = "", QString wavPath = "", AVP::AVPSize size = AVP::kAVPMediumSize);

    int init();

    void cleanup();

    void notifyQuit();

    int SDLAudioDecoderInternal(void *opaque);

    void SDLFillAudioInternal(void *data, uint8_t *stream, int length);

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

    AVFormatContext *videoFmtCxt = NULL;
    int videoStreamID = 0;
    AVFormatContext *audioFmtCxt = NULL;
    int audioStreamID = 0;

    const AVCodec *videoDecoder = NULL;
    AVCodecContext *videoDecoderCxt = NULL;
    const AVCodec *audioDecoder = NULL;
    AVCodecContext *audioDecoderCxt = NULL;

    AVFilterGraph *videoFilterGraph = NULL;
    AVFilterInOut *videoFilterInput = NULL;
    AVFilterInOut *videoFilterOutput = NULL;

    const AVFilter *videoFilterSrc = NULL;
    AVFilterContext *videoFilterSrcCxt = NULL;
    const AVFilter *videoFilterSink = NULL;
    AVFilterContext *videoFilterSinkCxt = NULL;

    SwsContext *scalerCxt = NULL;
    SwrContext *resamplerCxt = NULL;

    AVPacket *vPacket = NULL;
    AVPacket *aPacket = NULL;
    AVFrame *frameIn = NULL;
    AVFrame *frameFiltered = NULL;
    AVFrame *frameScaled = NULL;
    AVFrame *frame = NULL;

    int iAudioBufferSize = 0;
    int iAudioBufferSampleCount = 0;
    uint8_t *iAudioBuffer = NULL;
    int oAudioBufferSize = 0;
    int oAudioBufferSampleCount = 0;
    uint8_t *oAudioBuffer = NULL;

    AVAudioFifo *audioQueue = NULL;
    SDL_mutex *audioQueueMutex = NULL;

    int volume = 50;
    SDL_mutex *volumeMutex = NULL;

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    SDL_Thread *threadRefresh = NULL;
    SDL_Thread *threadDecodeAudio = NULL;
    SDL_Event eventSDL;
    SDL_AudioSpec wantedSpec;

private slots:
    void do_updatePosition(int val);

    void do_play();

    void do_pause();

    void do_volumeChanged(int val);

protected:
    void run();
};

#endif // TPLAYVIDEO_H

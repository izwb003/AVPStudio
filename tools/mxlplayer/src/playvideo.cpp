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
#include "playvideo.h"

#include <SDL.h>

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

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

#define SDL_CUSTOM_REFRESH_EVENT (SDL_USEREVENT + 1)
#define SDL_CUSTOM_POSITION_UPDATE_EVENT (SDL_USEREVENT + 2)
#define SDL_CUSTOM_QUIT_EVENT (SDL_USEREVENT + 4)

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

TPlayVideo *player = NULL;

/*
 * Flag to control the behavior of the SDL refresher.
 * case 0: Play normally.
 * case 1: Paused.
 * case 2: Quit.
 */
static int refresherFlag = 1;
SDL_mutex *refresherFlagMutex = NULL;

// FFmpeg filter graph description
static const char *filterGraphLarge =
    "[in]split[in1][in2];"
    "[in1]crop=3840:1080:0:0[left];"
    "[in2]crop=2326:1080:1513:1080[right];"
    "[left][right]hstack=2[out]";
static const char *filterGraphMedium =
    "[in]split[in1][in2];"
    "[in1]crop=3073:1080:767:0[left];"
    "[in2]crop=1559:1080:1513:1080[right];"
    "[left][right]hstack=2[out]";
static const char *filterGraphSmall =
    "[in]split[in1][in2];"
    "[in1]crop=2172:1080:1668:0[left];"
    "[in2]crop=658:1080:1513:1080[right];"
    "[left][right]hstack=2[out]";

static int SDLRefresher(void *opaque)
{
    SDL_Event refreshEvent;
    refreshEvent.type = SDL_CUSTOM_REFRESH_EVENT;
    while(true)
    {
        SDL_LockMutex(refresherFlagMutex);
        if(refresherFlag == 0)
        {
            SDL_PushEvent(&refreshEvent);
            SDL_Delay(*(int*)opaque);
        }
        else if(refresherFlag == 1)
            continue;
        else if(refresherFlag == 2)
            break;
        SDL_UnlockMutex(refresherFlagMutex);
    }
    return 0;
}

static int SDLAudioDecoder(void *opaque)
{
    return player->SDLAudioDecoderInternal(opaque);
}

static void SDLFillAudio(void *data, uint8_t *stream, int length)
{
    return player->SDLFillAudioInternal(data, stream, length);
}

TPlayVideo::TPlayVideo(QObject *parent, QString mxlPath, QString wavPath, AVP::AVPSize size)
    : QThread{parent}
{
    this->mxlPath = mxlPath;
    this->wavPath = wavPath;
    this->size = size;

    AVPHeight = 1080;
    switch(size)
    {
    case AVP::kAVPLargeSize:
        AVPWidth = 6166;
        break;
    case AVP::kAVPMediumSize:
        AVPWidth = 4632;
        break;
    case AVP::kAVPSmallSize:
        AVPWidth = 2830;
        break;
    }

    player = this;
}

int TPlayVideo::init()
{
    // FFmpeg init
    av_log_set_level(AV_LOG_QUIET);
    static int avError = 0;

    // Open input file and get stream info
    avError = avformat_open_input(&videoFmtCxt, mxlPath.toUtf8(), 0, 0);
    if(avError < 0)
    {
        emit showError(tr("打开MXL失败。"));
        cleanup();
        return avError;
    }
    avError = avformat_find_stream_info(videoFmtCxt, 0);
    if(avError < 0)
    {
        emit showError(tr("打开MXL失败：不能找到流信息。"));
        cleanup();
        return avError;
    }

    if(!wavPath.isEmpty())
    {
        avError = avformat_open_input(&audioFmtCxt, wavPath.toUtf8(), 0, 0);
        if(avError < 0)
        {
            emit showError(tr("打开WAV失败。"));
            cleanup();
            return avError;
        }
        avError = avformat_find_stream_info(audioFmtCxt, 0);
        if(avError < 0)
        {
            emit showError(tr("打开WAV失败：不能找到流信息。"));
            cleanup();
            return avError;
        }
    }

    // Get input video stream
    videoStreamID = av_find_best_stream(videoFmtCxt, AVMEDIA_TYPE_VIDEO, -1, -1, &videoDecoder, 0);
    if(videoStreamID == AVERROR_STREAM_NOT_FOUND)
    {
        emit showError(tr("打开MXL失败：不能找到流信息。"));
        cleanup();
        return -1;
    }

    if(videoFmtCxt->streams[videoStreamID]->codecpar->width != 3840 || videoFmtCxt->streams[videoStreamID]->codecpar->height !=2160)
    {
        emit showError(tr("打开MXL失败：不正确的视频尺寸。"));
        cleanup();
        return -1;
    }

    if(!wavPath.isEmpty())
    {
        audioStreamID = av_find_best_stream(audioFmtCxt, AVMEDIA_TYPE_AUDIO, -1, -1, &audioDecoder, 0);
        if(audioStreamID == AVERROR_STREAM_NOT_FOUND)
        {
            emit showError(tr("打开WAV失败：不能找到流信息。"));
            cleanup();
            return -1;
        }
    }

    // Open decoder
    videoDecoderCxt = avcodec_alloc_context3(videoDecoder);
    avError = avcodec_parameters_to_context(videoDecoderCxt, videoFmtCxt->streams[videoStreamID]->codecpar);
    if(avError < 0)
    {
        emit showError(tr("打开MXL失败：不能找到解码器。"));
        cleanup();
        return avError;
    }
    avError = avcodec_open2(videoDecoderCxt, videoDecoder, 0);
    if(avError < 0)
    {
        emit showError(tr("打开MXL失败：不能打开解码器。"));
        cleanup();
        return avError;
    }

    if(!wavPath.isEmpty())
    {
        audioDecoderCxt = avcodec_alloc_context3(audioDecoder);
        avError = avcodec_parameters_to_context(audioDecoderCxt, audioFmtCxt->streams[audioStreamID]->codecpar);
        if(avError < 0)
        {
            emit showError(tr("打开WAV失败：不能找到解码器。"));
            cleanup();
            return avError;
        }
        avError = avcodec_open2(audioDecoderCxt, audioDecoder, 0);
        if(avError < 0)
        {
            emit showError(tr("打开WAV失败：不能打开解码器。"));
            cleanup();
            return avError;
        }
        if(audioDecoderCxt->ch_layout.nb_channels > 2)
        {
            emit showError(tr("由于软件限制，MXLPlayer暂不能回放非单声道/立体声配置的WAV音频。\n这并不会影响您的WAV文件正常在杜比影院设备上的播放。\n您可以使用其它音频播放器检视该WAV音频文件。"));
            cleanup();
            return avError;
        }
    }

    // Init filter
    videoFilterGraph = avfilter_graph_alloc();

    videoFilterSrc = avfilter_get_by_name("buffer");
    char videoFilterSrcArgs[512];
    snprintf(videoFilterSrcArgs, sizeof(videoFilterSrcArgs), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", videoDecoderCxt->width, videoDecoderCxt->height, AV_PIX_FMT_YUV420P, videoFmtCxt->streams[videoStreamID]->time_base.num, videoFmtCxt->streams[videoStreamID]->time_base.den, videoDecoderCxt->sample_aspect_ratio.num, videoDecoderCxt->sample_aspect_ratio.den);
    avError = avfilter_graph_create_filter(&videoFilterSrcCxt, videoFilterSrc, "in", videoFilterSrcArgs, 0, videoFilterGraph);

    videoFilterSink = avfilter_get_by_name("buffersink");
    avError = avfilter_graph_create_filter(&videoFilterSinkCxt, videoFilterSink, "out", 0, 0, videoFilterGraph);

    videoFilterInput = avfilter_inout_alloc();
    videoFilterInput -> name = av_strdup("in");
    videoFilterInput -> filter_ctx = videoFilterSrcCxt;
    videoFilterInput -> pad_idx = 0;
    videoFilterInput -> next = NULL;

    videoFilterOutput = avfilter_inout_alloc();
    videoFilterOutput -> name = av_strdup("out");
    videoFilterOutput -> filter_ctx = videoFilterSinkCxt;
    videoFilterOutput -> pad_idx = 0;
    videoFilterOutput -> next = NULL;

    switch(size)
    {
    case AVP::kAVPLargeSize:
        avError = avfilter_graph_parse_ptr(videoFilterGraph, filterGraphLarge, &videoFilterOutput, &videoFilterInput, 0);
        break;
    case AVP::kAVPMediumSize:
        avError = avfilter_graph_parse_ptr(videoFilterGraph, filterGraphMedium, &videoFilterOutput, &videoFilterInput, 0);
        break;
    case AVP::kAVPSmallSize:
        avError = avfilter_graph_parse_ptr(videoFilterGraph, filterGraphSmall, &videoFilterOutput, &videoFilterInput, 0);
        break;
    }

    avError = avfilter_graph_config(videoFilterGraph, 0);

    // Init scaler
    scalerCxt = sws_getContext(videoDecoderCxt->width, videoDecoderCxt->height, videoDecoderCxt->pix_fmt, videoDecoderCxt->width, videoDecoderCxt->height, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, 0, 0, 0);

    // Init resampler
    if(!wavPath.isEmpty())
    {
        avError = swr_alloc_set_opts2(&resamplerCxt, &audioDecoderCxt->ch_layout, AV_SAMPLE_FMT_S16, 44100, &audioDecoderCxt->ch_layout, audioDecoderCxt->sample_fmt, audioDecoderCxt->sample_rate, 0, 0);
        avError = swr_init(resamplerCxt);
    }

    // Allocate memory
    vPacket = av_packet_alloc();
    aPacket = av_packet_alloc();
    frameIn = av_frame_alloc();
    frameScaled = av_frame_alloc();
    frameFiltered = av_frame_alloc();
    frame = av_frame_alloc();

    if(!wavPath.isEmpty())
    {
        iAudioBuffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
        oAudioBuffer = (uint8_t*)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
        audioQueue = av_audio_fifo_alloc(AV_SAMPLE_FMT_S16, audioDecoderCxt->ch_layout.nb_channels, 4096);
    }

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
    SDL_DisplayMode display;
    SDL_GetDesktopDisplayMode(0, &display);
    window = SDL_CreateWindow("AVPStudio - MXLPlayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, display.w, (int)((double)AVPHeight * ((double)display.w / (double)AVPWidth)), 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, AVPWidth, AVPHeight);

    if(!wavPath.isEmpty())
    {
        wantedSpec.freq = 44100;
        wantedSpec.format = AUDIO_S16;
        wantedSpec.channels = audioDecoderCxt->ch_layout.nb_channels;
        wantedSpec.silence = 0;
        wantedSpec.samples = 1024;
        wantedSpec.callback = SDLFillAudio;
        wantedSpec.userdata = audioDecoderCxt;

        if(SDL_OpenAudio(&wantedSpec, 0) < 0)
        {
            emit showError(tr("无法打开音频设备。"));
            cleanup();
            return -1;
        }
        SDL_PauseAudio(1);
    }

    // Set info
    emit setPositionBarMax(videoFmtCxt->streams[videoStreamID]->duration, av_q2d(videoFmtCxt->streams[videoStreamID]->time_base));

    return 0;
}

void TPlayVideo::cleanup()
{
    SDL_CloseAudio();
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    avformat_close_input(&videoFmtCxt);
    avformat_free_context(videoFmtCxt);
    avformat_close_input(&audioFmtCxt);
    avformat_free_context(audioFmtCxt);

    avcodec_free_context(&videoDecoderCxt);
    avcodec_free_context(&audioDecoderCxt);

    avfilter_free(videoFilterSrcCxt);
    avfilter_free(videoFilterSinkCxt);

    avfilter_graph_free(&videoFilterGraph);
    avfilter_inout_free(&videoFilterInput);
    avfilter_inout_free(&videoFilterOutput);

    sws_freeContext(scalerCxt);
    swr_free(&resamplerCxt);

    av_packet_free(&vPacket);
    av_packet_free(&aPacket);
    av_frame_free(&frameIn);
    av_frame_free(&frameFiltered);
    av_frame_free(&frameScaled);
    av_frame_free(&frame);

    av_free(iAudioBuffer);
    av_free(oAudioBuffer);

    av_audio_fifo_free(audioQueue);
    SDL_DestroyMutex(audioQueueMutex);
    SDL_DestroyMutex(volumeMutex);
    SDL_DestroyMutex(refresherFlagMutex);
}

void TPlayVideo::notifyQuit()
{
    SDL_Event quitEvent;
    quitEvent.type = SDL_CUSTOM_QUIT_EVENT;
    SDL_PushEvent(&quitEvent);
}

int TPlayVideo::SDLAudioDecoderInternal(void *opaque)
{
    static int avError = 0;

    while(true)
    {
        SDL_LockMutex(refresherFlagMutex);
        if(refresherFlag != 0)
            continue;
        SDL_UnlockMutex(refresherFlagMutex);

        if(av_read_frame(audioFmtCxt, aPacket) == 0)
        {
            if(aPacket->stream_index == audioStreamID)
            {
                avError = avcodec_send_packet(audioDecoderCxt, aPacket);
                while(true)
                {
                    avError = avcodec_receive_frame(audioDecoderCxt, frame);
                    if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                        break;

                    iAudioBufferSampleCount = swr_convert(resamplerCxt, &iAudioBuffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t**)frame->data, frame->nb_samples);

                    iAudioBufferSize = av_samples_get_buffer_size(0, frame->ch_layout.nb_channels, iAudioBufferSampleCount, AV_SAMPLE_FMT_S16, 1);

                    while(iAudioBufferSampleCount > av_audio_fifo_space(audioQueue));
                    SDL_LockMutex(audioQueueMutex);
                    avError = av_audio_fifo_write(audioQueue, (void**)&iAudioBuffer, iAudioBufferSampleCount);
                    SDL_UnlockMutex(audioQueueMutex);

                    av_frame_unref(frame);
                }
                av_packet_unref(aPacket);
            }
        }
        else
            SDL_PauseAudio(1);
    }

    return 0;
}

void TPlayVideo::SDLFillAudioInternal(void *data, uint8_t *stream, int length)
{
    SDL_memset(stream, 0, length);
    SDL_LockMutex(audioQueueMutex);
    oAudioBufferSampleCount = av_audio_fifo_read(audioQueue, (void**)&oAudioBuffer, length / 4);
    SDL_UnlockMutex(audioQueueMutex);
    if(oAudioBufferSampleCount <= 0)
        return;
    oAudioBufferSize = av_samples_get_buffer_size(0, audioDecoderCxt->ch_layout.nb_channels, oAudioBufferSampleCount, AV_SAMPLE_FMT_S16, 1);
    if(oAudioBufferSize <= 0)
        return;
    SDL_LockMutex(volumeMutex);
    SDL_MixAudio(stream, oAudioBuffer, oAudioBufferSize, volume);
    SDL_UnlockMutex(volumeMutex);
}

void TPlayVideo::do_updatePosition(int val)
{
    newPosition = val;
    SDL_Event positionUpdateEvent;
    positionUpdateEvent.type = SDL_CUSTOM_POSITION_UPDATE_EVENT;
    SDL_PushEvent(&positionUpdateEvent);
}

void TPlayVideo::do_play()
{
    SDL_LockMutex(refresherFlagMutex);
    refresherFlag = 0;
    SDL_LockMutex(refresherFlagMutex);
    if(!wavPath.isEmpty())
        SDL_PauseAudio(0);
}

void TPlayVideo::do_pause()
{
    SDL_LockMutex(refresherFlagMutex);
    refresherFlag = 1;
    SDL_LockMutex(refresherFlagMutex);
    if(!wavPath.isEmpty())
        SDL_PauseAudio(1);
}

void TPlayVideo::do_volumeChanged(int val)
{
    SDL_LockMutex(volumeMutex);
    volume = val;
    SDL_UnlockMutex(volumeMutex);
}

void TPlayVideo::run()
{
    static int avError = 0;
    int frameDuration = (int) 1000 / av_q2d(videoDecoderCxt->framerate);
    threadRefresh = SDL_CreateThread(SDLRefresher, 0, &frameDuration);
    if(!wavPath.isEmpty())
        threadDecodeAudio = SDL_CreateThread(SDLAudioDecoder, 0, 0);

    while(true)
    {
        while(SDL_PollEvent(&eventSDL))
        {
            if(eventSDL.type == SDL_CUSTOM_REFRESH_EVENT)
            {
                if(av_read_frame(videoFmtCxt, vPacket) == 0)
                {
                    if(vPacket->stream_index == videoStreamID)
                    {
                        avError = avcodec_send_packet(videoDecoderCxt, vPacket);
                        while(true)
                        {
                            avError = avcodec_receive_frame(videoDecoderCxt, frameIn);
                            if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                                break;

                            emit setPosition(frameIn->pkt_dts);

                            sws_scale_frame(scalerCxt, frameScaled, frameIn);

                            avError = av_buffersrc_add_frame(videoFilterSrcCxt, frameScaled);
                            avError = av_buffersink_get_frame(videoFilterSinkCxt, frameFiltered);

                            SDL_UpdateYUVTexture(texture, 0, frameFiltered->data[0], frameFiltered->linesize[0], frameFiltered->data[1], frameFiltered->linesize[1], frameFiltered->data[2], frameFiltered->linesize[2]);
                            SDL_RenderClear(renderer);
                            SDL_RenderCopy(renderer, texture, 0, 0);
                            SDL_RenderPresent(renderer);

                            av_frame_unref(frameIn);
                            av_frame_unref(frameScaled);
                            av_frame_unref(frameFiltered);
                        }
                        av_packet_unref(vPacket);
                    }
                }
                else
                {
                    av_seek_frame(videoFmtCxt, videoStreamID, 0, AVSEEK_FLAG_BACKWARD);
                    if(!wavPath.isEmpty())
                    {
                        av_seek_frame(audioFmtCxt, audioStreamID, 0, AVSEEK_FLAG_BACKWARD);
                        SDL_PauseAudio(0);
                    }
                }
            }
            else if(eventSDL.type == SDL_CUSTOM_POSITION_UPDATE_EVENT)
            {
                av_seek_frame(videoFmtCxt, videoStreamID, newPosition, AVSEEK_FLAG_ANY);
                if(!wavPath.isEmpty())
                {
                    av_seek_frame(audioFmtCxt, audioStreamID, av_rescale_q(newPosition, videoFmtCxt->streams[videoStreamID]->time_base, audioFmtCxt->streams[audioStreamID]->time_base), AVSEEK_FLAG_ANY);
                    SDL_LockMutex(audioQueueMutex);
                    av_audio_fifo_reset(audioQueue);
                    SDL_UnlockMutex(audioQueueMutex);
                }
            }
            else if(eventSDL.type == SDL_CUSTOM_QUIT_EVENT)
            {
                SDL_LockMutex(refresherFlagMutex);
                refresherFlag = 2;
                SDL_UnlockMutex(refresherFlagMutex);
                cleanup();
                return;
            }
            else if(eventSDL.type == SDL_QUIT)
            {
                emit sdlQuit();
            }
        }
    }
}

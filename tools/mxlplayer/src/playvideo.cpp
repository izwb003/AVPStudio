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

#include <QDebug>

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

#define SDL_CUSTOM_REFRESH_EVENT (SDL_USEREVENT + 1)
#define SDL_CUSTOM_POSITION_UPDATE_EVENT (SDL_USEREVENT + 2)
#define SDL_CUSTOM_QUIT_EVENT (SDL_USEREVENT + 4)

static int avError = 0;

/*
 * Flag to control the behavior of the SDL refresher.
 * case 0: Play normally.
 * case 1: Paused.
 * case 2: Quit.
 */
static int refresherFlag = 1;

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
}

int TPlayVideo::init()
{
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

    // Get input video stream
    videoStreamID = av_find_best_stream(videoFmtCxt, AVMEDIA_TYPE_VIDEO, -1, -1, &videoDecoder, 0);
    if(videoStreamID == AVERROR_STREAM_NOT_FOUND)
    {
        emit showError(tr("打开MXL失败：不能找到流信息。"));
        cleanup();
        return avError;
    }

    if(videoFmtCxt->streams[videoStreamID]->codecpar->width != 3840 || videoFmtCxt->streams[videoStreamID]->codecpar->height !=2160)
    {
        emit showError(tr("打开MXL失败：不正确的视频尺寸。"));
        cleanup();
        return -1;
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

    // Allocate memory
    packet = av_packet_alloc();
    frameIn = av_frame_alloc();
    frameScaled = av_frame_alloc();
    frameFiltered = av_frame_alloc();

    // Init SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode display;
    SDL_GetDesktopDisplayMode(0, &display);
    window = SDL_CreateWindow("AVPStudio - MXLPlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, display.w, (int)((double)AVPHeight * ((double)display.w / (double)AVPWidth)), 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, AVPWidth, AVPHeight);

    // Set info
    emit setPositionBarMax(videoFmtCxt->streams[videoStreamID]->duration, av_q2d(videoFmtCxt->streams[videoStreamID]->time_base));

    return 0;
}

void TPlayVideo::cleanup()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    avformat_close_input(&videoFmtCxt);
    avformat_free_context(videoFmtCxt);

    avcodec_free_context(&videoDecoderCxt);

    avfilter_free(videoFilterSrcCxt);
    avfilter_free(videoFilterSinkCxt);

    avfilter_graph_free(&videoFilterGraph);
    avfilter_inout_free(&videoFilterInput);
    avfilter_inout_free(&videoFilterOutput);

    sws_freeContext(scalerCxt);

    av_packet_free(&packet);
    av_frame_free(&frameIn);
    av_frame_free(&frameFiltered);
    av_frame_free(&frameScaled);
}

void TPlayVideo::notifyQuit()
{
    SDL_Event quitEvent;
    quitEvent.type = SDL_CUSTOM_QUIT_EVENT;
    SDL_PushEvent(&quitEvent);
}

int TPlayVideo::SDLRefresher(void *opaque)
{
    SDL_Event refreshEvent;
    refreshEvent.type = SDL_CUSTOM_REFRESH_EVENT;
    while(true)
    {
        if(refresherFlag == 0)
        {
            SDL_PushEvent(&refreshEvent);
            SDL_Delay(*(int*)opaque);
        }
        else if(refresherFlag == 1)
            continue;
        else if(refresherFlag == 2)
            break;
    }
    return 0;
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
    refresherFlag = 0;
}

void TPlayVideo::do_pause()
{
    refresherFlag = 1;
}

void TPlayVideo::run()
{
    int frameDuration = (int) 1000 / av_q2d(videoDecoderCxt->framerate);
    thread = SDL_CreateThread(SDLRefresher, 0, &frameDuration);

    while(true)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_CUSTOM_REFRESH_EVENT)
            {
                if(av_read_frame(videoFmtCxt, packet) == 0)
                {
                    if(packet->stream_index == videoStreamID)
                    {
                        avError = avcodec_send_packet(videoDecoderCxt, packet);
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
                        av_packet_unref(packet);
                    }
                }
                else
                    av_seek_frame(videoFmtCxt, videoStreamID, 0, AVSEEK_FLAG_BACKWARD);
            }
            else if(event.type == SDL_CUSTOM_POSITION_UPDATE_EVENT)
                av_seek_frame(videoFmtCxt, videoStreamID, newPosition, AVSEEK_FLAG_ANY);
            else if(event.type == SDL_CUSTOM_QUIT_EVENT)
            {
                refresherFlag = 2;
                return;
            }
            else if(event.type == SDL_QUIT)
            {
                emit sdlQuit();
            }
        }
    }
}

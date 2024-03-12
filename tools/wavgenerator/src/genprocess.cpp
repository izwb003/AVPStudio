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
#include "genprocess.h"

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswresample/swresample.h>
}

TGenProcess::TGenProcess(QObject *parent, QString inputFilePath, QString outputFilePath, int volumePercent)
    : QThread{parent}
{
    this->inputFilePath = inputFilePath;
    this->outputFilePath = outputFilePath;
    this->volumePercent = volumePercent;
}

void TGenProcess::run()
{
    // FFmpeg init
    av_log_set_level(AV_LOG_QUIET);

    // Init variables
    static int avError = 0;

    AVFormatContext *iAudioFmtCxt = NULL;
    int iAudioStreamID = -1;
    const AVCodec *iAudioDecoder = NULL;
    AVCodecContext *iAudioDecoderCxt = NULL;

    AVPacket *packet = NULL;
    AVFrame *frameInput = NULL;
    AVFrame *frameFiltered = NULL;
    AVFrame *frameOutput = NULL;

    AVFilterGraph *volumeFilterGraph = NULL;

    const AVFilter *volumeFilter = NULL;
    AVFilterContext *volumeFilterCxt = NULL;

    const AVFilter *volumeFilterSrc = NULL;
    AVFilterContext *volumeFilterSrcCxt = NULL;
    const AVFilter *volumeFilterSink = NULL;
    AVFilterContext *volumeFilterSinkCxt = NULL;

    SwrContext *resamplerCxt = NULL;

    AVStream *oAudioStream = NULL;
    AVFormatContext *oAudioFmtCxt = NULL;
    const AVCodec *oAudioEncoder = NULL;
    AVCodecContext *oAudioEncoderCxt = NULL;

    // Open input file and find stream info
    iAudioFmtCxt = avformat_alloc_context();
    avError = avformat_open_input(&iAudioFmtCxt, this->inputFilePath.toUtf8(), 0, 0);
    if(avError < 0)
    {
        emit showError(tr("不能打开输入文件。"), tr("操作失败"));
        goto end;
    }
    avError = avformat_find_stream_info(iAudioFmtCxt, 0);
    if(avError < 0)
    {
        emit showError(tr("不能查找到流信息。"), tr("操作失败"));
        goto end;
    }
    iAudioStreamID = av_find_best_stream(iAudioFmtCxt, AVMEDIA_TYPE_AUDIO, -1, -1, &iAudioDecoder, 0);
    if(avError < 0)
    {
        emit showError(tr("不能找到音频流。"), tr("操作失败"));
        goto end;
    }

    // Open decoder
    iAudioDecoderCxt = avcodec_alloc_context3(iAudioDecoder);
    avError = avcodec_parameters_to_context(iAudioDecoderCxt, iAudioFmtCxt->streams[iAudioStreamID]->codecpar);
    if(avError < 0)
    {
        emit showError(tr("不能解析输入信息参数。"), tr("操作失败"));
        goto end;
    }
    avError = avcodec_open2(iAudioDecoderCxt, iAudioDecoder, 0);
    if(avError < 0)
    {
        emit showError(tr("不能打开解码器。"), tr("操作失败"));
        goto end;
    }

    // Init encoder
    oAudioEncoder = avcodec_find_encoder(AV_CODEC_ID_PCM_S24LE);
    oAudioEncoderCxt = avcodec_alloc_context3(oAudioEncoder);
    oAudioEncoderCxt -> time_base = {1, iAudioDecoderCxt->sample_rate};
    oAudioEncoderCxt -> ch_layout = iAudioDecoderCxt->ch_layout;
    oAudioEncoderCxt -> sample_fmt = AV_SAMPLE_FMT_S32;
    oAudioEncoderCxt -> sample_rate = iAudioDecoderCxt->sample_rate;

    // Create output format and stream
    avError = avformat_alloc_output_context2(&oAudioFmtCxt, 0, 0, this->outputFilePath.toUtf8());
    oAudioStream = avformat_new_stream(oAudioFmtCxt, 0);
    avError = avcodec_parameters_from_context(oAudioStream->codecpar, oAudioEncoderCxt);
    oAudioStream -> time_base = oAudioEncoderCxt->time_base;

    // Open encoder/file and write file headers
    avError = avcodec_open2(oAudioEncoderCxt, oAudioEncoder, 0);
    if(avError < 0)
    {
        emit showError(tr("不能打开编码器。"), tr("操作失败"));
        goto end;
    }
    avError = avio_open(&oAudioFmtCxt->pb, this->outputFilePath.toUtf8(), AVIO_FLAG_WRITE);
    if(avError < 0)
    {
        emit showError(tr("不能打开输出文件I/O。"), tr("操作失败"));
        goto end;
    }
    avError = avformat_write_header(oAudioFmtCxt, 0);
    if(avError < 0)
    {
        emit showError(tr("不能写入输出文件头。"), tr("操作失败"));
        goto end;
    }

    // Begin conversion
    packet = av_packet_alloc();
    frameInput = av_frame_alloc();
    frameFiltered = av_frame_alloc();
    frameOutput = av_frame_alloc();

    emit setProgressMax(iAudioFmtCxt->streams[iAudioStreamID]->duration * av_q2d(iAudioFmtCxt->streams[iAudioStreamID]->time_base));

    // Set volume filter
    char chLayoutDescription[64];

    volumeFilterGraph = avfilter_graph_alloc();

    volumeFilterSrc = avfilter_get_by_name("abuffer");
    volumeFilterSrcCxt = avfilter_graph_alloc_filter(volumeFilterGraph, volumeFilterSrc, "in");
    avError = av_channel_layout_describe(&iAudioDecoderCxt->ch_layout, chLayoutDescription, sizeof(chLayoutDescription));
    avError = av_opt_set(volumeFilterSrcCxt, "channel_layout", chLayoutDescription, AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set(volumeFilterSrcCxt, "sample_fmt", av_get_sample_fmt_name(iAudioDecoderCxt->sample_fmt), AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set_q(volumeFilterSrcCxt, "time_base", iAudioDecoderCxt->time_base, AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set_int(volumeFilterSrcCxt, "sample_rate", iAudioDecoderCxt->sample_rate, AV_OPT_SEARCH_CHILDREN);
    avError = avfilter_init_str(volumeFilterSrcCxt, 0);

    volumeFilter = avfilter_get_by_name("volume");
    volumeFilterCxt = avfilter_graph_alloc_filter(volumeFilterGraph, volumeFilter, "volume");
    avError = av_opt_set(volumeFilterCxt, "volume", QString::number(volumePercent / 100.0, 'f', 2).toUtf8(), AV_OPT_SEARCH_CHILDREN);
    avError = avfilter_init_str(volumeFilterCxt, 0);

    volumeFilterSink = avfilter_get_by_name("abuffersink");
    volumeFilterSinkCxt = avfilter_graph_alloc_filter(volumeFilterGraph, volumeFilterSink, "out");
    avError = av_channel_layout_describe(&iAudioDecoderCxt->ch_layout, chLayoutDescription, sizeof(chLayoutDescription));
    avError = av_opt_set(volumeFilterSinkCxt, "channel_layout", chLayoutDescription, AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set(volumeFilterSinkCxt, "sample_fmt", av_get_sample_fmt_name(iAudioDecoderCxt->sample_fmt), AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set_q(volumeFilterSinkCxt, "time_base", iAudioDecoderCxt->time_base, AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set_int(volumeFilterSinkCxt, "sample_rate", iAudioDecoderCxt->sample_rate, AV_OPT_SEARCH_CHILDREN);
    avError = avfilter_init_str(volumeFilterSinkCxt, 0);

    avError = avfilter_link(volumeFilterSrcCxt, 0, volumeFilterCxt, 0);
    avError = avfilter_link(volumeFilterCxt, 0, volumeFilterSinkCxt, 0);
    avError = avfilter_graph_config(volumeFilterGraph, 0);

    // Set resampler
    avError = swr_alloc_set_opts2(&resamplerCxt, &iAudioDecoderCxt->ch_layout, AV_SAMPLE_FMT_S32, iAudioDecoderCxt->sample_rate, &iAudioDecoderCxt->ch_layout, iAudioDecoderCxt->sample_fmt, iAudioDecoderCxt->sample_rate, 0, 0);
    avError = swr_init(resamplerCxt);

    while(av_read_frame(iAudioFmtCxt, packet) == 0)
    {
        if(packet->stream_index == iAudioStreamID)
        {
            avError = avcodec_send_packet(iAudioDecoderCxt, packet);
            while(true)
            {
                avError = avcodec_receive_frame(iAudioDecoderCxt, frameInput);
                if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                    break;

                emit setProgress(frameInput->pkt_dts * av_q2d(iAudioFmtCxt->streams[iAudioStreamID]->time_base));

                // Copy frame settings
                frameOutput -> ch_layout = frameInput -> ch_layout;
                frameOutput -> sample_rate = frameInput -> sample_rate;
                frameOutput -> format = AV_SAMPLE_FMT_S32;
                frameOutput -> nb_samples = av_rescale_rnd(swr_get_delay(resamplerCxt, iAudioDecoderCxt->sample_rate) + frameInput->nb_samples, iAudioDecoderCxt->sample_rate, iAudioDecoderCxt->sample_rate, AV_ROUND_UP);

                // Apply volume filter
                avError = av_buffersrc_add_frame(volumeFilterSrcCxt, frameInput);
                avError = av_buffersink_get_frame(volumeFilterSinkCxt, frameFiltered);

                // Resample
                avError = swr_config_frame(resamplerCxt, frameOutput, frameFiltered);
                avError = swr_convert_frame(resamplerCxt, frameOutput, frameFiltered);

                // Encode
                avError = avcodec_send_frame(oAudioEncoderCxt, frameOutput);
                avError = avcodec_receive_packet(oAudioEncoderCxt, packet);
                avError = av_write_frame(oAudioFmtCxt, packet);
            }
        }
    }

    // Write file tail
    avError = av_write_trailer(oAudioFmtCxt);
    if(avError < 0)
    {
        emit showError(tr("不能写入输出文件尾。"), tr("操作失败"));
        goto end;
    }

    // Close files
    avformat_close_input(&iAudioFmtCxt);
    avio_close(oAudioFmtCxt->pb);

    avError = 0;

end:    // Jump flag for errors

    // Free memory
    avformat_free_context(iAudioFmtCxt);
    avformat_free_context(oAudioFmtCxt);

    avcodec_free_context(&iAudioDecoderCxt);
    avcodec_free_context(&oAudioEncoderCxt);

    av_packet_free(&packet);

    av_frame_free(&frameInput);
    av_frame_free(&frameFiltered);
    av_frame_free(&frameOutput);

    avfilter_graph_free(&volumeFilterGraph);

    swr_free(&resamplerCxt);
}

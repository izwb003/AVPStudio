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
#include "doprocess.h"

#include "settings.h"

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
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

// FFmpeg filter graph description
static const char *filterGraphLarge =
    "[in]pad[expanded];"
    "[expanded]scale=6166:1080[scaled];"
    "[scaled]split[scaled1][scaled2];"
    "[scaled1]crop=3840:1080:0:0[left];"
    "[scaled2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";
static const char *filterGraphMedium =
    "[in]pad[expanded];"
    "[expanded]scale=4632:1080[scaled];"
    "[scaled]pad=6166:1080:767:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";
static const char *filterGraphSmall =
    "[in]pad[expanded];"
    "[expanded]scale=2830:1080[scaled];"
    "[scaled]pad=6166:1080:1668:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";

TDoProcess::TDoProcess(QObject *parent) {}

void TDoProcess::run()
{
    // FFmpeg init
    av_log_set_level(AV_LOG_ERROR);

    // Init variables
    static int avError = 0;
    QString avErrorMsg;

    AVFormatContext *iVideoFmtCxt = NULL;

    int iVideoStreamID = -1;
    int iAudioStreamID = -1;

    const AVCodec *iVideoDecoder = NULL;
    const AVCodec *iAudioDecoder = NULL;
    AVCodecContext *iVideoDecoderCxt = NULL;
    AVCodecContext *iAudioDecoderCxt = NULL;

    const AVCodec *oVideoEncoder = NULL;
    const AVCodec *oAudioEncoder = NULL;
    AVCodecContext *oVideoEncoderCxt = NULL;
    AVCodecContext *oAudioEncoderCxt = NULL;

    AVFormatContext *oVideoFmtCxt = NULL;
    AVFormatContext *oAudioFmtCxt = NULL;

    AVStream *oVideoStream = NULL;
    AVStream *oAudioStream = NULL;

    AVPacket *packet = NULL;

    char filterArgs[512] = {0};

    AVFrame *vFrameIn = NULL;
    AVFrame *vFrameFiltered = NULL;
    AVFrame *vFrameOut = NULL;

    AVFilterGraph *videoFilterGraph = NULL;
    AVFilterInOut *videoFilterInput = NULL;
    AVFilterInOut *videoFilterOutput = NULL;

    const AVFilter *videoFilterSrc = NULL;
    AVFilterContext *videoFilterSrcCxt = NULL;
    const AVFilter *videoFilterSink = NULL;
    AVFilterContext *videoFilterSinkCxt = NULL;

    SwsContext *scale422Cxt = NULL;

    AVFrame *aFrameIn = NULL;
    AVFrame *aFrameFiltered = NULL;
    AVFrame *aFrameOut = NULL;

    AVFilterGraph *volumeFilterGraph = NULL;

    const AVFilter *volumeFilter = NULL;
    AVFilterContext *volumeFilterCxt = NULL;

    const AVFilter *volumeFilterSrc = NULL;
    AVFilterContext *volumeFilterSrcCxt = NULL;
    const AVFilter *volumeFilterSink = NULL;
    AVFilterContext *volumeFilterSinkCxt = NULL;

    SwrContext *resamplerCxt = NULL;

    // Open input file and find stream info
    iVideoFmtCxt = avformat_alloc_context();
    avError = avformat_open_input(&iVideoFmtCxt, settings.inputVideoPath.toLocal8Bit(), 0, 0);
    avError = avformat_find_stream_info(iVideoFmtCxt, 0);

    // Get input video and audio stream
    iVideoStreamID = av_find_best_stream(iVideoFmtCxt, AVMEDIA_TYPE_VIDEO, -1, -1, &iVideoDecoder, 0);
    iAudioStreamID = av_find_best_stream(iVideoFmtCxt, AVMEDIA_TYPE_AUDIO, -1, -1, &iAudioDecoder, 0);

    // Open decoder
    iVideoDecoderCxt = avcodec_alloc_context3(iVideoDecoder);
    avError = avcodec_parameters_to_context(iVideoDecoderCxt, iVideoFmtCxt->streams[iVideoStreamID]->codecpar);
    avError = avcodec_open2(iVideoDecoderCxt, iVideoDecoder, 0);

    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        iAudioDecoderCxt = avcodec_alloc_context3(iAudioDecoder);
        avError = avcodec_parameters_to_context(iAudioDecoderCxt, iVideoFmtCxt->streams[iAudioStreamID]->codecpar);
        avError = avcodec_open2(iAudioDecoderCxt, iAudioDecoder, 0);
    }

    // Init encoder
    oVideoEncoder = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);
    oVideoEncoderCxt = avcodec_alloc_context3(oVideoEncoder);
    oVideoEncoderCxt -> time_base = av_inv_q(settings.outputFrameRate);
    oVideoEncoderCxt -> width = 3840;
    oVideoEncoderCxt -> height = 2160;
    oVideoEncoderCxt -> bit_rate = settings.outputVideoBitRate * 1000000;
    oVideoEncoderCxt -> rc_max_rate = oVideoEncoderCxt->bit_rate;
    oVideoEncoderCxt -> rc_min_rate = oVideoEncoderCxt->bit_rate;
    oVideoEncoderCxt -> rc_buffer_size = oVideoEncoderCxt->bit_rate / 2;
    oVideoEncoderCxt -> bit_rate_tolerance = 0;
    oVideoEncoderCxt -> pix_fmt = AV_PIX_FMT_YUV422P;
    oVideoEncoderCxt -> color_primaries = settings.outputColor.outputColorPrimary;
    oVideoEncoderCxt -> colorspace = settings.outputColor.outputVideoColorSpace;
    oVideoEncoderCxt -> color_trc = settings.outputColor.outputVideoColorTrac;
    oVideoEncoderCxt -> profile = 0;
    oVideoEncoderCxt -> max_b_frames = 0;

    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        oAudioEncoder = avcodec_find_encoder(AV_CODEC_ID_PCM_S24LE);
        oAudioEncoderCxt = avcodec_alloc_context3(oAudioEncoder);
        oAudioEncoderCxt -> time_base = {1, iAudioDecoderCxt->sample_rate};
        oAudioEncoderCxt -> ch_layout = iAudioDecoderCxt->ch_layout;
        oAudioEncoderCxt -> sample_fmt = AV_SAMPLE_FMT_S32;
        oAudioEncoderCxt -> sample_rate = iAudioDecoderCxt->sample_rate;
    }

    // Create output format and stream
    avError = avformat_alloc_output_context2(&oVideoFmtCxt, av_guess_format("mpeg2video", 0, 0), 0, QString(settings.outputFilePath + "/" + settings.getOutputVideoFinalName()).toLocal8Bit());
    oVideoStream = avformat_new_stream(oVideoFmtCxt, 0);
    avError = avcodec_parameters_from_context(oVideoStream->codecpar, oVideoEncoderCxt);
    oVideoStream -> time_base = oVideoEncoderCxt->time_base;
    oVideoStream ->r_frame_rate = settings.outputFrameRate;

    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        avError = avformat_alloc_output_context2(&oAudioFmtCxt, 0, 0, QString(settings.outputFilePath + "/" + settings.getOutputAudioFinalName()).toLocal8Bit());
        oAudioStream = avformat_new_stream(oAudioFmtCxt, 0);
        avError = avcodec_parameters_from_context(oAudioStream->codecpar, oAudioEncoderCxt);
        oAudioStream -> time_base = oAudioEncoderCxt->time_base;
    }

    // Open encoder/file and write file headers
    avError = avcodec_open2(oVideoEncoderCxt, oVideoEncoder, 0);
    avError = avio_open(&oVideoFmtCxt->pb, QString(settings.outputFilePath + "/" + settings.getOutputVideoFinalName()).toLocal8Bit(), AVIO_FLAG_WRITE);
    avError = avformat_write_header(oVideoFmtCxt, 0);

    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        avError = avcodec_open2(oAudioEncoderCxt, oAudioEncoder, 0);
        avError = avio_open(&oAudioFmtCxt->pb, QString(settings.outputFilePath + "/" + settings.getOutputAudioFinalName()).toLocal8Bit(), AVIO_FLAG_WRITE);
        avError = avformat_write_header(oAudioFmtCxt, 0);
    }

    // Begin conversion
    packet = av_packet_alloc();

    // Convert video
    vFrameIn = av_frame_alloc();
    vFrameFiltered = av_frame_alloc();
    vFrameOut = av_frame_alloc();

    emit setLabel(tr("转换视频中...") + settings.getOutputVideoFinalName());
    emit setProgressMax(iVideoFmtCxt->streams[iVideoStreamID]->duration * av_q2d(iVideoFmtCxt->streams[iVideoStreamID]->time_base));

    // Set video filter
    videoFilterGraph = avfilter_graph_alloc();

    videoFilterSrc = avfilter_get_by_name("buffer");
    char videoFilterSrcArgs[512];
    snprintf(videoFilterSrcArgs, sizeof(videoFilterSrcArgs), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", iVideoDecoderCxt->width, iVideoDecoderCxt->height, iVideoDecoderCxt->pix_fmt, iVideoFmtCxt->streams[iVideoStreamID]->time_base.num, iVideoFmtCxt->streams[iVideoStreamID]->time_base.den, iVideoDecoderCxt->sample_aspect_ratio.num, iVideoDecoderCxt->sample_aspect_ratio.den);
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

    switch(settings.size)
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

    // Set YUV422 rescaler
    scale422Cxt = sws_getContext(3840, 2160, iVideoDecoderCxt->pix_fmt, 3840, 2160, AV_PIX_FMT_YUV422P, SWS_FAST_BILINEAR, 0, 0, 0);

    while(av_read_frame(iVideoFmtCxt, packet) == 0)
    {
        if(packet->stream_index == iVideoStreamID)
        {
            avError = avcodec_send_packet(iVideoDecoderCxt, packet);
            while(true)
            {
                avError = avcodec_receive_frame(iVideoDecoderCxt, vFrameIn);
                if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                    break;

                emit setProgress(vFrameIn->pkt_dts * av_q2d(iVideoFmtCxt->streams[iVideoStreamID]->time_base));

                // Apply filter
                avError = av_buffersrc_add_frame(videoFilterSrcCxt, vFrameIn);
                avError = av_buffersink_get_frame(videoFilterSinkCxt, vFrameFiltered);

                // Rescale to YUV422
                avError = sws_scale_frame(scale422Cxt, vFrameOut, vFrameFiltered);

                // Encode
                avError = avcodec_send_frame(oVideoEncoderCxt, vFrameOut);
                avError = avcodec_receive_packet(oVideoEncoderCxt, packet);
                av_packet_rescale_ts(packet, oVideoEncoderCxt->time_base, oVideoFmtCxt->streams[0]->time_base);
                avError = av_write_frame(oVideoFmtCxt, packet);
            }
        }
    }

    av_seek_frame(iVideoFmtCxt, iVideoStreamID, 0, AVSEEK_FLAG_BACKWARD);

    // Convert audio
    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        aFrameIn = av_frame_alloc();
        aFrameFiltered = av_frame_alloc();
        aFrameOut = av_frame_alloc();

        emit setLabel(tr("转换音频中...") + settings.getOutputAudioFinalName());
        emit setProgressMax(iVideoFmtCxt->streams[iAudioStreamID]->duration * av_q2d(iVideoFmtCxt->streams[iAudioStreamID]->time_base));

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
        avError = av_opt_set(volumeFilterCxt, "volume", QString::number(settings.outputVolume / 100.0, 'f', 2).toLocal8Bit(), AV_OPT_SEARCH_CHILDREN);
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
        avError = swr_alloc_set_opts2(&resamplerCxt, &iAudioDecoderCxt->ch_layout, AV_SAMPLE_FMT_S32, 48000, &iAudioDecoderCxt->ch_layout, iAudioDecoderCxt->sample_fmt, iAudioDecoderCxt->sample_rate, 0, 0);
        avError = swr_init(resamplerCxt);

        while(av_read_frame(iVideoFmtCxt, packet) == 0)
        {
            if(packet->stream_index == iAudioStreamID)
            {
                avError = avcodec_send_packet(iAudioDecoderCxt, packet);
                while(true)
                {
                    avError = avcodec_receive_frame(iAudioDecoderCxt, aFrameIn);
                    if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                        break;

                    emit setProgress(aFrameIn->pkt_dts * av_q2d(iVideoFmtCxt->streams[iAudioStreamID]->time_base));

                    // Copy frame settings
                    aFrameOut -> ch_layout = aFrameIn -> ch_layout;
                    aFrameOut -> sample_rate = aFrameIn -> sample_rate;
                    aFrameOut -> format = AV_SAMPLE_FMT_S32;
                    aFrameOut -> nb_samples = av_rescale_rnd(swr_get_delay(resamplerCxt, 48000) + aFrameIn->nb_samples, 48000, iAudioDecoderCxt->sample_rate, AV_ROUND_UP);
                    aFrameOut -> pts = aFrameIn -> pts;

                    // Apply volume filter
                    avError = av_buffersrc_add_frame(volumeFilterSrcCxt, aFrameIn);
                    avError = av_buffersink_get_frame(volumeFilterSinkCxt, aFrameFiltered);

                    // Resample
                    avError = swr_convert_frame(resamplerCxt, aFrameOut, aFrameFiltered);

                    // Encode
                    avError = avcodec_send_frame(oAudioEncoderCxt, aFrameOut);
                    avError = avcodec_receive_packet(oAudioEncoderCxt, packet);
                    avError = av_write_frame(oAudioFmtCxt, packet);
                }
            }
        }
    }

    // Write file tail
    av_write_trailer(oVideoFmtCxt);
    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
        av_write_trailer(oAudioFmtCxt);

    // Close files
    avformat_close_input(&iVideoFmtCxt);

    avio_close(oVideoFmtCxt->pb);
    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
        avio_close(oAudioFmtCxt->pb);

    emit completed(avError, avErrorMsg);
}

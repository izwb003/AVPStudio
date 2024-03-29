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

/* To anyone who reads my code:
 * I am not very familiar with multi-thread programming.
 * I was sleeping when my teacher taught me these things.
 * So I may have made some foolish mistakes in this regard, such as using terminate() extensively.
 * If I have the opportunity to continue maintaining these codes in the future, perhaps I can use more enriched experience to correct them.
 * But now, they are "usable" - limited to being "able to run", that's all.
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
    "[in]pad=iw:ih:0:0:black[expanded];"
    "[expanded]scale=6166:1080[scaled];"
    "[scaled]split[scaled1][scaled2];"
    "[scaled1]crop=3840:1080:0:0[left];"
    "[scaled2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[ready];"
    "[ready]fps=24[out]";
static const char *filterGraphMedium =
    "[in]pad=iw:ih:0:0:black[expanded];"
    "[expanded]scale=4632:1080[scaled];"
    "[scaled]pad=6166:1080:767:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[ready];"
    "[ready]fps=24[out]";
static const char *filterGraphSmall =
    "[in]pad=iw:ih:0:0:black[expanded];"
    "[expanded]scale=2830:1080[scaled];"
    "[scaled]pad=6166:1080:1668:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[ready];"
    "[ready]fps=24[out]";

template<typename T> int toUpperInt(T val)
{
    if((int)val % 2 == 1)
        return (int)val + 1;
    else
        return (int)val;
}

TDoProcess::TDoProcess(QObject *parent) {}

void TDoProcess::run()
{
    // FFmpeg init
    av_log_set_level(AV_LOG_QUIET);

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

    AVFilterContext *videoFilterPadCxt = NULL;
    AVFilterContext *videoFilterFpsCxt = NULL;

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

    uint64_t audioPTSCounter = 0;

    // Open input file and find stream info
    iVideoFmtCxt = avformat_alloc_context();
    avError = avformat_open_input(&iVideoFmtCxt, settings.inputVideoPath.toUtf8(), 0, 0);
    if(avError < 0)
    {
        avErrorMsg = tr("加载输入文件失败：打开视频文件出错。");
        goto end;
    }
    avError = avformat_find_stream_info(iVideoFmtCxt, 0);
    if(avError < 0)
    {
        avErrorMsg = tr("加载输入文件失败：不能找到视频流信息。");
        goto end;
    }

    // Get input video and audio stream
    iVideoStreamID = av_find_best_stream(iVideoFmtCxt, AVMEDIA_TYPE_VIDEO, -1, -1, &iVideoDecoder, 0);
    if(iVideoStreamID == AVERROR_STREAM_NOT_FOUND)
    {
        avErrorMsg = tr("加载输入文件失败：找不到视频流。");
        goto end;
    }
    iAudioStreamID = av_find_best_stream(iVideoFmtCxt, AVMEDIA_TYPE_AUDIO, -1, -1, &iAudioDecoder, 0);

    // Open decoder
    iVideoDecoderCxt = avcodec_alloc_context3(iVideoDecoder);
    avError = avcodec_parameters_to_context(iVideoDecoderCxt, iVideoFmtCxt->streams[iVideoStreamID]->codecpar);
    if(avError < 0)
    {
        avErrorMsg = tr("加载输入文件失败：没有对应的视频解码器。");
        goto end;
    }
    avError = avcodec_open2(iVideoDecoderCxt, iVideoDecoder, 0);
    if(avError < 0)
    {
        avErrorMsg = tr("加载输入文件失败：无法打开视频解码器。");
        goto end;
    }

    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        iAudioDecoderCxt = avcodec_alloc_context3(iAudioDecoder);
        avError = avcodec_parameters_to_context(iAudioDecoderCxt, iVideoFmtCxt->streams[iAudioStreamID]->codecpar);
        if(avError < 0)
        {
            avErrorMsg = tr("加载输入文件失败：没有对应的音频解码器。");
            goto end;
        }
        avError = avcodec_open2(iAudioDecoderCxt, iAudioDecoder, 0);
        if(avError < 0)
        {
            avErrorMsg = tr("加载输入文件失败：无法打开音频解码器。");
            goto end;
        }
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
    oVideoEncoderCxt -> framerate = settings.outputFrameRate;

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
    avError = avformat_alloc_output_context2(&oVideoFmtCxt, av_guess_format("mpeg2video", 0, 0), 0, QString(settings.outputFilePath + "/" + settings.getOutputVideoFinalName()).toUtf8());
    if(avError < 0)
    {
        avErrorMsg = tr("写入视频输出文件失败：无法创建输出上下文。");
        goto end;
    }
    oVideoStream = avformat_new_stream(oVideoFmtCxt, 0);
    avError = avcodec_parameters_from_context(oVideoStream->codecpar, oVideoEncoderCxt);
    if(avError < 0)
    {
        avErrorMsg = tr("写入视频输出文件失败：无法解析输出上下文。");
        goto end;
    }
    oVideoStream -> time_base = oVideoEncoderCxt->time_base;
    oVideoStream -> r_frame_rate = settings.outputFrameRate;

    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        avError = avformat_alloc_output_context2(&oAudioFmtCxt, 0, 0, QString(settings.outputFilePath + "/" + settings.getOutputAudioFinalName()).toUtf8());
        if(avError < 0)
        {
            avErrorMsg = tr("写入音频输出文件失败：无法创建输出上下文。");
            goto end;
        }
        oAudioStream = avformat_new_stream(oAudioFmtCxt, 0);
        avError = avcodec_parameters_from_context(oAudioStream->codecpar, oAudioEncoderCxt);
        if(avError < 0)
        {
            avErrorMsg = tr("写入音频输出文件失败：无法解析输出上下文。");
            goto end;
        }
        oAudioStream -> time_base = oAudioEncoderCxt->time_base;
    }

    // Open encoder/file and write file headers
    avError = avcodec_open2(oVideoEncoderCxt, oVideoEncoder, 0);
    if(avError < 0)
    {
        avErrorMsg = tr("写入视频输出文件失败：无法打开视频编码器。");
        goto end;
    }
    avError = avio_open(&oVideoFmtCxt->pb, QString(settings.outputFilePath + "/" + settings.getOutputVideoFinalName()).toUtf8(), AVIO_FLAG_WRITE);
    if(avError < 0)
    {
        avErrorMsg = tr("写入视频输出文件失败：无法打开视频输出I/O。");
        goto end;
    }
    avError = avformat_write_header(oVideoFmtCxt, 0);
    if(avError < 0)
    {
        avErrorMsg = tr("写入视频输出文件失败：无法写入文件头。");
        goto end;
    }

    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        avError = avcodec_open2(oAudioEncoderCxt, oAudioEncoder, 0);
        if(avError < 0)
        {
            avErrorMsg = tr("写入音频输出文件失败：无法打开音频编码器。");
            goto end;
        }
        avError = avio_open(&oAudioFmtCxt->pb, QString(settings.outputFilePath + "/" + settings.getOutputAudioFinalName()).toUtf8(), AVIO_FLAG_WRITE);
        if(avError < 0)
        {
            avErrorMsg = tr("写入音频输出文件失败：无法打开音频输出I/O。");
            goto end;
        }
        avError = avformat_write_header(oAudioFmtCxt, 0);
        if(avError < 0)
        {
            avErrorMsg = tr("写入音频输出文件失败：无法写入文件头。");
            goto end;
        }
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

    videoFilterPadCxt = avfilter_graph_get_filter(videoFilterGraph, "Parsed_pad_0");

    /*
     * Special note to this fix:
     * Although most video tools will generate videos that has an even width/height, some of the videos may have an odd width/height.
     * The "pad" filter receives an odd size and automatically rounds down to an even number. If the rounded size is smaller than the size of the input image, the filter system will throw an exception.
     * So it is necessary to manually adjust the parameters of the incoming "pad" filter to accept even data with a larger size than the input content.
     */
    avError = av_opt_set(videoFilterPadCxt, "width", QString::number(toUpperInt(iVideoDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set(videoFilterPadCxt, "height", QString::number(toUpperInt(iVideoDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
    if(iVideoDecoderCxt->width == settings.getWidth() && iVideoDecoderCxt->height == 1080)
        settings.scalePicture = false;

    if(!settings.scalePicture)
    {
        if((iVideoDecoderCxt->width / iVideoDecoderCxt->height) < (settings.getWidth() / 1080))
        {
            switch(settings.size)
            {
            case AVP::kAVPLargeSize:
                avError = av_opt_set(videoFilterPadCxt, "width", QString::number(toUpperInt(iVideoDecoderCxt->height * 5.71)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "height", QString::number(toUpperInt(iVideoDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "x", QString::number(toUpperInt(((iVideoDecoderCxt->height * 5.71) / 2) - (iVideoDecoderCxt->width / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPMediumSize:
                avError = av_opt_set(videoFilterPadCxt, "width", QString::number(toUpperInt(iVideoDecoderCxt->height * 4.29)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "height", QString::number(toUpperInt(iVideoDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "x", QString::number(toUpperInt(((iVideoDecoderCxt->height * 4.29) / 2) - (iVideoDecoderCxt->width / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPSmallSize:
                avError = av_opt_set(videoFilterPadCxt, "width", QString::number(toUpperInt(iVideoDecoderCxt->height * 2.62)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "height", QString::number(toUpperInt(iVideoDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "x", QString::number(toUpperInt(((iVideoDecoderCxt->height * 2.62) / 2) - (iVideoDecoderCxt->width / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            }
        }
        else if((iVideoDecoderCxt->width / iVideoDecoderCxt->height) > (settings.getWidth() / 1080))
        {
            switch(settings.size)
            {
            case AVP::kAVPLargeSize:
                avError = av_opt_set(videoFilterPadCxt, "width", QString::number(toUpperInt(iVideoDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "height", QString::number(toUpperInt(iVideoDecoderCxt->width * 0.175)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "y", QString::number(toUpperInt(((iVideoDecoderCxt->width * 0.175) / 2) - (iVideoDecoderCxt->height / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPMediumSize:
                avError = av_opt_set(videoFilterPadCxt, "width", QString::number(toUpperInt(iVideoDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "height", QString::number(toUpperInt(iVideoDecoderCxt->width * 0.233)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "y", QString::number(toUpperInt(((iVideoDecoderCxt->width * 0.233) / 2) - (iVideoDecoderCxt->height / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPSmallSize:
                avError = av_opt_set(videoFilterPadCxt, "width", QString::number(toUpperInt(iVideoDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "height", QString::number(toUpperInt(iVideoDecoderCxt->width * 0.382)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(videoFilterPadCxt, "y", QString::number(toUpperInt(((iVideoDecoderCxt->width * 0.382) / 2) - (iVideoDecoderCxt->height / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            }
        }
    }

    if(settings.size == AVP::kAVPMediumSize || settings.size == AVP::kAVPMediumSize)
        videoFilterFpsCxt = avfilter_graph_get_filter(videoFilterGraph, "Parsed_fps_7");
    if(settings.size == AVP::kAVPLargeSize)
        videoFilterFpsCxt = avfilter_graph_get_filter(videoFilterGraph, "Parsed_fps_6");
    avError = av_opt_set(videoFilterFpsCxt, "fps", QString::number(settings.outputFrameRate.num).toUtf8() + "/" + QString::number(settings.outputFrameRate.den).toUtf8(), AV_OPT_SEARCH_CHILDREN);

    avError = avfilter_graph_config(videoFilterGraph, 0);
    if(avError < 0)
    {
        avErrorMsg = tr("转换失败：不能创建滤镜链。");
        goto end;
    }

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
                while(true)
                {
                    avError = av_buffersink_get_frame(videoFilterSinkCxt, vFrameFiltered);
                    if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                        break;

                    // Rescale to YUV422
                    avError = sws_scale_frame(scale422Cxt, vFrameOut, vFrameFiltered);

                    // Encode
                    avError = avcodec_send_frame(oVideoEncoderCxt, vFrameOut);
                    avError = avcodec_receive_packet(oVideoEncoderCxt, packet);
                    av_packet_rescale_ts(packet, oVideoEncoderCxt->time_base, oVideoFmtCxt->streams[0]->time_base);
                    avError = av_interleaved_write_frame(oVideoFmtCxt, packet);

                    // Unref frame
                    av_frame_unref(vFrameIn);
                    av_frame_unref(vFrameFiltered);
                    av_frame_unref(vFrameOut);
                }
            }
            // Unref packet
            av_packet_unref(packet);
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
        avError = av_opt_set(volumeFilterCxt, "volume", QString::number(settings.outputVolume / 100.0, 'f', 2).toUtf8(), AV_OPT_SEARCH_CHILDREN);
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
                    aFrameOut -> nb_samples = av_rescale_rnd(swr_get_delay(resamplerCxt, iAudioDecoderCxt->sample_rate) + aFrameIn->nb_samples, iAudioDecoderCxt->sample_rate, iAudioDecoderCxt->sample_rate, AV_ROUND_UP);

                    // Apply volume filter
                    avError = av_buffersrc_add_frame(volumeFilterSrcCxt, aFrameIn);
                    avError = av_buffersink_get_frame(volumeFilterSinkCxt, aFrameFiltered);

                    // Resample
                    avError = swr_config_frame(resamplerCxt, aFrameOut, aFrameFiltered);
                    avError = swr_convert_frame(resamplerCxt, aFrameOut, aFrameFiltered);

                    aFrameOut -> pts = audioPTSCounter;
                    audioPTSCounter += oAudioEncoderCxt->frame_size;

                    // Encode
                    avError = avcodec_send_frame(oAudioEncoderCxt, aFrameOut);
                    avError = avcodec_receive_packet(oAudioEncoderCxt, packet);
                    avError = av_write_frame(oAudioFmtCxt, packet);

                    // Unref frames
                    av_frame_unref(aFrameIn);
                    av_frame_unref(aFrameFiltered);
                    av_frame_unref(aFrameOut);
                }
                // Unref packet
                av_packet_unref(packet);
            }
        }
    }

    // Write file tail
    avError = av_write_trailer(oVideoFmtCxt);
    if(avError < 0)
    {
        avErrorMsg = tr("写入视频输出文件失败：无法写入文件尾。");
        goto end;
    }
    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
    {
        avError = av_write_trailer(oAudioFmtCxt);
        if(avError < 0)
        {
            avErrorMsg = tr("写入音频输出文件失败：无法写入文件尾。");
            goto end;
        }
    }

    // Close files
    avformat_close_input(&iVideoFmtCxt);

    avio_close(oVideoFmtCxt->pb);
    if(iAudioStreamID != AVERROR_STREAM_NOT_FOUND)
        avio_close(oAudioFmtCxt->pb);

    avError = 0;

end:    // Jump flag for errors

    // Free memory
    avformat_free_context(iVideoFmtCxt);

    avcodec_free_context(&iVideoDecoderCxt);
    avcodec_free_context(&iAudioDecoderCxt);

    avcodec_free_context(&oVideoEncoderCxt);
    avcodec_free_context(&oAudioEncoderCxt);

    avformat_free_context(oVideoFmtCxt);
    avformat_free_context(oAudioFmtCxt);

    av_packet_free(&packet);

    av_frame_free(&vFrameIn);
    av_frame_free(&vFrameFiltered);
    av_frame_free(&vFrameOut);

    avfilter_free(videoFilterSrcCxt);
    avfilter_free(videoFilterSinkCxt);
    avfilter_free(videoFilterPadCxt);
    avfilter_free(videoFilterFpsCxt);

    avfilter_graph_free(&videoFilterGraph);
    avfilter_inout_free(&videoFilterInput);
    avfilter_inout_free(&videoFilterOutput);

    sws_freeContext(scale422Cxt);

    av_frame_free(&aFrameIn);
    av_frame_free(&aFrameFiltered);
    av_frame_free(&aFrameOut);

    avfilter_free(volumeFilterSrcCxt);
    avfilter_free(volumeFilterSinkCxt);
    avfilter_free(volumeFilterCxt);
    avfilter_graph_free(&volumeFilterGraph);

    swr_free(&resamplerCxt);

    emit completed(avError, avErrorMsg);
}

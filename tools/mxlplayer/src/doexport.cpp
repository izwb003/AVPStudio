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
#include "doexport.h"

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavcodec/defs.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

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

TDoExport::TDoExport(QObject *parent, QString mxlPath, QString wavPath, QString videoPath, AVP::AVPSize size)
    : QThread{parent}
{
    this->mxlPath = mxlPath;
    this->wavPath = wavPath;
    this->videoPath = videoPath;
    this->size = size;
}

void TDoExport::run()
{
    // FFmpeg init
    av_log_set_level(AV_LOG_QUIET);

    // Init variables
    static int avError = 0;

    AVFormatContext *iVideoFmtCxt = NULL;
    AVFormatContext *iAudioFmtCxt = NULL;

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

    AVStream *oVideoStream = NULL;
    AVStream *oAudioStream = NULL;

    AVPacket *packet = NULL;

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

    SwsContext *scale420Cxt = NULL;

    int videoPTSCounter = 0;

    AVFrame *aFrameIn = NULL;
    AVFrame *aFrameOut = NULL;

    SwrContext *resamplerCxt = NULL;

    AVAudioFifo *aFifo = NULL;
    uint8_t **aSamples = NULL;
    int aSamplesLineSize;
    uint64_t audioPTSCounter = 0;

    // Open input file and find stream info
    iVideoFmtCxt = avformat_alloc_context();
    avError = avformat_open_input(&iVideoFmtCxt, mxlPath.toUtf8(), 0, 0);
    if(avError < 0)
    {
        emit showError(tr("打开MXL出错"), tr("不能打开输入文件。"));
        goto end;
    }
    avError = avformat_find_stream_info(iVideoFmtCxt, 0);
    if(avError < 0)
    {
        emit showError(tr("打开MXL出错"), tr("不能找到视频流。"));
        goto end;
    }
    iAudioFmtCxt = avformat_alloc_context();
    if(!wavPath.isEmpty())
    {
        avError = avformat_open_input(&iAudioFmtCxt, wavPath.toUtf8(), 0, 0);
        if(avError < 0)
        {
            emit showError(tr("打开WAV出错"), tr("不能打开输入文件。"));
            goto end;
        }
        avError = avformat_find_stream_info(iAudioFmtCxt, 0);
        if(avError < 0)
        {
            emit showError(tr("打开WAV出错"), tr("不能找到音频流。"));
            goto end;
        }
    }

    // Get input video and audio stream
    iVideoStreamID = av_find_best_stream(iVideoFmtCxt, AVMEDIA_TYPE_VIDEO, -1, -1, &iVideoDecoder, 0);
    if(iVideoStreamID == AVERROR_STREAM_NOT_FOUND)
    {
        emit showError(tr("查找流信息失败"), tr("找不到视频流。"));
        goto end;
    }
    if(!wavPath.isEmpty())
    {
        iAudioStreamID = av_find_best_stream(iAudioFmtCxt, AVMEDIA_TYPE_AUDIO, -1, -1, &iAudioDecoder, 0);
        if(iAudioStreamID == AVERROR_STREAM_NOT_FOUND)
        {
            emit showError(tr("查找流信息失败"), tr("找不到音频流。"));
            goto end;
        }
    }

    // Open decoder
    iVideoDecoderCxt = avcodec_alloc_context3(iVideoDecoder);
    avError = avcodec_parameters_to_context(iVideoDecoderCxt, iVideoFmtCxt->streams[iVideoStreamID]->codecpar);
    if(avError < 0)
    {
        emit showError(tr("打开MXL出错"), tr("无法加载解码器。"));
        goto end;
    }
    avError = avcodec_open2(iVideoDecoderCxt, iVideoDecoder, 0);
    if(avError < 0)
    {
        emit showError(tr("打开MXL出错"), tr("无法打开解码器。"));
        goto end;
    }

    if(!wavPath.isEmpty())
    {
        iAudioDecoderCxt = avcodec_alloc_context3(iAudioDecoder);
        avError = avcodec_parameters_to_context(iAudioDecoderCxt, iAudioFmtCxt->streams[iAudioStreamID]->codecpar);
        if(avError < 0)
        {
            emit showError(tr("打开WAV出错"), tr("无法加载解码器。"));
            goto end;
        }
        avError = avcodec_open2(iAudioDecoderCxt, iAudioDecoder, 0);
        if(avError < 0)
        {
            emit showError(tr("打开WAV出错"), tr("无法打开解码器。"));
            goto end;
        }
    }

    // Check for video info
    if(iVideoDecoderCxt->width != 3840 || iVideoDecoderCxt->height != 2160)
    {
        emit showError(tr("打开MXL出错"), tr("不支持的视频尺寸。"));
        goto end;
    }

    // Init encoder
    oVideoEncoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    oVideoEncoderCxt = avcodec_alloc_context3(oVideoEncoder);
    av_opt_set(oVideoEncoderCxt->priv_data, "preset", "slow", 0);
    oVideoEncoderCxt -> time_base = av_inv_q(iVideoDecoderCxt->framerate);
    switch(size)
    {
    case AVP::kAVPSmallSize:
        oVideoEncoderCxt -> width = 2830;
        break;
    case AVP::kAVPMediumSize:
        oVideoEncoderCxt -> width = 4632;
        break;
    case AVP::kAVPLargeSize:
        oVideoEncoderCxt -> width = 6166;
        break;
    }
    oVideoEncoderCxt -> height = 1080;
    oVideoEncoderCxt -> pix_fmt = AV_PIX_FMT_YUV420P;
    oVideoEncoderCxt -> gop_size = 10;
    oVideoEncoderCxt -> max_b_frames = 4;
    oVideoEncoderCxt -> color_primaries = iVideoDecoderCxt->color_primaries;
    oVideoEncoderCxt -> color_range = iVideoDecoderCxt->color_range;
    oVideoEncoderCxt -> color_trc = iVideoDecoderCxt->color_trc;
    oVideoEncoderCxt -> profile = AV_PROFILE_H264_MAIN;

    if(!wavPath.isEmpty())
    {
        oAudioEncoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
        oAudioEncoderCxt = avcodec_alloc_context3(oAudioEncoder);
        oAudioEncoderCxt -> time_base = iAudioFmtCxt->streams[iAudioStreamID]->time_base;
        oAudioEncoderCxt -> ch_layout = iAudioDecoderCxt->ch_layout;
        oAudioEncoderCxt -> sample_fmt = AV_SAMPLE_FMT_FLTP;
        oAudioEncoderCxt -> sample_rate = iAudioDecoderCxt -> sample_rate;
        oAudioEncoderCxt -> profile = FF_PROFILE_AAC_LOW;
        oAudioEncoderCxt -> flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // Create output format and stream
    avError = avformat_alloc_output_context2(&oVideoFmtCxt, av_guess_format("mp4", 0, 0), 0, videoPath.toUtf8());
    if(avError < 0)
    {
        emit showError(tr("写入输出视频失败"), tr("无法创建输出上下文。"));
        goto end;
    }
    oVideoStream = avformat_new_stream(oVideoFmtCxt, 0);
    avError = avcodec_parameters_from_context(oVideoStream->codecpar, oVideoEncoderCxt);
    if(avError < 0)
    {
        emit showError(tr("写入输出视频失败"), tr("无法解析输出上下文。"));
        goto end;
    }
    oVideoStream -> time_base = oVideoEncoderCxt->time_base;
    oVideoStream -> r_frame_rate = iVideoDecoderCxt->framerate;

    if(!wavPath.isEmpty())
    {
        oAudioStream = avformat_new_stream(oVideoFmtCxt, 0);
        avError = avcodec_parameters_from_context(oAudioStream->codecpar, oAudioEncoderCxt);
        if(avError < 0)
        {
            emit showError(tr("写入输出视频失败"), tr("无法解析输出上下文。"));
            goto end;
        }
        oAudioStream -> time_base = oAudioEncoderCxt->time_base;
    }

    // Open encoder/file and write file headers
    avError = avcodec_open2(oVideoEncoderCxt, oVideoEncoder, 0);
    if(avError < 0)
    {
        emit showError(tr("写入输出视频失败"), tr("无法打开视频编码器。"));
        goto end;
    }
    if(!wavPath.isEmpty())
    {
        avError = avcodec_open2(oAudioEncoderCxt, oAudioEncoder, 0);
        if(avError < 0)
        {
            emit showError(tr("写入输出视频失败"), tr("无法打开音频编码器。"));
            goto end;
        }
        /*
         * Special note to this fix:
         * In newest ffmpeg API, after opening the encoder, we have to copy the parameters again.
         * Otherwise, decoders will not understand this stream is AAC LC, but a "-1" profile instead.
         * That's weird and certainly not as we expected, so copy the parameters again to fix.
         */
        avError = avcodec_parameters_from_context(oAudioStream->codecpar, oAudioEncoderCxt);
    }

    avError = avio_open(&oVideoFmtCxt->pb, videoPath.toUtf8(), AVIO_FLAG_WRITE);
    if(avError < 0)
    {
        emit showError(tr("写入输出视频失败"), tr("无法打开视频输出I/O。"));
        goto end;
    }
    avError = avformat_write_header(oVideoFmtCxt, 0);
    if(avError < 0)
    {
        emit showError(tr("写入输出视频失败"), tr("无法写入视频文件头。"));
        goto end;
    }

    // Begin conversion
    packet = av_packet_alloc();

    // Convert video
    vFrameIn = av_frame_alloc();
    vFrameFiltered = av_frame_alloc();
    vFrameOut = av_frame_alloc();

    emit setProgressText(tr("转换视频中..."));
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
    if(avError < 0)
    {
        emit showError(tr("转换出错"), tr("不能创建滤镜链。"));
        goto end;
    }

    // Set YUV420 rescaler
    scale420Cxt = sws_getContext(oVideoEncoderCxt -> width, 1080, iVideoDecoderCxt->pix_fmt, oVideoEncoderCxt -> width, 1080, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, 0, 0, 0);

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

                // Rescale to YUV420
                avError = sws_scale_frame(scale420Cxt, vFrameOut, vFrameFiltered);

                // Encode
                vFrameOut -> pts = videoPTSCounter ++;
                avError = avcodec_send_frame(oVideoEncoderCxt, vFrameOut);
                while(true)
                {
                    avError = avcodec_receive_packet(oVideoEncoderCxt, packet);
                    if(avError)
                    {
                        av_packet_unref(packet);
                        break;
                    }
                    av_packet_rescale_ts(packet, oVideoEncoderCxt->time_base, oVideoStream->time_base);
                    packet -> stream_index = 0;
                    avError = av_interleaved_write_frame(oVideoFmtCxt, packet);
                }
            }
        }
    }

    // Flush buffer
    avcodec_send_frame(oVideoEncoderCxt, NULL);
    while(true)
    {
        avError = avcodec_receive_packet(oVideoEncoderCxt, packet);
        if(avError)
        {
            av_packet_unref(packet);
            break;
        }
        av_packet_rescale_ts(packet, oVideoEncoderCxt->time_base, oVideoStream->time_base);
        packet -> stream_index = 0;
        avError = av_interleaved_write_frame(oVideoFmtCxt, packet);
    }

    // Convert audio
    if(!wavPath.isEmpty())
    {
        aFrameIn = av_frame_alloc();
        aFrameOut = av_frame_alloc();

        emit setProgressText(tr("转换音频中..."));
        emit setProgressMax(iAudioFmtCxt->streams[iAudioStreamID]->duration * av_q2d(iAudioFmtCxt->streams[iAudioStreamID]->time_base));
        emit setProgress(0);

        // Set resampler
        avError = swr_alloc_set_opts2(&resamplerCxt, &iAudioDecoderCxt->ch_layout, AV_SAMPLE_FMT_FLTP, iAudioDecoderCxt->sample_rate, &iAudioDecoderCxt->ch_layout, iAudioDecoderCxt->sample_fmt, iAudioDecoderCxt->sample_rate, 0, 0);
        avError = swr_init(resamplerCxt);

        // Set FIFO
        /* Special note to this fix:
         * AAC LC requires 1024 samples to be fed into the encoder at a time.
         * But the number of samples in a frame after the sample often does not reach this number.
         * Therefore, it is necessary to maintain a FIFO queue to ensure that the number of samples sent to the encoder each time is 1024.
         */
        aFifo = av_audio_fifo_alloc(AV_SAMPLE_FMT_FLTP, iAudioDecoderCxt->ch_layout.nb_channels, 1);

        while(av_read_frame(iAudioFmtCxt, packet) == 0)
        {
            if(packet->stream_index == iAudioStreamID)
            {
                avError = avcodec_send_packet(iAudioDecoderCxt, packet);
                while(true)
                {
                    avError = avcodec_receive_frame(iAudioDecoderCxt, aFrameIn);
                    if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                        break;

                    emit setProgress(aFrameIn->pkt_dts * av_q2d(iAudioFmtCxt->streams[iAudioStreamID]->time_base));

                    // Resample
                    avError = av_samples_alloc_array_and_samples(&aSamples, &aSamplesLineSize, iAudioDecoderCxt->ch_layout.nb_channels, aFrameIn->nb_samples, AV_SAMPLE_FMT_FLTP, 0);
                    avError = swr_convert(resamplerCxt, aSamples, aFrameIn->nb_samples, (const uint8_t**)aFrameIn->extended_data, aFrameIn->nb_samples);

                    // Organize FIFO
                    avError = av_audio_fifo_write(aFifo, (void **)aSamples, aFrameIn->nb_samples);

                    // Encode
                    while(av_audio_fifo_size(aFifo) >= oAudioEncoderCxt->frame_size)
                    {
                        // Copy frame settings
                        av_frame_unref(aFrameOut);
                        aFrameOut -> ch_layout = aFrameIn -> ch_layout;
                        aFrameOut -> sample_rate = aFrameIn -> sample_rate;
                        aFrameOut -> format = AV_SAMPLE_FMT_FLTP;
                        aFrameOut -> nb_samples = oAudioEncoderCxt->frame_size;
                        aFrameOut -> pts = audioPTSCounter;
                        audioPTSCounter += aFrameOut->nb_samples;
                        avError = av_frame_get_buffer(aFrameOut, 0);

                        // Encoding
                        avError = av_audio_fifo_read(aFifo, (void **)aFrameOut->data, oAudioEncoderCxt->frame_size);
                        avError = avcodec_send_frame(oAudioEncoderCxt, aFrameOut);
                        while(true)
                        {
                            avError = avcodec_receive_packet(oAudioEncoderCxt, packet);
                            if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                                break;
                            packet -> stream_index = 1;
                            avError = av_write_frame(oVideoFmtCxt, packet);
                        }
                    }
                }
            }
        }

        // Flush buffer

        // Copy frame settings
        av_frame_unref(aFrameOut);
        aFrameOut -> ch_layout = aFrameIn -> ch_layout;
        aFrameOut -> sample_rate = aFrameIn -> sample_rate;
        aFrameOut -> format = AV_SAMPLE_FMT_FLTP;
        aFrameOut -> nb_samples = av_audio_fifo_size(aFifo);
        aFrameOut -> pts = audioPTSCounter;
        avError = av_frame_get_buffer(aFrameOut, 0);

        // Encoding
        avError = av_audio_fifo_read(aFifo, (void **)aFrameOut->data, oAudioEncoderCxt->frame_size);
        avError = avcodec_send_frame(oAudioEncoderCxt, aFrameOut);
        while(true)
        {
            avError = avcodec_receive_packet(oAudioEncoderCxt, packet);
            if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                break;
            packet -> stream_index = 1;
            avError = av_write_frame(oVideoFmtCxt, packet);
        }
    }

    // Write file tail
    avError = av_write_trailer(oVideoFmtCxt);
    if(avError < 0)
    {
        emit showError(tr("写入输出视频失败"), tr("无法写入视频文件尾。"));
        goto end;
    }

    // Close files
    avformat_close_input(&iVideoFmtCxt);
    if(!wavPath.isEmpty())
        avformat_close_input(&iAudioFmtCxt);
    avio_close(oVideoFmtCxt->pb);

    avError = 0;

end:    // Jump flag for errors
    avformat_free_context(iVideoFmtCxt);
    avformat_free_context(iAudioFmtCxt);

    avcodec_free_context(&iVideoDecoderCxt);
    if(!wavPath.isEmpty())
        avcodec_free_context(&iAudioDecoderCxt);

    avcodec_free_context(&oVideoEncoderCxt);
    if(!wavPath.isEmpty())
        avcodec_free_context(&oAudioEncoderCxt);

    avformat_free_context(oVideoFmtCxt);

    av_packet_free(&packet);

    av_frame_free(&vFrameIn);
    av_frame_free(&vFrameFiltered);
    av_frame_free(&vFrameOut);

    avfilter_free(videoFilterSrcCxt);
    avfilter_free(videoFilterSinkCxt);

    avfilter_graph_free(&videoFilterGraph);
    avfilter_inout_free(&videoFilterInput);
    avfilter_inout_free(&videoFilterOutput);

    sws_freeContext(scale420Cxt);

    if(!wavPath.isEmpty())
    {
        av_frame_free(&aFrameIn);
        av_frame_free(&aFrameOut);

        swr_free(&resamplerCxt);

        av_audio_fifo_free(aFifo);
        av_freep(aSamples);
    }

    if(avError == 0)
        emit completed();
}

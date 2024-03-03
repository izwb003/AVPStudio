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
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libswresample/swresample.h>
}

#include <QMessageBox>

// FFmpeg error parser
static int avError;
static char avErrorStr[AV_ERROR_MAX_STRING_SIZE];

// FFmpeg filter graph description
static const char *filterGraphLarge =
    "[in]scale=6166:1080[scaled];"
    "[scaled]split[scaled1][scaled2];"
    "[scaled1]crop=3840:1080:0:0[left];"
    "[scaled2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";
static const char *filterGraphMedium =
    "[in]scale=4632:1080[scaled];"
    "[scaled]pad=6166:1080:767:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";
static const char *filterGraphSmall =
    "[in]scale=2830:1080[scaled];"
    "[scaled]pad=6166:1080:1668:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";

TDoProcess::TDoProcess(QObject *parent) {}

void TDoProcess::run()
{
    // FFmpeg init
    //av_log_set_level(AV_LOG_ERROR);
    //av_log_set_callback(custom_output);

    // Init variables
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
    AVFrame *frame = NULL;

    SwrContext *resamplerCxt = NULL;
    uint8_t **resampleDat = NULL;

    AVFrame *iFrame = NULL;
    AVFrame *oFrame = NULL;

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
    oVideoEncoderCxt -> width = 852;
    oVideoEncoderCxt -> height = 480;
    oVideoEncoderCxt -> bit_rate = settings.outputVideoBitRate * 1000000;
    oVideoEncoderCxt -> rc_max_rate = oVideoEncoderCxt->bit_rate;
    oVideoEncoderCxt -> rc_min_rate = oVideoEncoderCxt->bit_rate;
    oVideoEncoderCxt -> rc_buffer_size = oVideoEncoderCxt->bit_rate / 2;
    oVideoEncoderCxt -> bit_rate_tolerance = 0;
    oVideoEncoderCxt -> pix_fmt = AV_PIX_FMT_YUV420P;
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
    frame = av_frame_alloc();

    // Convert video
    emit setLabel(tr("转换视频中...") + settings.getOutputVideoFinalName());
    emit setProgressMax(iVideoFmtCxt->streams[iVideoStreamID]->duration * av_q2d(iVideoFmtCxt->streams[iVideoStreamID]->time_base));

    while(av_read_frame(iVideoFmtCxt, packet) == 0)
    {
        if(packet->stream_index == iVideoStreamID)
        {
            avError = avcodec_send_packet(iVideoDecoderCxt, packet);
            while(true)
            {
                avError = avcodec_receive_frame(iVideoDecoderCxt, frame);
                if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                    break;

                // Encode
                avError = avcodec_send_frame(oVideoEncoderCxt, frame);
                avError = avcodec_receive_packet(oVideoEncoderCxt, packet);
                av_packet_rescale_ts(packet, oVideoEncoderCxt->time_base, oVideoFmtCxt->streams[0]->time_base);
                avError = av_write_frame(oVideoFmtCxt, packet);

                emit setProgress(frame->pkt_dts * av_q2d(iVideoFmtCxt->streams[iVideoStreamID]->time_base));
            }
        }
    }

    avformat_flush(iVideoFmtCxt);
    av_seek_frame(iVideoFmtCxt, iVideoStreamID, 0, AVSEEK_FLAG_BACKWARD);

    // Convert audio
    emit setLabel(tr("转换音频中...") + settings.getOutputAudioFinalName());
    emit setProgressMax(iVideoFmtCxt->streams[iAudioStreamID]->duration * av_q2d(iVideoFmtCxt->streams[iAudioStreamID]->time_base));

    iFrame = av_frame_alloc();
    oFrame = av_frame_alloc();

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
                avError = avcodec_receive_frame(iAudioDecoderCxt, iFrame);
                if(avError == AVERROR(EAGAIN) || avError == AVERROR_EOF)
                    break;

                oFrame -> ch_layout = iFrame -> ch_layout;
                oFrame -> sample_rate = iFrame -> sample_rate;
                oFrame -> format = AV_SAMPLE_FMT_S32;
                oFrame -> nb_samples = av_rescale_rnd(swr_get_delay(resamplerCxt, 48000) + iFrame->nb_samples, 48000, iAudioDecoderCxt->sample_rate, AV_ROUND_UP);

                // Resample
                avError = swr_convert_frame(resamplerCxt, oFrame, iFrame);

                // Encode
                avError = avcodec_send_frame(oAudioEncoderCxt, oFrame);
                avError = avcodec_receive_packet(oAudioEncoderCxt, packet);
                avError = av_write_frame(oAudioFmtCxt, packet);

                emit setProgress(iFrame->pkt_dts * av_q2d(iVideoFmtCxt->streams[iAudioStreamID]->time_base));
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
}
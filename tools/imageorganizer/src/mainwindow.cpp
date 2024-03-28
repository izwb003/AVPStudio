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
#include "mainwindow.h"
#include "./ui_mainwindow.h"

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
}

#include <QFileDialog>
#include <QImage>
#include <QMessageBox>
#include <QPixmap>

static const char *filterGraphLarge =
    "[in]pad=iw:ih:0:0:black[expanded];"
    "[expanded]scale=6166:1080[scaled];"
    "[scaled]split[scaled1][scaled2];"
    "[scaled1]crop=3840:1080:0:0[left];"
    "[scaled2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";
static const char *filterGraphMedium =
    "[in]pad=iw:ih:0:0:black[expanded];"
    "[expanded]scale=4632:1080[scaled];"
    "[scaled]pad=6166:1080:767:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";
static const char *filterGraphSmall =
    "[in]pad=iw:ih:0:0:black[expanded];"
    "[expanded]scale=2830:1080[scaled];"
    "[scaled]pad=6166:1080:1668:0:black[padded];"
    "[padded]split[padded1][padded2];"
    "[padded1]crop=3840:1080:0:0[left];"
    "[padded2]crop=3840:1080:2327:0[right];"
    "[left][right]vstack=2[out]";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlags(windowFlags()& ~Qt::WindowMaximizeButtonHint);
    this->setFixedSize(this->width(), this->height());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_radioButtonSmallSize_clicked(bool checked)
{
    ui->labelSize->setText(tr("尺寸信息：5.5m x 2.1m / 分辨率：2830 x 1080"));
    ui->labelImagePreview->setMinimumSize(700, 266);
    ui->labelImagePreview->setMaximumSize(700, 266);
    on_lineEditInPath_editingFinished();
    settings.size = AVP::kAVPSmallSize;
    settings.width = 2830;
}

void MainWindow::on_radioButtonMediumSize_clicked(bool checked)
{
    ui->labelSize->setText(tr("尺寸信息：9m x 2.1m / 分辨率：4633 x 1080"));
    ui->labelImagePreview->setMinimumSize(700, 161);
    ui->labelImagePreview->setMaximumSize(700, 161);
    on_lineEditInPath_editingFinished();
    settings.size = AVP::kAVPMediumSize;
    settings.width = 4633;
}


void MainWindow::on_radioButtonLargeSize_clicked(bool checked)
{
    ui->labelSize->setText(tr("尺寸信息：12m x 2.1m / 分辨率：6167 x 1080"));
    ui->labelImagePreview->setMinimumSize(700, 122);
    ui->labelImagePreview->setMaximumSize(700, 122);
    on_lineEditInPath_editingFinished();
    settings.size = AVP::kAVPLargeSize;
    settings.width = 6167;
}


void MainWindow::on_pushButtonQuit_clicked()
{
    qApp->quit();
}


void MainWindow::on_checkBoxExtend_stateChanged(int arg1)
{
    if(arg1)
    {
        settings.isExtended = true;
        ui->labelImagePreview->setScaledContents(true);
        on_lineEditInPath_editingFinished();
    }
    else
    {
        settings.isExtended = false;
        ui->labelImagePreview->setScaledContents(false);
        ui->labelImagePreview->setPixmap(ui->labelImagePreview->pixmap().scaledToHeight(ui->labelImagePreview->height()));
    }
}

void MainWindow::on_checkBoxDolbyNaming_stateChanged(int arg1)
{
    if(arg1)
    {
        settings.isDolbyNaming = true;
    }
    else
    {
        settings.isDolbyNaming = false;
    }
}


void MainWindow::on_pushButtonInBrowse_clicked()
{
    settings.fileInputPath = QFileDialog::getOpenFileName(this, tr("保存输出图片"), QDir::homePath(), tr("所有图片 (*.jpg *.jpeg *.png *.bmp *.tiff);;JPEG图片 (*.jpg *.jpeg);;PNG图片 (*.png);;BMP图片 (*.bmp);;TIFF图片 (*.tiff)"));
    ui->lineEditInPath->setText(settings.fileInputPath);
    on_lineEditInPath_editingFinished();
}


void MainWindow::on_lineEditInPath_editingFinished()
{
    settings.fileInputPath = ui->lineEditInPath->text();

    if(settings.fileInputPath.isEmpty())
        return;

    QImage previewImg;
    if(!(previewImg.load(settings.fileInputPath)))
    {
        QMessageBox::critical(this, tr("读取图片错误"), tr("不能解析图片。"));
        return;
    }
    if(ui->labelImagePreview->hasScaledContents())
        ui->labelImagePreview->setPixmap(QPixmap::fromImage(previewImg));
    else
        ui->labelImagePreview->setPixmap(QPixmap::fromImage(previewImg).scaledToHeight(ui->labelImagePreview->height()));
}

void MainWindow::on_pushButtonConvert_clicked()
{
    settings.fileOutputPath = QFileDialog::getSaveFileName(this, tr("选择保存输出图片位置..."), QDir::homePath(), tr("JPEG图片 (*.jpg);;PNG图片 (*.png)"));
    if(settings.isDolbyNaming)
    {
        QFileInfo outputFileInfo(settings.fileOutputPath);
        switch(settings.size)
        {
        case AVP::kAVPSmallSize:
            settings.fileOutputPath = outputFileInfo.absolutePath() + "/V2_" + outputFileInfo.baseName() + "_video_5m." + outputFileInfo.suffix();
            break;
        case AVP::kAVPMediumSize:
            settings.fileOutputPath = outputFileInfo.absolutePath() + "/V2_" + outputFileInfo.baseName() + "_video_9m." + outputFileInfo.suffix();
            break;
        case AVP::kAVPLargeSize:
            settings.fileOutputPath = outputFileInfo.absolutePath() + "/V2_" + outputFileInfo.baseName() + "_video_12m." + outputFileInfo.suffix();
            break;
        }
    }
    doConversion();
}

template<typename T> int toUpperInt(T val)
{
    if((int)val % 2 == 1)
        return (int)val + 1;
    else
        return (int)val;
}

void MainWindow::doConversion()
{
    // FFmpeg init
    av_log_set_level(AV_LOG_QUIET);

    // Init variables
    static int avError = 0;

    AVFormatContext *iImageFmtCxt = NULL;
    AVFormatContext *oImageFmtCxt = NULL;

    int iImageStreamID = -1;

    QFileInfo oImageFileInfo(settings.fileOutputPath);

    const AVCodec *iImageDecoder = NULL;
    const AVCodec *oImageEncoder = NULL;
    AVCodecContext *iImageDecoderCxt = NULL;
    AVCodecContext *oImageEncoderCxt = NULL;

    AVStream *oImageStream = NULL;

    AVPacket *packet = NULL;
    AVFrame *imageFrameIn = NULL;
    AVFrame *imageFrameFiltered = NULL;
    AVFrame *imageFrameOut = NULL;

    AVFilterGraph *imageFilterGraph = NULL;
    AVFilterInOut *imageFilterInput = NULL;
    AVFilterInOut *imageFilterOutput = NULL;

    AVFilterContext *imageFilterPadCxt = NULL;

    const AVFilter *imageFilterSrc = NULL;
    AVFilterContext *imageFilterSrcCxt = NULL;
    const AVFilter *imageFilterSink = NULL;
    AVFilterContext *imageFilterSinkCxt = NULL;

    SwsContext *scaleCxt = NULL;

    // Open input file and find stream info
    iImageFmtCxt = avformat_alloc_context();
    avError = avformat_open_input(&iImageFmtCxt, settings.fileInputPath.toUtf8(), 0, 0);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("加载文件失败"), tr("打开文件出错。"));
        goto end;
    }
    avError = avformat_find_stream_info(iImageFmtCxt, 0);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("加载文件失败"), tr("不能找到流信息。"));
        goto end;
    }

    // Get input image stream
    iImageStreamID = av_find_best_stream(iImageFmtCxt, AVMEDIA_TYPE_VIDEO, -1, -1, &iImageDecoder, 0);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("加载文件失败"), tr("不能找到图片流。"));
        goto end;
    }

    // Open decoder
    iImageDecoderCxt = avcodec_alloc_context3(iImageDecoder);
    avError = avcodec_parameters_to_context(iImageDecoderCxt, iImageFmtCxt->streams[iImageStreamID]->codecpar);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("加载文件失败"), tr("没有对应的图片解码器。"));
        goto end;
    }
    avError = avcodec_open2(iImageDecoderCxt, iImageDecoder, 0);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("加载文件失败"), tr("无法打开图片解码器。"));
        goto end;
    }

    // Init encoder
    if(oImageFileInfo.suffix() == "jpg")
        oImageEncoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    else if(oImageFileInfo.suffix() == "png")
        oImageEncoder = avcodec_find_encoder(AV_CODEC_ID_PNG);
    else
    {
        QMessageBox::critical(this, tr("写入输出失败"), tr("无法取得输出格式。"));
        goto end;
    }
    oImageEncoderCxt = avcodec_alloc_context3(oImageEncoder);
    oImageEncoderCxt -> width = 3840;
    oImageEncoderCxt -> height = 2160;
    if(oImageFileInfo.suffix() == "jpg")
        oImageEncoderCxt -> pix_fmt = AV_PIX_FMT_YUVJ420P;
    else if(oImageFileInfo.suffix() == "png")
        oImageEncoderCxt -> pix_fmt = AV_PIX_FMT_RGBA;
    oImageEncoderCxt -> time_base = (AVRational){1, 25};

    // Create output format and stream
    avError = avformat_alloc_output_context2(&oImageFmtCxt, 0, 0, settings.fileOutputPath.toUtf8());
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("写入输出失败"), tr("无法分配输出上下文。"));
        goto end;
    }
    oImageStream = avformat_new_stream(oImageFmtCxt, 0);
    avError = avcodec_parameters_from_context(oImageStream->codecpar, oImageEncoderCxt);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("写入输出失败"), tr("无法解析输出上下文。"));
        goto end;
    }

    // Open encoder/file and write file headers
    avError = avcodec_open2(oImageEncoderCxt, oImageEncoder, 0);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("写入输出失败"), tr("无法打开输出编码器。"));
        goto end;
    }
    avError = avio_open(&oImageFmtCxt->pb, settings.fileOutputPath.toUtf8(), AVIO_FLAG_WRITE);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("写入输出失败"), tr("无法打开输出I/O。"));
        goto end;
    }
    avError = avformat_write_header(oImageFmtCxt, 0);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("写入输出失败"), tr("无法写入图片文件头。"));
        goto end;
    }

    // Begin conversion
    packet = av_packet_alloc();

    imageFrameIn = av_frame_alloc();
    imageFrameFiltered = av_frame_alloc();
    imageFrameOut = av_frame_alloc();

    // Set image filter
    imageFilterGraph = avfilter_graph_alloc();

    imageFilterSrc = avfilter_get_by_name("buffer");
    char imageFilterSrcArgs[512];
    snprintf(imageFilterSrcArgs, sizeof(imageFilterSrcArgs), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", iImageDecoderCxt->width, iImageDecoderCxt->height, iImageDecoderCxt->pix_fmt, iImageFmtCxt->streams[iImageStreamID]->time_base.num, iImageFmtCxt->streams[iImageStreamID]->time_base.den, iImageDecoderCxt->sample_aspect_ratio.num, iImageDecoderCxt->sample_aspect_ratio.den);
    avError = avfilter_graph_create_filter(&imageFilterSrcCxt, imageFilterSrc, "in", imageFilterSrcArgs, 0, imageFilterGraph);

    imageFilterSink = avfilter_get_by_name("buffersink");
    avError = avfilter_graph_create_filter(&imageFilterSinkCxt, imageFilterSink, "out", 0, 0, imageFilterGraph);

    imageFilterInput = avfilter_inout_alloc();
    imageFilterInput -> name = av_strdup("in");
    imageFilterInput -> filter_ctx = imageFilterSrcCxt;
    imageFilterInput -> pad_idx = 0;
    imageFilterInput -> next = NULL;

    imageFilterOutput = avfilter_inout_alloc();
    imageFilterOutput -> name = av_strdup("out");
    imageFilterOutput -> filter_ctx = imageFilterSinkCxt;
    imageFilterOutput -> pad_idx = 0;
    imageFilterOutput -> next = NULL;

    switch(settings.size)
    {
    case AVP::kAVPLargeSize:
        avError = avfilter_graph_parse_ptr(imageFilterGraph, filterGraphLarge, &imageFilterOutput, &imageFilterInput, 0);
        break;
    case AVP::kAVPMediumSize:
        avError = avfilter_graph_parse_ptr(imageFilterGraph, filterGraphMedium, &imageFilterOutput, &imageFilterInput, 0);
        break;
    case AVP::kAVPSmallSize:
        avError = avfilter_graph_parse_ptr(imageFilterGraph, filterGraphSmall, &imageFilterOutput, &imageFilterInput, 0);
        break;
    }

    imageFilterPadCxt = avfilter_graph_get_filter(imageFilterGraph, "Parsed_pad_0");

    /*
     * Special note to this fix:
     * Unlike videos, images may have an odd width/height.
     * The "pad" filter receives an odd size and automatically rounds down to an even number. If the rounded size is smaller than the size of the input image, the filter system will throw an exception.
     * So it is necessary to manually adjust the parameters of the incoming "pad" filter to accept even data with a larger size than the input content.
     */
    avError = av_opt_set(imageFilterPadCxt, "width", QString::number(toUpperInt(iImageDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
    avError = av_opt_set(imageFilterPadCxt, "height", QString::number(toUpperInt(iImageDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
    if(iImageDecoderCxt->width == settings.width && iImageDecoderCxt->height == 1080)
        settings.isExtended = false;


    if(!settings.isExtended)
    {
        if((iImageDecoderCxt->width / iImageDecoderCxt->height) < (settings.width / 1080))
        {
            switch(settings.size)
            {
            case AVP::kAVPLargeSize:
                avError = av_opt_set(imageFilterPadCxt, "width", QString::number(toUpperInt(iImageDecoderCxt->height * 5.71)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "height", QString::number(toUpperInt(iImageDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "x", QString::number(toUpperInt(((iImageDecoderCxt->height * 5.71) / 2) - (iImageDecoderCxt->width / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPMediumSize:
                avError = av_opt_set(imageFilterPadCxt, "width", QString::number(toUpperInt(iImageDecoderCxt->height * 4.29)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "height", QString::number(toUpperInt(iImageDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "x", QString::number(toUpperInt(((iImageDecoderCxt->height * 4.29) / 2) - (iImageDecoderCxt->width / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPSmallSize:
                avError = av_opt_set(imageFilterPadCxt, "width", QString::number(toUpperInt(iImageDecoderCxt->height * 2.62)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "height", QString::number(toUpperInt(iImageDecoderCxt->height)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "x", QString::number(toUpperInt(((iImageDecoderCxt->height * 2.62) / 2) - (iImageDecoderCxt->width / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            }
        }
        else if((iImageDecoderCxt->width / iImageDecoderCxt->height) > (settings.width / 1080))
        {
            switch(settings.size)
            {
            case AVP::kAVPLargeSize:
                avError = av_opt_set(imageFilterPadCxt, "width", QString::number(toUpperInt(iImageDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "height", QString::number(toUpperInt(iImageDecoderCxt->width * 0.175)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "y", QString::number(toUpperInt(((iImageDecoderCxt->width * 0.175) / 2) - (iImageDecoderCxt->height / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPMediumSize:
                avError = av_opt_set(imageFilterPadCxt, "width", QString::number(toUpperInt(iImageDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "height", QString::number(toUpperInt(iImageDecoderCxt->width * 0.233)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "y", QString::number(toUpperInt(((iImageDecoderCxt->width * 0.233) / 2) - (iImageDecoderCxt->height / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            case AVP::kAVPSmallSize:
                avError = av_opt_set(imageFilterPadCxt, "width", QString::number(toUpperInt(iImageDecoderCxt->width)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "height", QString::number(toUpperInt(iImageDecoderCxt->width * 0.382)).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                avError = av_opt_set(imageFilterPadCxt, "y", QString::number(toUpperInt(((iImageDecoderCxt->width * 0.382) / 2) - (iImageDecoderCxt->height / 2))).toUtf8(), AV_OPT_SEARCH_CHILDREN);
                break;
            }
        }
    }

    avError = avfilter_graph_config(imageFilterGraph, 0);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("转换出错"), tr("不能创建滤镜链。\n尝试改变输入文件尺寸（保持宽度、高度均为偶数）\n或关闭“拉伸图片以填充”重试。"));
        goto end;
    }

    // Set YUV420/RGBA rescaler
    if(oImageFileInfo.suffix() == "jpg")
        scaleCxt = sws_getContext(3840, 2160, iImageDecoderCxt->pix_fmt, 3840, 2160, AV_PIX_FMT_YUVJ420P, SWS_FAST_BILINEAR, 0, 0, 0);
    else if(oImageFileInfo.suffix() == "png")
        scaleCxt = sws_getContext(3840, 2160, iImageDecoderCxt->pix_fmt, 3840, 2160, AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR, 0, 0, 0);

    // Decode input
    avError = av_read_frame(iImageFmtCxt, packet);
    avError = avcodec_send_packet(iImageDecoderCxt, packet);
    avError = avcodec_receive_frame(iImageDecoderCxt, imageFrameIn);

    // Apply filter
    avError = av_buffersrc_add_frame(imageFilterSrcCxt, imageFrameIn);
    avError = av_buffersink_get_frame(imageFilterSinkCxt, imageFrameFiltered);

    // Rescale to YUV420/RGBA
    avError = sws_scale_frame(scaleCxt, imageFrameOut, imageFrameFiltered);

    // Encode
    avError = avcodec_send_frame(oImageEncoderCxt, imageFrameOut);
    avError = avcodec_receive_packet(oImageEncoderCxt, packet);
    avError = av_write_frame(oImageFmtCxt, packet);

    // Write file tail
    avError = av_write_trailer(oImageFmtCxt);
    if(avError < 0)
    {
        QMessageBox::critical(this, tr("写入输出失败"), tr("无法写入图片文件尾。"));
        goto end;
    }

    QMessageBox::information(this, tr("写入输出成功"), tr("转换完成。"));

end:    // Jump flag for errors

    avformat_free_context(iImageFmtCxt);
    avformat_free_context(oImageFmtCxt);

    avcodec_free_context(&iImageDecoderCxt);
    avcodec_free_context(&oImageEncoderCxt);

    av_packet_free(&packet);
    av_frame_free(&imageFrameIn);
    av_frame_free(&imageFrameFiltered);
    av_frame_free(&imageFrameOut);

    avfilter_free(imageFilterPadCxt);
    avfilter_free(imageFilterSrcCxt);
    avfilter_free(imageFilterSinkCxt);

    avfilter_graph_free(&imageFilterGraph);
    avfilter_inout_free(&imageFilterInput);
    avfilter_inout_free(&imageFilterOutput);

    sws_freeContext(scaleCxt);
}


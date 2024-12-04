#include "spike2video.h"
#include "QDebug"
#include "QFileInfo"
#include "QDir"

Spike2video::Spike2video(QObject *parent)
        :QThread(parent)
{
    av_log_set_level(AV_LOG_DEBUG);
    this->tfirecon = new TfiRecon;
    this->tfprecon = new TfpRecon;
    recon_img_list.clear();
}

Spike2video::~Spike2video()
{
    if(this->tfirecon != nullptr) delete this->tfirecon;
    if(this->tfprecon != nullptr) delete this->tfprecon;
}

void Spike2video::slot_set_exprot_save_path(QString path)
{
    this->exprot_save_path = path;
    QDir dir(this->exprot_save_path);

    if (!dir.exists()) {
        dir.mkpath(this->exprot_save_path);
    }
}

void Spike2video::slot_set_recon_img_func(int reon_img_func, int tfp_win)
{
    this->recon_img_func = reon_img_func;

    this->tfp_win = tfp_win;
    this->tfprecon->setTfp_win(tfp_win);
}

void Spike2video::slot_set_gamma_value(double new_gamm_value)
{
    this->tfprecon->setGamma_value(new_gamm_value);
    this->tfirecon->setGamma_value(new_gamm_value);
}

void Spike2video::slot_set_img_size_file(QSize new_img_size, QString file_name)
{
    this->img_size = new_img_size;

    this->tfprecon->set_recon_img_size(new_img_size);
    this->tfirecon->set_recon_img_size(new_img_size);

    if(this->m_spike_dat_path != file_name) {
        this->tfprecon->set_spikedat_path(file_name);
        this->tfirecon->set_spikedat_path(file_name);
        this->m_spike_dat_path = file_name;
    }
}

void Spike2video::slot_start_spike2video()
{
    if(this->isRunning()) return;
    this->hanld_cnt = 0;
    this->setIs_runnig(true);
    this->start();

}

void Spike2video::slot_stop_spike2video()
{
    this->setIs_runnig(false);
    this->requestInterruption();
}

void Spike2video::slot_set_img_mirror(int new_mirror_index)
{
    this->mirror_index = new_mirror_index;
}

void Spike2video::run()
{
    recon_img_list.clear();

    int process_cnt = 0, totoal_imgs = 0;
    this->init_encoder();

    this->tfprecon->get_process_progress(process_cnt, totoal_imgs);

    while(!this->isInterruptionRequested() && this->getIs_runnig()
                        &&(this->hanld_cnt != totoal_imgs)) {
        QImage src_img, recon_img;
        if(this->recon_img_func == 0) {
            this->tfirecon->get_one_src_recon_img(src_img, recon_img);
            this->tfirecon->get_process_progress(process_cnt, totoal_imgs);
        } else {
            this->tfprecon->get_one_src_recon_img(src_img, recon_img);
            this->tfprecon->get_process_progress(process_cnt, totoal_imgs);
        }
        this->mirror_img(src_img, recon_img);
        this->recon_img_list.append(recon_img);
        this->convert_spike2video();
        this->hanld_cnt = process_cnt;
        emit this->signal_src2video_process(this->hanld_cnt, totoal_imgs);
    }

    this->convert_imgvideo_back_work();

}

bool Spike2video::init_encoder()
{
    av_log_set_level(AV_LOG_ERROR);
    int ret;
    this->pts_counter = 0;
    // 打开输出文件
    QFileInfo fileInfo(this->m_spike_dat_path);
    QString save_path = this->exprot_save_path + "/" + fileInfo.completeBaseName();
    QDir dir(save_path);

    if(!dir.exists()) {
        dir.mkpath(".");
    }

    // QString avi_out_file_name = save_path + "/"+ fileInfo.completeBaseName()  + ".avi";
    QString avi_out_file_name = save_path + "/"+ fileInfo.completeBaseName()  + ".mp4";
    qDebug() << "avi_out_file_name:" << avi_out_file_name;

    if(avformat_alloc_output_context2(&m_format_ctx, NULL, NULL, avi_out_file_name.toUtf8().constData()) != 0) {
        qDebug() << "init m_format_ctx failed";
        return false;
    }

    // 查找编码器
    // m_codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    m_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!m_codec) {
        avformat_free_context(m_format_ctx);
        qDebug() << "init m_codec failed";
        return false;
    }

    m_codec_ctx = avcodec_alloc_context3(m_codec);
    if (!m_codec_ctx) {
        avformat_free_context(m_format_ctx);
        qDebug() << "init avcodec_alloc_context3 failed";
        return false;
    }

    m_codec_ctx->bit_rate = 10000000;
    m_codec_ctx->codec_id = m_codec->id;
    m_codec_ctx->width = img_size.width();
    m_codec_ctx->height = img_size.height();
    m_codec_ctx->time_base = AVRational{1, 25};
    m_codec_ctx->framerate = {25, 1};
    m_codec_ctx->gop_size = 25;
    m_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

    ret = avcodec_open2(m_codec_ctx, m_codec, NULL);
    if (ret < 0) {
        char errorMsg[1024];
        av_strerror(ret, errorMsg, sizeof(errorMsg));
        avcodec_free_context(&m_codec_ctx);
        avformat_free_context(m_format_ctx);
        qDebug("open2_error:%s", errorMsg);
        return false;
    }

    // 创建 AVStream 对象
    this->m_stream = avformat_new_stream(this->m_format_ctx, nullptr);
    if (!this->m_stream) {
        qDebug() << "无法创建流";
        return false;
    }
    // 设置流的编码器参数
    avcodec_parameters_from_context(this->m_stream->codecpar, this->m_codec_ctx);

    // 打开输出文件
    if (avio_open(&this->m_format_ctx->pb, avi_out_file_name.toUtf8().constData(), AVIO_FLAG_WRITE) < 0) {
        qDebug() << "无法打开输出文件";
        return false;
    }

    // 写入文件头
    if (avformat_write_header(this->m_format_ctx, nullptr) < 0) {
        qDebug() << "无法写入文件头";
        return false;
    }

    // 创建用于图像转换的 SwsContext 对象
    this->m_sws_ctx = sws_getContext(img_size.width(), img_size.height(), AV_PIX_FMT_GRAY8,
                                            img_size.width(), img_size.height(), this->m_codec_ctx->pix_fmt,
                                            SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!this->m_sws_ctx) {
        qDebug() << "无法创建图像转换上下文";
        return false;
    }

    return true;
}

void Spike2video::convert_imgvideo_back_work()
{
    int ret = avcodec_send_frame(this->m_codec_ctx, nullptr);
    if (ret < 0) {
        qDebug() << "Error sending a NULL frame to the encoder:" << ret;
        // 处理错误，例如记录日志、释放资源等
        return;
    }

    // 循环直到没有更多数据包
    while (ret >= 0) {
        ret = avcodec_receive_packet(this->m_codec_ctx, this->packet);
        if (ret == 0) {
            this->packet->stream_index = this->m_stream->index;
            av_packet_rescale_ts(this->packet, this->m_codec_ctx->time_base, this->m_stream->time_base);
            if (av_interleaved_write_frame(this->m_format_ctx, this->packet) < 0) {
                qDebug() << "Error while writing packet to the output file.";
                // 处理错误
            }
            av_packet_unref(this->packet);
        } else {
            qDebug() << "Error receiving packet from the encoder:" << ret;
            // 处理错误
            break;
        }
    }

    // 写入文件尾
    if (av_write_trailer(this->m_format_ctx) < 0) {
        qDebug() << "Error writing trailer to the output file.";
        // 处理错误
    }

    // 关闭文件和释放资源
    avcodec_close(this->m_codec_ctx);
    if (avio_closep(&this->m_format_ctx->pb) < 0) {
        qDebug() << "Error closing the output file.";
        // 处理错误
    }
    avformat_free_context(this->m_format_ctx);

    // 重置编码状态
    m_encoding = false;
}

void Spike2video::convert_spike2video()
{

    m_encoding = true;

    this->packet = av_packet_alloc();

    while (!isInterruptionRequested() && !this->recon_img_list.isEmpty()) {
        QImage image = this->recon_img_list.takeFirst();

        AVFrame *frame = av_frame_alloc();

        frame->format = m_codec_ctx->pix_fmt;
        frame->width = m_codec_ctx->width;
        frame->height = m_codec_ctx->height;

        av_frame_get_buffer(frame, 0);

        int srcLinesize[4] = { image.bytesPerLine(), 0, 0, 0 };
        uint8_t* srcData[4] = { (uint8_t *)image.constBits(), nullptr, nullptr, nullptr };

        sws_scale(this->m_sws_ctx, srcData, srcLinesize, 0, image.height(),
                  frame->data, frame->linesize);

        frame->pts = this->pts_counter++;
        frame->pts = av_rescale_q(frame->pts, {1, 25}, m_codec_ctx->time_base);

    // 编码 AVFrame 得到 AVPacket
        avcodec_send_frame(this->m_codec_ctx, frame);
        while (avcodec_receive_packet(this->m_codec_ctx, this->packet) == 0) {
            // 打印数据包信息
            // qDebug() << "Received packet with size" << this->packet->size
            //          << "and stream index" << this->packet->stream_index
            //          << "with pts" << this->packet->pts
            //          << "and dts" << this->packet->dts;

            this->packet->stream_index = this->m_stream->index;
            av_packet_rescale_ts(this->packet, this->m_codec_ctx->time_base, this->m_stream->time_base);

            // 写入数据包到文件
            if (av_interleaved_write_frame(this->m_format_ctx, this->packet) < 0) {
                qDebug() << "Error while writing packet to the output file.";
            }
            av_packet_unref(this->packet);
            this->packet->pts = av_rescale_q(this->packet->pts, m_codec_ctx->time_base, this->m_stream->time_base);
            this->packet->dts = av_rescale_q(this->packet->dts, m_codec_ctx->time_base, this->m_stream->time_base);
        }

        av_frame_free(&frame);
    }

//    if(this->recon_img_func == 0) {
//        emit this->signal_src2video_process(this->handler_cnt, this->total_img_cnt);
//    } else if(this->recon_img_func == 1) {
//        emit this->signal_src2video_process(this->handler_cnt/this->tfp_win, this->total_img_cnt/this->tfp_win);
//    }
}

bool Spike2video::getIs_runnig() const
{
    return is_runnig;
}

void Spike2video::setIs_runnig(bool value)
{
    is_runnig = value;
}

void Spike2video::mirror_img(QImage &src_img, QImage &recon_img)
{

    //选择不同的旋转方式
    switch (this->mirror_index) {
        case 0:
            break;
        case 1:
            src_img = src_img.mirrored(true, false);
            recon_img = recon_img.mirrored(true, false);
            break;
        case 2:
            src_img = src_img.mirrored(false, true);
            recon_img = recon_img.mirrored(false, true);
            break;
    }

}

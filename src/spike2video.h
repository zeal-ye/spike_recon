#ifndef SPIKE2VIDEO_H
#define SPIKE2VIDEO_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QFile>
#include <QList>
#include "tfirecon.h"
#include "tfprecon.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class Spike2video : public QThread
{
    Q_OBJECT
public:
    explicit Spike2video(QObject *parent = nullptr);
    ~Spike2video();

signals:
    void signal_src2video_process(int process, int total);


public slots:
    void slot_set_exprot_save_path(QString path);
    void slot_set_recon_img_func(int reon_img_func, int tfp_win);
    void slot_set_gamma_value(double new_gamm_value);
    void slot_set_img_size_file(QSize new_img_size, QString file_name);
    void slot_start_spike2video(void);
    void slot_stop_spike2video(void);
    void slot_set_img_mirror(int new_mirror_index);

protected:
    void run() override;

private:
    bool init_encoder(void);
    void convert_imgvideo_back_work(void);
    void convert_spike2video(void);
    bool getIs_runnig() const;
    void setIs_runnig(bool value);
    void mirror_img(QImage &src_img, QImage &recon_img);


private:
    AVFormatContext *m_format_ctx = nullptr;
    const AVCodec *m_codec = nullptr;
    AVStream *m_stream = nullptr;
    AVCodecContext *m_codec_ctx = nullptr;
    SwsContext *m_sws_ctx = nullptr;
    AVPacket* packet = nullptr;

    QString exprot_save_path;
    int64_t pts_counter = 0;

    bool m_encoding = false;

    QString m_spike_dat_path;
    QSize img_size;

    int mirror_index = 0;  //默认无镜像
    int recon_img_func = 0;
    int tfp_win = 0;
    int total_img_cnt = 0;

    TfiRecon * tfirecon = nullptr;
    TfpRecon * tfprecon = nullptr;
    bool is_runnig = false;

    QList<QImage> recon_img_list;
    int hanld_cnt = 0;

};

#endif // SPIKE2VIDEO_H

#ifndef RECONIMGTHREAD_H
#define RECONIMGTHREAD_H

#include <QObject>
#include <QByteArray>
#include <QImage>
#include <QFile>
#include <QThread>
#include <QList>
#include "tfprecon.h"
#include "tfirecon.h"

class ReconImgThread : public QThread
{
    Q_OBJECT
public:
    explicit ReconImgThread(QObject *parent = nullptr);
    ~ReconImgThread();

    void run();

    void setIs_pause(bool newIs_pause);

    bool getIs_pause() const;

private:


    int m_dispaly_interval_ms = 30;
    bool is_runnig = false;
    int m_mirror_index = 0;  //默认无镜像
    bool is_pause = false;
    int img_recon_func = 0;
    int m_play_img_index = 0;

    TfpRecon *tfp_recon = nullptr;
    TfiRecon *tfi_recon = nullptr;

signals:
    void signal_src_img(QImage src_img);
    void signal_recon_img(QImage recon_img);
    void signal_handler_percentage(int handler_cnt, int total_cnt);


public slots:
    void slot_set_img_size_file(QSize new_img_size, QString file_name);
    void slot_set_display_time(int time);
    void slot_set_src_data_file(QString file_name);
    void slot_set_start_recon(void);
    void slot_set_stop_recon(void);
    void slot_set_gamma_value(double new_gamm_value);
    void slot_set_pause_tfi_recon(void);
    void slot_set_img_mirror(int new_mirror_index);
    void slot_set_recon_img_func(int recon_func, int m_tfp_win);
    void slot_set_play_img_index(int index);
    void slot_setIs_runnig(bool newIs_runnig);

private:

    bool getIs_runnig() const;
    void emit_images_based_on_mirror_index(QImage& srcImg, QImage& reconImg);
    void thread_delay_ms(int ms);
};

#endif // RECONIMGTHREAD_H

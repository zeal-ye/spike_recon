#include "reconimgthread.h"
#include <QtMath>
#include <QThread>
#include <QApplication>
#include <QDebug>


ReconImgThread::ReconImgThread(QObject *parent) : QThread(parent)
{
    this->tfp_recon = new TfpRecon;
    this->tfi_recon = new TfiRecon;
}

ReconImgThread::~ReconImgThread()
{
    delete this->tfp_recon;
    delete this->tfi_recon;
}

void ReconImgThread::run()
{
    int process_cnt = 0, total_imgs = 0;
    QImage src_img, recon_img;

    while(this->getIs_runnig() && !this->isInterruptionRequested()) {
        if(this->img_recon_func == 0) {
            this->tfi_recon->get_one_src_recon_img(src_img, recon_img);
            this->tfi_recon->get_process_progress(process_cnt, total_imgs);

        } else {
            this->tfp_recon->get_one_src_recon_img(src_img, recon_img);
            this->tfp_recon->get_process_progress(process_cnt, total_imgs);
        }

        emit this->signal_handler_percentage(process_cnt, total_imgs);
        this->emit_images_based_on_mirror_index(src_img, recon_img);
        this->thread_delay_ms(this->m_dispaly_interval_ms);
    }
}

void ReconImgThread::setIs_pause(bool newIs_pause)
{
    is_pause = newIs_pause;
}

bool ReconImgThread::getIs_pause() const
{
    return is_pause;
}

void ReconImgThread::slot_setIs_runnig(bool newIs_runnig)
{
    is_runnig = newIs_runnig;
}

bool ReconImgThread::getIs_runnig() const
{
    return is_runnig;
}

void ReconImgThread::slot_set_img_size_file(QSize new_img_size, QString file_name)
{
    this->tfi_recon->set_recon_img_size(new_img_size);
    this->tfi_recon->set_spikedat_path(file_name);

    this->tfp_recon->set_recon_img_size(new_img_size);
    this->tfp_recon->set_spikedat_path(file_name);
}


void ReconImgThread::emit_images_based_on_mirror_index(QImage& srcImg, QImage& reconImg) {

    //选择不同的旋转方式
    switch (this->m_mirror_index) {
        case 0:
            emit this->signal_src_img(srcImg.copy());
            emit this->signal_recon_img(reconImg.copy());
            break;
        case 1:
            emit this->signal_src_img(srcImg.mirrored(true, false).copy());
            emit this->signal_recon_img(reconImg.mirrored(true, false).copy());
            break;
        case 2:
            emit this->signal_src_img(srcImg.mirrored(false, true).copy());
            emit this->signal_recon_img(reconImg.mirrored(false, true).copy());
            break;
    }
}

void ReconImgThread::thread_delay_ms(int ms)
{
    if (this->getIs_pause()) {
        while (this->getIs_pause()) {
            QApplication::processEvents();
            if (!this->getIs_runnig()) return;
            QThread::msleep(ms);
        }
    } else {
        QThread::msleep(ms);
        if (!this->getIs_runnig()) return;
    }

}

void ReconImgThread::slot_set_display_time(int time)
{
    this->m_dispaly_interval_ms = time;
}

void ReconImgThread::slot_set_src_data_file(QString file_name)
{
    if(file_name.isEmpty()) return;

    this->slot_setIs_runnig(false);
    this->requestInterruption();

    this->tfp_recon->set_spikedat_path(file_name);
    this->tfi_recon->set_spikedat_path(file_name);
    
}

void ReconImgThread::slot_set_start_recon()
{
    if(!this->getIs_runnig()) {
        this->slot_setIs_runnig(true);
    }

    if(this->getIs_pause()) {
        this->setIs_pause(false);
    }

    if(!this->isRunning()) {
        this->start();
    }
}

void ReconImgThread::slot_set_stop_recon()
{
    this->slot_setIs_runnig(false);
    this->requestInterruption();
}

void ReconImgThread::slot_set_gamma_value(double new_gamm_value)
{
    this->tfp_recon->setGamma_value(new_gamm_value);
    this->tfi_recon->setGamma_value(new_gamm_value);
}

void ReconImgThread::slot_set_pause_tfi_recon()
{
    this->setIs_pause(true);
}

void ReconImgThread::slot_set_img_mirror(int new_mirror_index)
{
    this->m_mirror_index = new_mirror_index;
}


void ReconImgThread::slot_set_recon_img_func(int recon_func, int tfp_win)
{
    this->img_recon_func = recon_func;
    this->tfp_recon->setTfp_win(tfp_win);
}

void ReconImgThread::slot_set_play_img_index(int index)
{
//    this->m_play_img_index = index;
    int process_cnt = 0, total_imgs = 0;
    QImage src_img, recon_img;

    if(this->img_recon_func == 0) {
        this->tfi_recon->get_one_src_recon_img(src_img, recon_img, index);
        this->tfi_recon->get_process_progress(process_cnt, total_imgs);
    } else {
        this->tfp_recon->get_one_src_recon_img(src_img, recon_img, index);
        this->tfp_recon->get_process_progress(process_cnt, total_imgs);
    }
    emit this->signal_handler_percentage(process_cnt, total_imgs);
    this->emit_images_based_on_mirror_index(src_img, recon_img);
    this->setIs_pause(true);
}




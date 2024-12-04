#include "spike2img.h"
#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include <QApplication>
#include <QDir>

Spike2Img::Spike2Img(QObject *parent)
    : QThread{parent}
{
    this->tfp_recon = new TfpRecon;
    this->tfi_recon = new TfiRecon;
}

Spike2Img::~Spike2Img()
{
    if(this->tfi_recon != nullptr) delete this->tfi_recon;
    if(this->tfp_recon != nullptr) delete this->tfp_recon;

}

void Spike2Img::run()
{
    QImage src_img, recon_img;

    this->handler_cnt = 0;
    this->total_img_cnt = this->tfp_recon->get_recon_total_cnt();
    int process = 0, process_total_cnt = 0;

    while(this->handler_cnt != this->total_img_cnt && !this->isInterruptionRequested()) {
        QApplication::processEvents();
        if(this->getIs_to_recon_img()) {
            if(this->recon_img_func == 0) {
                this->tfi_recon->get_one_src_recon_img(src_img, recon_img);
                this->tfi_recon->get_process_progress(process, process_total_cnt);
                this->miror_save_img(src_img, recon_img);
                this->handler_cnt++;
            } else {
                this->tfp_recon->get_one_src_recon_img(src_img, recon_img);
                this->tfp_recon->get_process_progress(process, process_total_cnt);
                this->miror_save_img(src_img, recon_img);
                this->handler_cnt += this->tfp_win;
            }
            emit this->signal_src2img_process(process, process_total_cnt);

        } else if(this->getIs_to_src_img()) {
            this->tfi_recon->get_one_src_recon_img(src_img, recon_img);
            this->tfi_recon->get_process_progress(process, process_total_cnt);
            this->miror_save_img(src_img, recon_img);
            this->handler_cnt++;
            emit this->signal_src2_src_img_process(process, process_total_cnt);
        }
    }


    this->setIs_to_recon_img(false);
    this->setIs_to_src_img(false);
}

void Spike2Img::slot_set_img_size_file(QSize new_img_size, QString file_name)
{
    this->tfp_recon->set_recon_img_size(new_img_size);
    this->tfi_recon->set_recon_img_size(new_img_size);

    if(this->m_spike_file_path != file_name) {
        this->tfp_recon->set_spikedat_path(file_name);
        this->tfi_recon->set_spikedat_path(file_name);
        this->m_spike_file_path = file_name;
    }
}

bool Spike2Img::getIs_to_src_img() const
{
    return is_to_src_img;
}

void Spike2Img::setIs_to_src_img(bool newIs_to_src_img)
{
    is_to_src_img = newIs_to_src_img;
}

bool Spike2Img::getIs_to_recon_img() const
{
    return is_to_recon_img;
}

void Spike2Img::setIs_to_recon_img(bool newIs_to_img)
{
    is_to_recon_img = newIs_to_img;
}

QImage Spike2Img::get_miror_img(QImage &img)
{
    switch(this->mirror_index) {
        case 0:
            return img.mirrored(false, false);
        break;

        case 1:
            return img.mirrored(true, false);
        break;

        case 2:
            return img.mirrored(false, true);
        break;
        default:
            return img;
    }
}

void Spike2Img::miror_save_img(QImage &src_img, QImage &recon_img)
{
    QFileInfo fileInfo(this->m_spike_file_path);
    QString save_path;

    if(this->getIs_to_src_img()) {
        src_img = get_miror_img(src_img);
        QString save_path = this->exprot_save_path + "/" + fileInfo.completeBaseName() +
                                                            "/src_img";
        QDir dir(save_path);

        if(!dir.exists()) {
            dir.mkpath(".");
        }
        QString export_path = save_path + "/";
        src_img.save(QString(export_path + "%1.jpg").arg(this->handler_cnt));
        return;
    } else if(this->getIs_to_recon_img()) {
        recon_img = get_miror_img(recon_img);
        if(this->recon_img_func == 0) {
            save_path = this->exprot_save_path + "/" + fileInfo.completeBaseName() +
                                                            "/tfi_recon_img";
            QDir dir(save_path);
            if(!dir.exists()) {
                dir.mkpath(".");
            }
        } else if(this->recon_img_func == 1) {
            save_path = this->exprot_save_path + "/" + fileInfo.completeBaseName() +
                                                            "/tfp_recon_img";
            QDir dir(save_path);
            if(!dir.exists()) {
                dir.mkpath(".");
            }
        }

        QString export_path = save_path + "/";
        recon_img.save(QString(export_path + "%1.jpg").arg(this->handler_cnt));
        return;
    }


}



bool Spike2Img::getIs_runnig() const
{
    return is_runnig;
}

void Spike2Img::setIs_runnig(bool newIs_runnig)
{
    is_runnig = newIs_runnig;
}


void Spike2Img::slot_set_start_spike2img()
{
    if(!this->isRunning()) {
        this->setIs_runnig(true);
        this->start();
        this->handler_cnt = 0;
    }
}

void Spike2Img::slot_set_to_recon_img(bool value)
{
    this->setIs_to_recon_img(value);
}

void Spike2Img::slot_set_to_src_img(bool value)
{
    this->setIs_to_src_img(value);
}

void Spike2Img::slot_set_exprot_save_path(QString path)
{
    this->exprot_save_path = path;
    QDir dir(this->exprot_save_path);

    if (!dir.exists()) {
        dir.mkpath(this->exprot_save_path);
    }
}

void Spike2Img::slot_set_img_mirror(int new_mirror_index)
{
    this->mirror_index = new_mirror_index;
}


void Spike2Img::slot_set_recon_img_func(int reon_img_func, int tfp_win)
{
    this->recon_img_func = reon_img_func;
    this->tfp_recon->setTfp_win(tfp_win);
    this->tfp_win = tfp_win;
}

void Spike2Img::slot_set_gamma_value(double new_gamm_value)
{
    this->tfp_recon->setGamma_value(new_gamm_value);
    this->tfi_recon->setGamma_value(new_gamm_value);
}


#ifndef SPIKE2IMG_H
#define SPIKE2IMG_H

#include <QThread>
#include <QImage>
#include <QFile>
#include <QList>
#include "tfirecon.h"
#include "tfprecon.h"




class Spike2Img : public QThread
{
    Q_OBJECT
public:
    explicit Spike2Img(QObject *parent = nullptr);
    ~Spike2Img(void);

    bool getIs_to_src_img() const;
    void setIs_to_src_img(bool newIs_to_src_img);

protected:
    void run(void) override;

private:

    TfiRecon * tfi_recon = nullptr;
    TfpRecon * tfp_recon = nullptr;

    QString exprot_save_path;
    QString m_spike_file_path;

    int total_img_cnt = 0;
    int handler_cnt = 0;

    bool is_runnig = false;


    bool is_to_recon_img = false;
    bool is_to_src_img = false;
    int mirror_index = 0;  //默认无镜像
    int recon_img_func = 0;
    int tfp_win = 0;

public slots:
    void slot_set_img_size_file(QSize new_img_size, QString file_name);
    void slot_set_start_spike2img(void);
    void slot_set_to_recon_img(bool value);
    void slot_set_to_src_img(bool value);
    void slot_set_exprot_save_path(QString path);
    void slot_set_img_mirror(int new_mirror_index);
    void slot_set_recon_img_func(int reon_img_func, int tfp_win);
    void slot_set_gamma_value(double new_gamm_value);

private:
    bool getIs_runnig() const;
    void setIs_runnig(bool newIs_runnig);

    bool getIs_to_recon_img() const;
    void setIs_to_recon_img(bool newIs_to_img);
    QImage get_miror_img(QImage& img);
    void miror_save_img(QImage& src_img, QImage& recon_img);

signals:
    void signal_src2img_process(int handler_cnt, int total_cnt);
    void signal_src2_src_img_process(int handler_cnt, int total_cnt);


};

#endif // SPIKE2IMG_H

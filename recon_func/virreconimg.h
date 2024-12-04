#ifndef VIRRECONIMG_H
#define VIRRECONIMG_H

#include "QFile"
#include "QImage"

class VirReconImg
{
public:
    virtual ~VirReconImg() = 0;
    virtual void set_spikedat_path(QString path) = 0;
    virtual void get_one_src_recon_img(QImage& src_img, QImage& recon_img, int index = -1) = 0;
    virtual void set_recon_img_size(QSize new_img_size) = 0;
    virtual void get_process_progress(int& process_cnt, int& total_imgs) = 0;
    virtual int get_recon_total_cnt() = 0;

};

#endif // VIRRECONIMG_H

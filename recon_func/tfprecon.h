#ifndef TFPRECON_H
#define TFPRECON_H

#include "virreconimg.h"
#include "QFile"


class TfpRecon : public VirReconImg
{
public:
    explicit TfpRecon();
    ~TfpRecon();

    void set_spikedat_path(QString path) override;
    void set_recon_img_size(QSize new_img_size) override;
    void get_one_src_recon_img(QImage& src_img, QImage& recon_img, int index = -1) override;
    void get_process_progress(int& process_cnt, int& total_imgs) override;
    int get_recon_total_cnt() override;

    void setTfp_win(int tfp_win);

    void setGamma_value(float gamma_value);

private:
    const int m_bitVal[9] {1,2,4,8,16,32,64,128,256};

    char * recon_img_dat = nullptr;
    char * src_img_dat = nullptr;

    int m_gammaLUT[256];
    float m_gamma_value = 2.2;
    int* m_merge_buf = nullptr;

    QSize m_recon_img_size;
    QFile m_spikedat_file;

    int m_total_number_of_imgs = 0;
    int m_tfp_win = 400;
    int m_play_img_index = 0;

};

#endif // TFPRECON_H

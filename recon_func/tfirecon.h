#ifndef TFIRECON_H
#define TFIRECON_H

#include "virreconimg.h"

class TfiRecon : public VirReconImg
{
public:
    explicit TfiRecon();
    ~TfiRecon();
    void set_spikedat_path(QString path) override;
    void get_one_src_recon_img(QImage &src_img, QImage &recon_img, int index = -1) override;
    void set_recon_img_size(QSize new_img_size) override;
    void get_process_progress(int& process_cnt, int& total_imgs) override;
    int get_recon_total_cnt() override;


    void setGamma_value(double gamma_value);

private:
    const int m_bitVal[9] {1,2,4,8,16,32,64,128,256};
    char * recon_img_dat = nullptr;
    char * src_img_dat = nullptr;

    int m_gammaLUT[256];
    double m_gamma_value = 2.2;


    QSize m_recon_img_size;
    QFile m_spikedat_file;

    int m_total_number_of_imgs = 0;
    int m_play_img_index = 0;

    int* m_prev_spikemnt = nullptr;
    int* m_next_spikemnt = nullptr;
    int* m_prev2moment_spikemnt = nullptr;

    QByteArray m_spike_dat;

};

#endif // TFIRECON_H

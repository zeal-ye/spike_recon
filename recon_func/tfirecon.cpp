#include "tfirecon.h"
#include "QtMath"
#include "QDebug"

TfiRecon::TfiRecon()
{
    for(int i = 0; i < 256; i++) {
        this->m_gammaLUT[i] =  (int)(255 * (pow((double)i / (double)255.0,
                                                    1.0/this->m_gamma_value)));
    }
}

TfiRecon::~TfiRecon()
{
    if(this->src_img_dat != nullptr) delete this->src_img_dat;
    if(this->recon_img_dat != nullptr) delete this->recon_img_dat;
    if(this->m_next_spikemnt != nullptr) delete this->m_next_spikemnt;
    if(this->m_prev_spikemnt != nullptr) delete this->m_prev_spikemnt;
    if(this->m_prev2moment_spikemnt != nullptr) delete this->m_prev2moment_spikemnt;
}

void TfiRecon::set_spikedat_path(QString path)
{
    if(path.isNull() || path.isEmpty()) return;
    if(this->m_spikedat_file.isOpen()) {
        this->m_spikedat_file.close();
    }

    this->m_spikedat_file.setFileName(path);
    if(!this->m_spikedat_file.open(QIODevice::ReadOnly)) return;
    this->m_total_number_of_imgs = (this->m_spikedat_file.size()/ ((
              this->m_recon_img_size.height() * this->m_recon_img_size.width()) / 8));

    this->m_play_img_index = 0;

}

void TfiRecon::get_one_src_recon_img(QImage &src_img, QImage &recon_img, int index)
{
    if (this->m_total_number_of_imgs <= 0 || index == this->m_play_img_index)
        return;

    if (index != -1 && index < this->m_total_number_of_imgs) {
        this->m_play_img_index = index;
    }

    int spike = 0, nextSpike = 0;
    const int width = this->m_recon_img_size.width(),
              height = this->m_recon_img_size.height();

    const int img_one_size = height * width;
    const int src_img_one_size = img_one_size / 8;
    if (this->m_play_img_index >= this->m_total_number_of_imgs)
        this->m_play_img_index = 0;

    if (this->m_play_img_index == 0) {
        this->m_spike_dat.clear();
        this->m_spikedat_file.seek(0);
        this->m_spike_dat = this->m_spikedat_file.readAll();
        memset(this->m_next_spikemnt, 0, height * width * sizeof(int));
        memset(this->m_prev_spikemnt, 0, height * width * sizeof(int));
        memset(this->m_prev2moment_spikemnt, 0, height * width * sizeof(int));
    }


    QByteArray handler_data = this->m_spike_dat;
    const uint8_t *handler_data_ptr = (const uint8_t *)handler_data.constData(); // 使用const指针访问QByteArray

    for (int pel = 0; pel < img_one_size; ++pel)
    {
        const int bytePos = pel >> 3;
        const int bitPos = pel & 7;
        spike = ((handler_data_ptr[this->m_play_img_index * src_img_one_size +
                                   bytePos] &
                  this->m_bitVal[bitPos]) > 0);

        if (spike > 0) {
            this->m_next_spikemnt[pel] = this->m_play_img_index;
        }

//        if (m_next_spikemnt[pel] == 0 || m_next_spikemnt[pel] < this->m_play_img_index) {
        if (m_next_spikemnt[pel] != this->m_play_img_index) {
            for (int nextMntIdx = this->m_play_img_index + 1; nextMntIdx < this->m_total_number_of_imgs; ++nextMntIdx) {
                nextSpike = ((handler_data_ptr[nextMntIdx * src_img_one_size +
                                               bytePos] &
                              this->m_bitVal[bitPos]) > 0);

                if (nextSpike > 0) {
                    m_next_spikemnt[pel] = nextMntIdx;
                    break;
                } else if (nextMntIdx == this->m_total_number_of_imgs - 1) {
                    m_next_spikemnt[pel] = m_prev2moment_spikemnt[pel];
                    break;
                }
            }
        }


        if(this->m_play_img_index != this->m_prev_spikemnt[pel] )
        for(int pre_mnt_idx = this->m_play_img_index - 1; pre_mnt_idx >= 0; pre_mnt_idx--) {
           int preSpike = ((handler_data_ptr[pre_mnt_idx * src_img_one_size +
                                          bytePos] &
                         this->m_bitVal[bitPos]) > 0);

           if (preSpike > 0) {
               m_prev_spikemnt[pel] = pre_mnt_idx;
               break;
           }
        }

        int interval = m_next_spikemnt[pel] - m_prev_spikemnt[pel];

        if (interval <= 0)
            interval = 255;

//        if (spike > 0)
//            m_prev_spikemnt[pel] = this->m_play_img_index;

        this->recon_img_dat[pel] = m_gammaLUT[255 / interval];
        this->src_img_dat[pel] = spike * 255;
    }

    this->m_play_img_index++;

    src_img = QImage((uchar *)this->src_img_dat, width, height,
                     QImage::Format_Grayscale8).copy();
    recon_img = QImage((uchar *)this->recon_img_dat, width, height,
                       QImage::Format_Grayscale8).copy();
}

void TfiRecon::set_recon_img_size(QSize new_img_size)
{

    if(this->m_recon_img_size == new_img_size) {
        this->m_recon_img_size = new_img_size;
       return;
    }

    this->m_recon_img_size = new_img_size;
    const int width = this->m_recon_img_size.width(),
            height = this->m_recon_img_size.height();

    size_t img_byte_size = width * height;


    if(this->src_img_dat != nullptr) delete this->src_img_dat;
    if(this->recon_img_dat != nullptr) delete this->recon_img_dat;
    if(this->m_next_spikemnt != nullptr) delete this->m_next_spikemnt;
    if(this->m_prev_spikemnt != nullptr) delete this->m_prev_spikemnt;
    if(this->m_prev2moment_spikemnt != nullptr) delete this->m_prev2moment_spikemnt;

    this->src_img_dat = new char[img_byte_size]();
    this->recon_img_dat = new char[img_byte_size]();
    this->m_next_spikemnt = new int[img_byte_size]();
    this->m_prev_spikemnt = new int[img_byte_size]();
    this->m_prev2moment_spikemnt = new int[img_byte_size]();

    this->m_play_img_index = 0;

}

void TfiRecon::get_process_progress(int& process_cnt, int& total_imgs)
{
    process_cnt = this->m_play_img_index, total_imgs = this->m_total_number_of_imgs;
}

int TfiRecon::get_recon_total_cnt()
{
    return this->m_total_number_of_imgs;
}

void TfiRecon::setGamma_value(double gamma_value)
{
    if(fabs(this->m_gamma_value - gamma_value) <= 1e-8) return;

    this->m_gamma_value = gamma_value;

    for(int i = 0; i < 256; i++) {
        this->m_gammaLUT[i] =  (int)(255 * (pow((double)i / (double)255.0,
                                                    1.0/this->m_gamma_value)));
    }
}

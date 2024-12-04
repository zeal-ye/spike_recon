#include "tfprecon.h"
#include "QtMath"
#include "QDebug"

TfpRecon::TfpRecon()
{
    for(int i = 0; i < 256; i++) {
        this->m_gammaLUT[i] =  (int)(255 * (pow((double)i / (double)255.0,
                                                    1.0/this->m_gamma_value)));
    }

}

TfpRecon::~TfpRecon()
{
    if(this->m_spikedat_file.isOpen()) {
        this->m_spikedat_file.close();
    }

    if(this->m_merge_buf != nullptr) delete this->m_merge_buf;
    if(this->src_img_dat != nullptr) delete this->src_img_dat;
    if(this->recon_img_dat != nullptr) delete this->recon_img_dat;
}

void TfpRecon::set_spikedat_path(QString path)
{
    if(path.isNull() || path.isEmpty()) return;
    if(this->m_spikedat_file.isOpen()) {
        this->m_spikedat_file.close();
    }

    this->m_play_img_index = 0;
    this->m_spikedat_file.setFileName(path);
    if(!this->m_spikedat_file.open(QIODevice::ReadOnly)) return;
    this->m_total_number_of_imgs = (this->m_spikedat_file.size()/ ((
              this->m_recon_img_size.height() * this->m_recon_img_size.width()) / 8));

}


void TfpRecon::set_recon_img_size(QSize new_img_size)
{
    if(this->m_recon_img_size == new_img_size) {
        this->m_recon_img_size = new_img_size;
       return;
    }


    this->m_recon_img_size = new_img_size;
    int width = this->m_recon_img_size.width(), height = this->m_recon_img_size.height();
    size_t img_byte_size = width * height;


    if(this->m_merge_buf != nullptr) delete this->m_merge_buf;
    if(this->src_img_dat != nullptr) delete this->src_img_dat;
    if(this->recon_img_dat != nullptr) delete this->recon_img_dat;

    this->m_merge_buf = new int[img_byte_size]();
    this->src_img_dat = new char[img_byte_size]();
    this->recon_img_dat = new char[img_byte_size]();
    this->m_play_img_index = 0;

}

void TfpRecon::get_one_src_recon_img(QImage &src_img, QImage &recon_img, int index)
{
    if (this->m_total_number_of_imgs <= 0 || this->m_play_img_index == index) return;

    if(index != -1) {this->m_play_img_index = index;}

    int spike = 0;
    const int width = this->m_recon_img_size.width(),
              height = this->m_recon_img_size.height();

    const int one_recon_img_size = width * height;
    const int src_img_one_size = one_recon_img_size / 8;

    const int pack_img = this->m_tfp_win;


    if(this->m_play_img_index >= this->m_total_number_of_imgs) {
        this->m_play_img_index = 0;
    }

    //防止出现重构图像不能被重构窗口整除的情况
    int handler_img_cnt = qMin(pack_img, this->m_total_number_of_imgs - this->m_play_img_index);

    this->m_spikedat_file.seek(this->m_play_img_index * src_img_one_size);
    QByteArray handler_data = this->m_spikedat_file.read(handler_img_cnt *
                                                       src_img_one_size);

    const uint8_t* data_ptr = (const uint8_t *)handler_data.constData();

    memset(this->m_merge_buf, 0, one_recon_img_size * sizeof(int));

    for (int j = 0; j < handler_img_cnt; j++) {
        for (int pel = 0; pel < one_recon_img_size; pel++) {
            const int bytePos = pel / 8;
            const int bitPos = pel % 8;
            spike = ((data_ptr[j * src_img_one_size + bytePos] & this->m_bitVal[bitPos]) > 0);
            this->m_merge_buf[pel] += spike;
            if(j == handler_img_cnt -1) {
                this->recon_img_dat[pel] = this->m_gammaLUT[(this->m_merge_buf[pel] * 255) /
                                                                handler_img_cnt];
                this->src_img_dat[pel] = spike * 255;
            }
        }
    }
    this->m_play_img_index += handler_img_cnt;


    src_img = QImage((uchar *)this->src_img_dat, width, height,
                                QImage::Format_Grayscale8).copy();
    recon_img = QImage((uchar *)this->recon_img_dat, width, height,
                                QImage::Format_Grayscale8).copy();

}

void TfpRecon::get_process_progress(int& process_cnt, int& total_imgs)
{
    process_cnt = this->m_play_img_index, total_imgs = this->m_total_number_of_imgs;
}

int TfpRecon::get_recon_total_cnt()
{
    return this->m_total_number_of_imgs;
}

void TfpRecon::setTfp_win(int tfp_win)
{
    m_tfp_win = tfp_win;
}

void TfpRecon::setGamma_value(float gamma_value)
{
    if(fabs(this->m_gamma_value - gamma_value) <= 1e-8) return;

    this->m_gamma_value = gamma_value;
    for(int i = 0; i < 256; i++) {
        this->m_gammaLUT[i] =  (int)(255 * (pow((double)i / (double)255.0,
                                                    1.0/this->m_gamma_value)));
    }
}

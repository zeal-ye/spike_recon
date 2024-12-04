#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QThread>
#include "reconimgthread.h"
#include "spike2img.h"
#include "spike2video.h"
#include <QTemporaryDir>
#include <QFile>


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:

    void on_pb_stop_clicked();
    void on_sb_dispaly_time_valueChanged(int arg1);
    void on_dsb_gamma_valueChanged(double arg1);
    void on_pb_pause_clicked();
    void on_pb_set_export_path_clicked();


    void on_pb_to_video_clicked();
    void on_pb_recon_to_img_clicked();
    void on_cb_rotate_currentIndexChanged(int index);
    void on_pb_src_to_img_clicked();
    void on_cb_recon_func_currentIndexChanged(int index);


    void on_pb_sel_dir_clicked();
    void on_pushButton_clicked();

    void on_hs_img_index_sliderMoved(int position);
    void on_hs_img_index_sliderReleased();
    void on_pb_play_clicked();
    void on_le_file_path_returnPressed();


private:
    Ui::Widget *ui;
    ReconImgThread *spike_recon;
    Spike2Img *spike2img;
    Spike2video * spike2video;


    QByteArray src_data;
    QString temporary_src_file_path;

    QString temp_path;
    QString spike_dat_path;

    int hs_slider_value = 0;
    int total_imgs = 0;

signals:
    void signal_set_img_size_file(QSize img_size, QString file_name);
    void signal_set_display_time(int time);
    void signal_set_src_data_file(QString file_name);

    void signal_set_start_recon(void);
    void signal_set_stop_recon(void);

    void signal_set_start_spike2video(void);
    void signal_set_stop_spike2video(void);

    void signal_set_gamma_value(double new_gamma_value);
    void signal_set_pause_recon(void);
    void signal_set_start_spike2img(void);
    void signal_set_is_to_recon_img(bool value);
    void signal_set_is_to_src_img(bool value);
    void signal_set_imgvideo_save_path(QString path);
    void signal_set_img_mirror(int angle);
    void signal_set_camera_class(int new_camera_class);
    void signal_set_img_recon_func(int img_func, int tfp_win);
    void signal_set_play_img_index(int index);

public slots:
    void set_src_img(QImage src_img);
    void set_recon_img(QImage tfi_img);
    void slot_set_handler_percentage(int handler_cnt, int total_cnt);
    void slot_spike2recon_img_process(int handler_cnt, int total_cnt);
    void slot_spike2_src_img_process(int handler_cnt, int total_cnt);
    void slot_src2video_process(int handler_cnt, int total_cnt);

private:
    void config_file_init(void);
    void config_file_save();
    void mkdir_clear_path(QString path);
    QString move_file_to_temp_path(QFile& file);
    void set_ui_status_start(void);
    void set_ui_status_stop(void);
    void tabw_src_dir_init(void);
    void get_src_dir_filelist(QString path);

    void clear_selection();
    int handle_selected_files();
    QStringList get_selected_filenames(void);
    void handle_singlefile(const QString &file_path, int img_width, int img_height);
    void handle_multiplefiles(const QStringList &file_names, int img_width, int img_height);

    void set_img_size_recon_func(void);

    void signal_slot_init(void);

};

#endif // WIDGET_H

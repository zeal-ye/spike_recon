#include "widget.h"
#include "./ui_widget.h"
#include "QFileDialog"
#include <QFile>
#include <QDebug>
#include <QSettings>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include "QDateTime"
#include "QCollator"


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    this->setWindowTitle("Spike recon v2.7.1");
    this->setWindowIcon(QIcon(":/icon_camera.png"));

    this->spike_recon = new ReconImgThread(this);
    this->spike2img = new Spike2Img(this);
    this->spike2video = new Spike2video(this);

    this->tabw_src_dir_init();
    this->set_ui_status_stop();
    this->ui->pb_play->setEnabled(false);
    this->ui->hs_img_index->setTracking(false);


    this->signal_slot_init();

    this->config_file_init();
    this->on_cb_recon_func_currentIndexChanged(this->ui->cb_recon_func->currentIndex());
    this->on_cb_rotate_currentIndexChanged(this->ui->cb_rotate->currentIndex());

    this->temp_path = QString(QDir::tempPath() + "/spikecv_reconimg_123");
    this->mkdir_clear_path(this->temp_path);
}

Widget::~Widget()
{
    this->config_file_save();
    emit this->signal_set_stop_recon();
    this->spike2video->requestInterruption();
    this->spike2img->requestInterruption();
    this->spike_recon->requestInterruption();

    this->spike_recon->quit();
    this->spike2img->quit();
    this->spike2video->quit();

    this->spike_recon->wait(500);
    this->mkdir_clear_path(this->temp_path);
    delete ui;


}


void Widget::set_src_img(QImage src_img)
{
    this->ui->lb_src_img->setPixmap(QPixmap::fromImage(src_img).scaled(this->ui->lb_src_img->size(), Qt::KeepAspectRatio));
}

void Widget::set_recon_img(QImage tfi_img)
{

    this->ui->lb_tfi_img->setPixmap(QPixmap::fromImage(tfi_img).scaled(this->ui->lb_tfi_img->size(), Qt::KeepAspectRatio));
}

void Widget::slot_set_handler_percentage(int handler_cnt, int total_cnt)
{

    this->total_imgs = total_cnt;
    if(this->ui->hs_img_index->maximum() != total_cnt) {
        this->ui->hs_img_index->setMaximum(total_cnt);
        this->ui->hs_img_index->setMinimum(0);
    }

    this->ui->hs_img_index->setValue(handler_cnt);
    QString format = QString("%1/%2").arg(handler_cnt).arg(total_cnt);
    this->ui->lb_dis_index->setText(format);
}

void Widget::slot_spike2recon_img_process(int handler_cnt, int total_cnt)
{
    this->ui->pb_img_process->setRange(0, total_cnt);
    this->ui->pb_img_process->setValue(handler_cnt);
    QString format = QString("%1/%2").arg(handler_cnt).arg(total_cnt);
    this->ui->pb_img_process->setFormat(format);
    if(handler_cnt == total_cnt) {
        this->ui->pb_recon_to_img->setText("保存重构图片");
        this->ui->pb_src_to_img->setEnabled(true);
    }
}

void Widget::slot_spike2_src_img_process(int handler_cnt, int total_cnt)
{
    this->ui->pb_src_img_process->setRange(0, total_cnt);
    this->ui->pb_src_img_process->setValue(handler_cnt);
    QString format = QString("%1/%2").arg(handler_cnt).arg(total_cnt);
    this->ui->pb_src_img_process->setFormat(format);

    if(handler_cnt == total_cnt) {
        this->ui->pb_src_to_img->setText("保存源图片");
        this->ui->pb_recon_to_img->setEnabled(true);
    }
}

void Widget::slot_src2video_process(int handler_cnt, int total_cnt)
{
    this->ui->pb_video_process->setRange(0, total_cnt);
    this->ui->pb_video_process->setValue(handler_cnt);
    QString format = QString("%1/%2").arg(handler_cnt).arg(total_cnt);
    this->ui->pb_video_process->setFormat(format);
    if(handler_cnt == total_cnt) {
        this->ui->pb_to_video->setText("转为视频");
    }
}

static int ceil2window(int value, int windowSize) {
    return (value % windowSize == 0) ? value : ((value / windowSize) + 1) * windowSize;
}
void Widget::on_hs_img_index_sliderMoved(int position)
{
    this->hs_slider_value = position;

    int tfp_win = this->ui->sb_tfp_win->value();

    // 根据选择的重建函数调整显示值
    int display_value = (this->ui->cb_recon_func->currentIndex() != 0)
                        ? ceil2window(position, tfp_win)
                        : position;
    // 更新显示标签
    QString format = QString("%1/%2").arg(display_value).arg(this->total_imgs);
    this->ui->lb_dis_index->setText(format);

}

void Widget::on_hs_img_index_sliderReleased()
{
    int position = this->hs_slider_value;
    int tfp_win = this->ui->sb_tfp_win->value(),
            display_value = position;

    emit this->signal_set_pause_recon();

    if(this->ui->cb_recon_func->currentIndex() == 0) {
        emit this->signal_set_play_img_index(position);
    } else {
        display_value = ceil2window(position, tfp_win);
        emit this->signal_set_play_img_index(display_value);
        this->ui->hs_img_index->setValue(display_value);
    }
    QString format = QString("%1/%2").arg(display_value).arg(this->total_imgs);
    this->ui->lb_dis_index->setText(format);

    this->ui->pb_pause->setText("继续");
    this->ui->pb_pause->setShortcut(QKeySequence(Qt::Key_Space));
}

void Widget::on_pb_play_clicked()
{

    this->set_img_size_recon_func();
    emit this->signal_set_display_time(this->ui->sb_dispaly_time->value());
    this->set_ui_status_start();
    this->ui->sb_tfp_win->setEnabled(false);
    this->ui->cb_recon_func->setEnabled(false);
    emit this->signal_set_start_recon();
}

void Widget::on_le_file_path_returnPressed()
{
    QString src_path = this->ui->le_file_path->text();
    QDir dir(src_path);
    if(!dir.exists()) {
        return;
    }
    this->get_src_dir_filelist(src_path);
}

void Widget::on_pb_stop_clicked()
{
    this->set_ui_status_stop();

    emit this->signal_set_stop_recon();
}


void Widget::on_sb_dispaly_time_valueChanged(int arg1)
{
    emit this->signal_set_display_time(arg1);
}

void Widget::on_dsb_gamma_valueChanged(double arg1)
{
    emit this->signal_set_gamma_value(arg1);
}


void Widget::on_pb_pause_clicked()
{
    QString pb_text = this->ui->pb_pause->text();
    if(pb_text == "继续") {
        emit this->signal_set_start_recon();
        this->ui->pb_pause->setText("暂停");
        this->ui->pb_pause->setShortcut(QKeySequence(Qt::Key_Space));
    } else {
        emit this->signal_set_pause_recon();
        this->ui->pb_pause->setText("继续");
        this->ui->pb_pause->setShortcut(QKeySequence(Qt::Key_Space));

    }
}


void Widget::on_pb_set_export_path_clicked()
{
    QString last_path = this->ui->le_export_path->text();

    QString fileName = QFileDialog::getExistingDirectory(this,
                                tr("选择保存路径"),
                                last_path);

    if (!fileName.isEmpty()) {
        ui->le_export_path->setText(fileName);
        emit this->signal_set_imgvideo_save_path(this->ui->le_export_path->text());
    }
}


void Widget::on_pb_to_video_clicked()
{

    QString text = this->ui->pb_to_video->text();
    if(text == "停止转为视频") {

        this->ui->pb_to_video->setText("转为视频");
        this->ui->pb_video_process->setEnabled(false);
        this->ui->lb_video->setEnabled(false);
        emit this->signal_set_stop_spike2video();

    } else if(text == "转为视频") {
        this->set_img_size_recon_func();
        this->ui->pb_to_video->setText("停止转为视频");
        this->ui->pb_video_process->setEnabled(true);
        this->ui->lb_video->setEnabled(true);
        emit this->signal_set_start_spike2video();
    }

}


void Widget::on_pb_recon_to_img_clicked()
{
    QString text = this->ui->pb_recon_to_img->text();

    if(text == "停止保存重构图片") {
        this->ui->pb_recon_to_img->setText("保存重构图片");
        emit this->signal_set_is_to_recon_img(false);
        this->ui->pb_img_process->setEnabled(false);
        this->ui->lb_img->setEnabled(false);
        this->ui->pb_src_to_img->setEnabled(true);
    } else if(text == "保存重构图片") {
        this->ui->pb_recon_to_img->setText("停止保存重构图片");
        this->set_img_size_recon_func();

        emit this->signal_set_is_to_recon_img(true);
        emit this->signal_set_start_spike2img();
        this->ui->pb_img_process->setEnabled(true);
        this->ui->lb_img->setEnabled(true);
        this->ui->pb_src_to_img->setEnabled(false);
    }

}


void Widget::on_cb_rotate_currentIndexChanged(int index)
{
    emit this->signal_set_img_mirror(index);
}


void Widget::on_pb_src_to_img_clicked()
{

    QString text = this->ui->pb_src_to_img->text();

    if(text == "停止保存源图片") {
        this->ui->pb_src_to_img->setText("保存源图片");
        emit this->signal_set_is_to_src_img(false);
        this->ui->pb_src_img_process->setEnabled(false);
        this->ui->lb_src_img_2->setEnabled(false);
        this->ui->pb_recon_to_img->setEnabled(true);
    } else if(text == "保存源图片"){

        this->ui->pb_src_to_img->setText("停止保存源图片");
        this->set_img_size_recon_func();
        emit this->signal_set_is_to_src_img(true);
        emit this->signal_set_start_spike2img();
        this->ui->pb_src_img_process->setEnabled(true);
        this->ui->lb_src_img_2->setEnabled(true);
        this->ui->pb_recon_to_img->setEnabled(false);

    }

}


void Widget::on_cb_recon_func_currentIndexChanged(int index)
{
    if(index== 0) {
        this->ui->sb_tfp_win->setEnabled(false);
    } else {
        this->ui->sb_tfp_win->setEnabled(true);
    }
}


void Widget::on_pb_sel_dir_clicked()
{
    static QString last_path;
    QString open_path;

    if(last_path.isEmpty()) {
        open_path = QDir::currentPath();
    } else {
        open_path = last_path;
    }

    QString directory = QFileDialog::getExistingDirectory(
        nullptr,  // 父窗口，这里设为 nullptr 表示没有父窗口
        "选择目录",  // 对话框标题
        open_path,  // 默认打开的目录
        QFileDialog::ShowDirsOnly
    );

    if (!directory.isEmpty()) {
        last_path = directory;
        this->ui->le_file_path->setText(last_path);
        this->get_src_dir_filelist(last_path);
    } else {
        return;
    }

}


void Widget::on_pushButton_clicked()
{
    if (ui->pushButton->text() == "清除选中") {
        clear_selection();
        return;
    }

    if(handle_selected_files() == -1) return;


    this->ui->pb_play->setEnabled(true);
    this->ui->pb_sel_dir->setEnabled(false);
    this->ui->le_file_path->setEnabled(false);
    set_ui_status_start();
}

void Widget::clear_selection()
{
    ui->tabw_src_dir->clearSelection();
    ui->tabw_src_dir->setEnabled(true);
    ui->pushButton->setText("选中");
    this->ui->pb_play->setEnabled(false);
    this->ui->pb_sel_dir->setEnabled(true);
    this->ui->le_file_path->setEnabled(true);

    this->set_ui_status_stop();

    emit this->signal_set_stop_recon();
    this->spike_recon->slot_setIs_runnig(false);

}

int Widget::handle_selected_files()
{
    mkdir_clear_path(temp_path);

    QStringList file_names = get_selected_filenames();

    if (file_names.isEmpty()) {
        return -1;
    }

    ui->pushButton->setText("清除选中");
    ui->tabw_src_dir->setEnabled(false);

    emit signal_set_stop_recon();
    spike_recon->slot_setIs_runnig(false);

    int img_width = ui->sb_img_width->value();
    int img_height = ui->sb_img_height->value();

    if (file_names.size() == 1) {
        handle_singlefile(file_names.first(), img_width, img_height);
        return 0;
    }

    handle_multiplefiles(file_names, img_width, img_height);
    return 0;
}

QStringList Widget::get_selected_filenames()
{
    QStringList file_names;
    QList<QTableWidgetItem*> selectedItems = ui->tabw_src_dir->selectedItems();

    for (QTableWidgetItem *item : selectedItems) {
        if (item->column() == 0) {
            file_names.append(ui->le_file_path->text() + "/" + item->text());
        }
    }

    return file_names;
}

void Widget::handle_singlefile(const QString &file_path, int img_width, int img_height)
{
    QFile spike_file(file_path);
    if (!spike_file.open(QIODevice::ReadOnly) || spike_file.size() < (img_width * img_height / 8)) {
        spike_file.close();
        return;
    }

    QString temp_file_path = move_file_to_temp_path(spike_file);
    ui->le_export_path->setText(QFileInfo(spike_file).path());
    this->spike_dat_path = temp_file_path;
    this->set_img_size_recon_func();
    emit signal_set_imgvideo_save_path(ui->le_export_path->text());
}

void Widget::handle_multiplefiles(const QStringList &file_names, int img_width, int img_height)
{
    QString ex_path;
    QString temp_file_path = temp_path + "/all_";

    //使用所选文件的文件名后两个字符做新的文件名
    for (const QString &file_path : file_names) {
        ex_path = QFileInfo(file_path).path();
        QString base_name = QFileInfo(file_path).baseName();
        QString last_four_chars = base_name.right(2);
        temp_file_path += last_four_chars;
    }
    temp_file_path += ".dat";

    QFile temp_src_file(temp_file_path);
    if (!temp_src_file.open(QIODevice::WriteOnly)) {
        return;
    }

    for (const QString &file_path : file_names) {
        QFile spike_file(file_path);
        if (!spike_file.open(QIODevice::ReadOnly) || spike_file.size() < (img_width * img_height / 8)) {
            spike_file.close();
            temp_src_file.close();
            return;
        }
        temp_src_file.write(spike_file.readAll());
    }

    temp_src_file.close();
    ui->le_export_path->setText(ex_path);

    this->spike_dat_path = temp_file_path;

    emit signal_set_img_size_file(QSize(img_width, img_height), temp_file_path);
    emit signal_set_imgvideo_save_path(ui->le_export_path->text());
}

void Widget::config_file_init()
{
    // 获取程序所在的目录
    QString appDirPath = QCoreApplication::applicationDirPath();
    // 构建settings.ini文件的完整路径
    QString settingsFilePath = QDir(appDirPath).filePath("settings.ini");

    // 使用QSettings，指定完整的文件路径和INI格式
    QSettings settings(settingsFilePath, QSettings::IniFormat);

    // 检查文件是否存在（这一步是可选的，因为QSettings会自动处理）
    QFile file(settingsFilePath);
    if (!file.exists()) {
        return;
    }

    this->ui->sb_img_width->setValue(settings.value("img_format/width").toInt());
    this->ui->sb_img_height->setValue(settings.value("img_format/height").toInt());
    this->ui->cb_rotate->setCurrentIndex(settings.value("img_format/img_mirror").toInt());
    this->ui->sb_tfp_win->setValue(settings.value("img_recon/tfp_win").toInt());
    this->ui->cb_recon_func->setCurrentIndex(settings.value("img_recon/recon_func").toInt());
}

void Widget::config_file_save()
{
    // 获取程序所在的目录
    QString appDirPath = QCoreApplication::applicationDirPath();
    // 构建settings.ini文件的完整路径
    QString settingsFilePath = QDir(appDirPath).filePath("settings.ini");

    // 使用QSettings，指定完整的文件路径和INI格式
    QSettings settings(settingsFilePath, QSettings::IniFormat);
//    qDebug() << "setingsFIlepath:" << settingsFilePath;

    settings.setValue("img_format/width", this->ui->sb_img_width->value());
    settings.setValue("img_format/height", this->ui->sb_img_height->value());
    settings.setValue("img_format/img_mirror", this->ui->cb_rotate->currentIndex());
    settings.setValue("img_recon/tfp_win", this->ui->sb_tfp_win->value());
    settings.setValue("img_recon/recon_func", this->ui->cb_recon_func->currentIndex());

}


void Widget::mkdir_clear_path(QString dirPath)
{
    QDir dir(dirPath);
    // 检查目录是否存在
    if (dir.exists()) {
        // 如果目录存在，则删除它及其所有内容
        if (!dir.removeRecursively()) {
            qDebug() << "无法删除目录及其内容：" << dirPath;
            return;
        }
        qDebug() << "目录已删除：" << dirPath;
    }

    // 尝试重新创建目录
    if (!dir.mkpath(dirPath)) {
        qDebug() << "无法创建目录：" << dirPath;
        return;
    }
    qDebug() << "目录已重新创建：" << dirPath;
    return;

}

QString Widget::move_file_to_temp_path(QFile& file)
{
    if(!file.exists()) return QString();

    QFileInfo file_info(file);
    QString file_name = file_info.fileName();
    QString temp_path_name = QString(this->temp_path + "/" + file_name);
    QFile temp_file(temp_path_name);

    temp_file.open(QIODevice::WriteOnly);

    if(file.isOpen()) {
        temp_file.write(file.readAll());
    } else {
        file.open(QIODevice::ReadOnly);
        temp_file.write(file.readAll());
    }
    temp_file.close();

    return temp_path_name;
}

void Widget::set_ui_status_start()
{

    this->ui->sb_img_height->setEnabled(false);
    this->ui->sb_img_width->setEnabled(false);
    this->ui->pb_play->setEnabled(true);
    this->ui->pb_stop->setEnabled(true);
    this->ui->pb_pause->setEnabled(true);
    this->ui->pb_to_video->setEnabled(true);
    this->ui->pb_recon_to_img->setEnabled(true);
    this->ui->pb_src_to_img->setEnabled(true);

    this->ui->hs_img_index->setEnabled(true);
    this->ui->lb_dis_index->setEnabled(true);

    this->ui->cb_recon_func->setEnabled(true);
    this->ui->sb_tfp_win->setEnabled(true);


    this->ui->pb_video_process->setRange(0, 1);
    this->ui->pb_img_process->setRange(0, 1);
    this->ui->pb_src_img_process->setRange(0, 1);

    this->ui->pb_video_process->setValue(0);
    this->ui->pb_img_process->setValue(0);
    this->ui->pb_src_img_process->setValue(0);
}

void Widget::set_ui_status_stop()
{

    this->ui->sb_img_height->setEnabled(true);
    this->ui->sb_img_width->setEnabled(true);
    this->ui->pb_pause->setEnabled(false);
    this->ui->pb_stop->setEnabled(false);

    this->ui->hs_img_index->setEnabled(false);
    this->ui->lb_dis_index->setEnabled(false);

    this->ui->cb_recon_func->setEnabled(true);
    if(this->ui->cb_recon_func->currentIndex() == 0) {
        this->ui->sb_tfp_win->setEnabled(false);
    } else {
        this->ui->sb_tfp_win->setEnabled(true);
    }

    this->ui->pb_video_process->setRange(0, 1);
    this->ui->pb_img_process->setRange(0, 1);
    this->ui->pb_src_img_process->setRange(0, 1);

    this->ui->pb_video_process->setValue(0);
    this->ui->pb_img_process->setValue(0);
    this->ui->pb_src_img_process->setValue(0);

    QString format = QString("%1/%2").arg(0).arg(0);
    this->ui->pb_src_img_process->setFormat(format);
    this->ui->pb_video_process->setFormat(format);
    this->ui->pb_img_process->setFormat(format);

    this->ui->pb_video_process->setEnabled(false);
    this->ui->pb_src_img_process->setEnabled(false);
    this->ui->pb_img_process->setEnabled(false);

    this->ui->lb_video->setEnabled(false);
    this->ui->lb_img->setEnabled(false);
    this->ui->lb_src_img_2->setEnabled(false);
}


void Widget::tabw_src_dir_init()
{
    this->ui->tabw_src_dir->setSelectionMode(QAbstractItemView::ExtendedSelection);
//    this->ui->tabw_src_dir->setSelectionMode(QAbstractItemView::MultiSelection);
    this->ui->tabw_src_dir->setContextMenuPolicy(Qt::CustomContextMenu);
    this->ui->tabw_src_dir->setColumnCount(1);
    QStringList headerLabels;
        headerLabels << "数据文件名"; // 设置列标题
    this->ui->tabw_src_dir->setHorizontalHeaderLabels(headerLabels); // 设置水平表头标签
    this->ui->tabw_src_dir->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    this->ui->tabw_src_dir->setEditTriggers(QAbstractItemView::NoEditTriggers);

}

void Widget::get_src_dir_filelist(QString path)
{
    if(path.isEmpty()) return;
    QDir dir(path);
    if (!dir.exists()) {
        return;
    }

    this->ui->tabw_src_dir->clearContents(); // 清除现有内容
    this->ui->tabw_src_dir->setRowCount(0);
    this->ui->tabw_src_dir->setColumnCount(0);

    this->ui->tabw_src_dir->setColumnCount(1);
    QStringList headerLabels;
    headerLabels << "数据文件名"; // 设置列标题
    this->ui->tabw_src_dir->setHorizontalHeaderLabels(headerLabels); // 设置水平表头标签
    this->ui->tabw_src_dir->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    this->ui->tabw_src_dir->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 获取文件夹中所有文件的列表，并过滤特定后缀的文件
    QStringList fileList = dir.entryList({"*.dat", "*.bin"}, QDir::Files);

    //排序文件
    QCollator collator;
    collator.setNumericMode(true);
    std::sort(fileList.begin(), fileList.end(), collator);

    // 遍历文件列表并添加到表格中
    for (const QString &fileName : fileList) {
        int rowCount = this->ui->tabw_src_dir->rowCount(); // 获取当前行数
        this->ui->tabw_src_dir->insertRow(rowCount); // 插入新行
//        qDebug() << fileName;
        QFileInfo fileInfo(dir.filePath(fileName));

        // 设置文件名
        QTableWidgetItem *itemName = new QTableWidgetItem(fileName);
        this->ui->tabw_src_dir->setItem(rowCount, 0, itemName);
    }

    // 调整列宽以适应内容
    this->ui->tabw_src_dir->resizeColumnsToContents();
    // 显示表格
//    this->ui->tabw_radio_file->show();
}


void Widget::set_img_size_recon_func()
{
    int width = this->ui->sb_img_width->value(),
            height = this->ui->sb_img_height->value();
    int recon_func = this->ui->cb_recon_func->currentIndex(),
            tfp_win = this->ui->sb_tfp_win->value();

    emit this->signal_set_img_size_file(QSize(width, height), this->spike_dat_path);
    emit this->signal_set_img_recon_func(recon_func, tfp_win);

}

void Widget::signal_slot_init()
{
    connect(this, &Widget::signal_set_img_size_file, this->spike_recon, &ReconImgThread::slot_set_img_size_file);
    connect(this, &Widget::signal_set_src_data_file, this->spike_recon, &ReconImgThread::slot_set_src_data_file);;
    connect(this, &Widget::signal_set_start_recon, this->spike_recon, &ReconImgThread::slot_set_start_recon);
    connect(this->spike_recon, &ReconImgThread::signal_src_img, this, &Widget::set_src_img);
    connect(this->spike_recon, &ReconImgThread::signal_recon_img, this, &Widget::set_recon_img);
    connect(this, &Widget::signal_set_img_recon_func, this->spike_recon, &ReconImgThread::slot_set_recon_img_func);
    connect(this, &Widget::signal_set_img_recon_func, this->spike2img, &Spike2Img::slot_set_recon_img_func);
    connect(this, &Widget::signal_set_img_mirror, this->spike_recon, &ReconImgThread::slot_set_img_mirror);

    connect(this, &Widget::signal_set_display_time, this->spike_recon, &ReconImgThread::slot_set_display_time);
    connect(this, &Widget::signal_set_stop_recon, this->spike_recon, &ReconImgThread::slot_set_stop_recon);
    connect(this->spike_recon, &ReconImgThread::signal_handler_percentage, this, &Widget::slot_set_handler_percentage);
    connect(this, &Widget::signal_set_play_img_index, this->spike_recon, &ReconImgThread::slot_set_play_img_index);
    connect(this, &Widget::signal_set_gamma_value, this->spike_recon, &ReconImgThread::slot_set_gamma_value);
    connect(this, &Widget::signal_set_gamma_value, this->spike2img, &Spike2Img::slot_set_gamma_value);
    connect(this, &Widget::signal_set_pause_recon, this->spike_recon, &ReconImgThread::slot_set_pause_tfi_recon);

    connect(this, &Widget::signal_set_start_spike2img, this->spike2img, &Spike2Img::slot_set_start_spike2img);
    connect(this, &Widget::signal_set_img_size_file, this->spike2img, &Spike2Img::slot_set_img_size_file);
    connect(this, &Widget::signal_set_imgvideo_save_path, this->spike2img, &Spike2Img::slot_set_exprot_save_path);
    connect(this, &Widget::signal_set_is_to_recon_img, this->spike2img, &Spike2Img::slot_set_to_recon_img);
    connect(this, &Widget::signal_set_is_to_src_img, this->spike2img, &Spike2Img::slot_set_to_src_img);
    connect(this->spike2img, &Spike2Img::signal_src2img_process, this, &Widget::slot_spike2recon_img_process);
    connect(this->spike2img, &Spike2Img::signal_src2_src_img_process, this, &Widget::slot_spike2_src_img_process);
    connect(this, &Widget::signal_set_img_mirror, this->spike2img, &Spike2Img::slot_set_img_mirror);

    connect(this, &Widget::signal_set_img_mirror, this->spike2video, &Spike2video::slot_set_img_mirror);
    connect(this, &Widget::signal_set_stop_spike2video, this->spike2video, &Spike2video::slot_stop_spike2video);
    connect(this, &Widget::signal_set_start_spike2video, this->spike2video, &Spike2video::slot_start_spike2video);
    connect(this, &Widget::signal_set_gamma_value, this->spike2video, &Spike2video::slot_set_gamma_value);
    connect(this, &Widget::signal_set_img_size_file, this->spike2video, &Spike2video::slot_set_img_size_file);
    connect(this, &Widget::signal_set_img_recon_func, this->spike2video, &Spike2video::slot_set_recon_img_func);
    connect(this, &Widget::signal_set_imgvideo_save_path, this->spike2video, &Spike2video::slot_set_exprot_save_path);

    connect(this->spike2video, &Spike2video::signal_src2video_process, this, &Widget::slot_src2video_process);
}




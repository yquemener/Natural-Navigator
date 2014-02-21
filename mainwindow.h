/*
Natural Navigator : an interface for the kinect
Copyright (C) 2011 Yves Quemener, IV-devs, Creartcom

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    */
    

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "display/glwidget.h"
#include <QTimer>
#include <QTime>
#include <QFile>
#include <QSettings>
#include "shareddata.h"
#include <QUdpSocket>
#include <limits>
#include <cflie/CCrazyflie.h>


namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    ZCamProcessing m_proc;
    GLWidget m_gl;
    GLWidget m_gl_top_view;
    QTimer m_clock;
    QSettings * m_settings;
    QTime m_timer;
    QTimer m_timer_learn;

    float m_z_near, m_z_far;
    float m_fps;

    QFile m_trajfile;
    QTextStream m_trajstream;

    SharedStruct::scene * m_pSharedData;

    CCrazyRadio * m_crRadio;
    CCrazyflie * m_cflieCopter;


private slots:
    void on_ui_visible_clicked();
    void on_but_deactivate_display_clicked();
    void on_lst_boxes_currentRowChanged(int currentRow);
    void on_pushButton_clicked();
    void on_sld_z_near_sliderMoved(int position);
    void on_lst_boxes_itemSelectionChanged();
    void on_but_add_clicked();
    void on_chk_boxes_clicked();
    void on_sld_z_far_valueChanged(int value);
    void on_sld_z_near_valueChanged(int value);
    void do_refreshVideo();
    void do_calib_changed();
    void on_but_background_depth_clicked();
    void on_but_view_reset_clicked();
    void on_but_reset_background_depth_clicked();
    void on_but_view_plongeante_clicked();
    void on_but_view_reset_3_clicked();
    void on_but_view_reset_4_clicked();
    void on_but_reset_boxes_clicked();
    void on_but_record_trajectory_toggled(bool checked);
};


/*QUdpSocket * udpSocket = new QUdpSocket(this);
udpSocket->bind(QHostAddress("192.168.0.1"), 7755);
QByteArray a;
a.append("toto");
udpSocket->writeDatagram(a,QHostAddress(ui->EdtIp->text()),
													ui->EdtPort->text().toInt());
udpSocket->flush();*/
#endif // MAINWINDOW_H

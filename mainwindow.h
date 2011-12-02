// (c) 2011 IV-devs, Yves Quemener

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "display/glwidget.h"
#include <QTimer>
#include <QSettings>
#include "shareddata.h"
#include <QUdpSocket>


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
		GLWidget m_gl;
		QTimer m_clock;
		QSettings * m_settings;
		QUdpSocket * m_udpSocket;

		float m_z_near, m_z_far;


		SharedStruct::scene * m_pSharedData;


private slots:
	void on_sld_left_margin_valueChanged(int value);
 void on_sld_right_margin_valueChanged(int value);
 void on_ui_visible_clicked();
 void on_sld_depth_boxes_sliderMoved(int position);
 void on_sld_depth_boxes_actionTriggered(int action);
 void on_but_deactivate_display_clicked();
 void on_sld_right_zone_sliderMoved(int position);
 void on_sld_left_zone_sliderMoved(int position);
 void on_sld_top_zone_sliderMoved(int position);
 void on_sld_z_blob_sliderMoved(int position);
 void on_lst_boxes_currentRowChanged(int currentRow);
 void on_pushButton_clicked();
 void on_sld_z_near_sliderMoved(int position);
	void on_lst_boxes_itemSelectionChanged();
	void on_but_add_clicked();
	void on_chk_boxes_clicked();
	void on_rad_perspective_clicked();
	void on_rad_ortho_clicked();
	void on_sld_z_far_valueChanged(int value);
	void on_sld_z_near_valueChanged(int value);
	void on_grab_threshold_valueChanged(int value);
	void on_refreshVideo();
	void on_calib_changed();
	void send_max_command(QString msg);
};


/*QUdpSocket * udpSocket = new QUdpSocket(this);
udpSocket->bind(QHostAddress("192.168.0.1"), 7755);
QByteArray a;
a.append("toto");
udpSocket->writeDatagram(a,QHostAddress(ui->EdtIp->text()),
													ui->EdtPort->text().toInt());
udpSocket->flush();*/
#endif // MAINWINDOW_H

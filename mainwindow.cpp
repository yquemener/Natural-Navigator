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
    
#include "mainwindow.h"
#include <stdio.h>
#include "ui_mainwindow.h"
#include "math.h"
#include <QTime>
#include "usleep.h"
#include <QNetworkInterface>
#include <cflie/CCrazyflie.h>


const int SIZEX = 8;
const int SIZEY = 4;

inline float sqr(float a){ return a*a;}

float normalized_slider(QSlider * o)
{
	return (float)(o->value() - o->minimum())/(float)(o->maximum()-o->minimum());
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    /*m_crRadio = new CCrazyRadio("radio://0/10/250K");
    if(m_crRadio->startRadio()) {
      m_cflieCopter = new CCrazyflie(m_crRadio);
    }
    else
    {
        m_cflieCopter = 0;
        printf("Could not open Crazyradio\n");
        fflush(stdout);
    }*/
	// initialize the shared scene
	m_pSharedData = new SharedStruct::scene();

    ui->setupUi(this);
    ui->layoutForOpenGL->addWidget(&this->m_gl);
    ui->layoutForOpenGL->addWidget(&this->m_gl_top_view);
        connect(&m_clock, SIGNAL(timeout()), this, SLOT(do_refreshVideo()));
        connect(ui->sld_offset_x, SIGNAL(valueChanged(int)), this, SLOT(do_calib_changed()));
        connect(ui->sld_offset_y, SIGNAL(valueChanged(int)), this, SLOT(do_calib_changed()));
        connect(ui->sld_scale_x, SIGNAL(valueChanged(int)), this, SLOT(do_calib_changed()));
        connect(ui->sld_scale_y, SIGNAL(valueChanged(int)), this, SLOT(do_calib_changed()));
		connect(ui->sld_z_far, SIGNAL(valueChanged(int)), this, SLOT(on_sld_z_far_valueChanged(int)));
		connect(ui->sld_z_near, SIGNAL(valueChanged(int)), this, SLOT(on_sld_z_near_valueChanged(int)));
		//connect(m_gl, SIGNAL(mouse))
    m_clock.start(10.2);
    m_timer.start();

    m_trajfile.setFileName("trajectories.log");
    m_trajfile.open(QIODevice::WriteOnly | QIODevice::Text);
    m_trajstream.setDevice(&m_trajfile);
		// Load settings

    m_settings = new QSettings("IV-devs", "NaturalNavigator");
    if(m_settings->contains("calib_offset_x"))
    {
      ui->sld_offset_x->setValue(m_settings->value("calib_offset_x").toInt());
      ui->sld_offset_y->setValue(m_settings->value("calib_offset_y").toInt());
      ui->sld_scale_x->setValue(m_settings->value("calib_scale_x").toInt());
      ui->sld_scale_y->setValue(m_settings->value("calib_scale_y").toInt());
      do_calib_changed();
      ui->sld_z_far->setValue(m_settings->value("calib_far_z").toInt());
      ui->sld_z_near->setValue(m_settings->value("calib_near_z").toInt());
      int size=m_settings->beginReadArray("boxes");
      for(int i=0;i<size;i++)
      {
        m_settings->setArrayIndex(i);
        float x = m_settings->value("x").toFloat();
        float y = m_settings->value("y").toFloat();
        float z = m_settings->value("z").toFloat();
        float width = m_settings->value("width").toFloat();
        float height = m_settings->value("height").toFloat();
        float depth = m_settings->value("depth").toFloat();
        int behav = m_settings->value("behavior").toInt();
        int udp_code = m_settings->value("udp_code").toInt();
        SharedStruct::box b;
        b.X1=x;
        b.Y1=y;
        b.Z1=z;
        b.X2=x+width;
        b.Y2=y+height;
        b.Z2=z+depth;
        b.behavior = SharedStruct::behavior_t(behav);
        b.state = 0;
        b.last_state = 0;
        b.udp_code = udp_code;
        //b.behavior = SharedStruct::HORIZONTAL_SLIDER;
        m_pSharedData->user_boxes.push_back(b);
      }
      m_settings->endArray();
    }
    SharedStruct::box b;
    b.behavior = SharedStruct::SIMPLE_BOX;
    m_pSharedData->nav_boxes.push_back(b);
    m_pSharedData->nav_boxes.push_back(b);
    m_pSharedData->nav_boxes.push_back(b);
    m_pSharedData->nav_boxes.push_back(b);
    m_pSharedData->nav_boxes.push_back(b);
    m_pSharedData->nav_boxes.push_back(b);
    m_pSharedData->nav_boxes.push_back(b);

    m_proc.set_shared_data(m_pSharedData);
    m_gl.set_shared_data(m_pSharedData);
    m_gl.m_dots_visible=false;
    m_gl.m_proc = &m_proc;

    m_gl_top_view.m_proc = &m_proc;
    m_gl_top_view.set_shared_data(m_pSharedData);
    m_gl_top_view.m_global_rot_x=-13;
    m_gl_top_view.m_global_rot_y=54;
    m_gl_top_view.m_viewer_distance=-150;
    m_gl_top_view.m_perspective=true;
    m_gl_top_view.m_dots_visible=true;
    m_gl_top_view.m_background_video_type = VIDEO_TYPE_NONE;

    m_z_near=0;
		m_z_far=180;
    on_sld_z_far_valueChanged(ui->sld_z_far->value());
    on_sld_z_near_valueChanged(ui->sld_z_near->value());
    on_sld_blobsize_valueChanged(ui->sld_blobsize->value());
    on_but_view_reset_3_clicked();
}



MainWindow::~MainWindow()
{
    m_settings->setValue("calib_offset_x", ui->sld_offset_x->value());
    m_settings->setValue("calib_offset_y", ui->sld_offset_y->value());
    m_settings->setValue("calib_scale_x", ui->sld_scale_x->value());
    m_settings->setValue("calib_scale_y", ui->sld_scale_y->value());
    m_settings->setValue("calib_far_z", ui->sld_z_far->value());
    m_settings->setValue("calib_near_z", ui->sld_z_near->value());

    m_settings->beginWriteArray("boxes");
    for(int i=0;i<m_pSharedData->user_boxes.size();i++)
    {
        m_settings->setArrayIndex(i);
        SharedStruct::box b = m_pSharedData->user_boxes[i];
        m_settings->setValue("x", QString::number(b.X1));
        m_settings->setValue("y", QString::number(b.Y1));
        m_settings->setValue("z", QString::number(b.Z1));
        m_settings->setValue("width", QString::number(b.X2-b.X1));
        m_settings->setValue("height", QString::number(b.Y2-b.Y1));
        m_settings->setValue("depth", QString::number(b.Z2-b.Z1));
        m_settings->setValue("behavior", QString::number(b.behavior));
        m_settings->setValue("udp_code", QString::number(b.udp_code));
    }
    m_settings->endArray();

    m_settings->sync();
    delete ui;
    //delete m_cflieCopter;
    delete m_crRadio;
}

void MainWindow::do_refreshVideo()
{
    int i;
    static int video_count = 0;
    video_count++;
	// Polls different settings

    /*if(m_cflieCopter)
    {


        for(int i=0;i<1;i++)
            m_cflieCopter->cycle();
        ui->log_cflie->append("Cycled " + QString::number(video_count)+"\n");
        if(m_cflieCopter->isInitialized())
        {
            ui->log_cflie->append("Initialized\n");
            m_cflieCopter->setThrust(40000+video_count);
            m_cflieCopter->setSendSetpoints(true);
        }
        else
        {
            ui->log_cflie->append("Not initialized\n");
        }
    }*/

	m_gl.m_background_video_type = VIDEO_TYPE_NONE;
	if(ui->rad_depth->isChecked())
		m_gl.m_background_video_type = VIDEO_TYPE_DEPTH;
	if(ui->rad_rgb->isChecked())
		m_gl.m_background_video_type = VIDEO_TYPE_RGB;
	if(ui->rad_blob->isChecked())
		m_gl.m_background_video_type = VIDEO_TYPE_DEBUG;
	if(ui->rad_none->isChecked())
		m_gl.m_background_video_type = VIDEO_TYPE_NONE;

    m_gl_top_view.m_dots_visible = ui->chk_dots_visible->isChecked();
    m_proc.m_points_rgb = ui->chk_dots_color->isChecked();
    m_gl.m_boxes_visible = ui->chk_boxes->isChecked();
    m_gl_top_view.m_boxes_visible = ui->chk_boxes->isChecked();

    // Processing

    std::vector<blob> bres;
    if (m_proc.update()!=0) return;
    m_proc.process_boxes(m_pSharedData->user_boxes, true);

    for(i=0;i<m_pSharedData->user_boxes.size();i++)
    {
        if(m_pSharedData->user_boxes[i].state !=
           m_pSharedData->user_boxes[i].last_state)
        {
            QString msg=QString::number(m_pSharedData->user_boxes[i].udp_code);
            msg+=" ";
            if(m_pSharedData->user_boxes[i].state!=0)
                msg+=QString::number(m_pSharedData->user_boxes[i].state);
            else
                msg+="0";
            //send_max_command(msg);
            m_pSharedData->user_boxes[i].last_state = m_pSharedData->user_boxes[i].state;
        }
    }


    blob b = m_proc.process_user_volume(m_z_near, m_z_far, 0, 640, 0, 480, ui->but_lock_boxes->isChecked(), m_maxblobsize);
    if((m_pSharedData->detection_user.state!=0)&&(ui->but_record_trajectory->isChecked()))
    {
        float cx = (m_pSharedData->detection_user.X1 + m_pSharedData->detection_user.X2)/2.0;
        float cy = (m_pSharedData->detection_user.Y1 + m_pSharedData->detection_user.Y2)/2.0;
        float cz = (m_pSharedData->detection_user.Z1 + m_pSharedData->detection_user.Z2)/2.0;
        float sx = fabs(m_pSharedData->detection_user.X1 - m_pSharedData->detection_user.X2);
        float sy = fabs(m_pSharedData->detection_user.Y1 - m_pSharedData->detection_user.Y2);
        float sz = fabs(m_pSharedData->detection_user.Z1 - m_pSharedData->detection_user.Z2);
        m_pSharedData->trajectory.push_back(SharedStruct::P3D(cx,cy,cz));
        /*m_trajstream << cx << "\t";
        m_trajstream << cy << "\t";
        m_trajstream << cz << "\t";
        m_trajstream << sx << "\t";
        m_trajstream << sy << "\t";
        m_trajstream << sz << "\n";*/
        QString s = QString::number(cx) + "\t"
                  + QString::number(cy) + "\t"
                  + QString::number(cz) + "\t";
        s += QString::number(sx)+"\t"+
             QString::number(sy)+"\t"+
             QString::number(sz)+"\n";
        cout << s.toStdString();
        cout.flush();
        ui->log_cflie->append(s + "\n");
    }
    m_gl.m_blobs.clear();
    m_gl.m_blobs.push_back(b.tip_x);
    m_gl.m_blobs.push_back(b.tip_y);
    m_gl.m_blobs.push_back(b.x1);
    m_gl.m_blobs.push_back(b.y1);
    m_gl.m_blobs.push_back(b.x2);
    m_gl.m_blobs.push_back(b.y2);

	QString s = QString("Biggest blob :\n")+
							"CX :"+QString::number(b.cx)+"\n"+
							"CY :"+QString::number(b.cy)+"\n"+
							"area :"+QString::number(b.area)+"\n"+
							"perimeter :"+QString::number(b.perimeter)+"\n"+
							"grab measure :"+QString::number((b.area/sqr(b.perimeter)))+"\n";

  //qDebug("%d %f %f %d", bres.size(), bres[bres.size()-1].cx, bres[bres.size()-1].cy, bres[bres.size()-1].x1);

/*  for(i=0;i<bres.size();i++)
    {
        m_gl.m_blobs.push_back(bres[i].cx);
        m_gl.m_blobs.push_back(bres[i].cy);
        m_gl.m_blobs.push_back(bres[i].x1);
        m_gl.m_blobs.push_back(bres[i].y1);
        m_gl.m_blobs.push_back(bres[i].x2);
        m_gl.m_blobs.push_back(bres[i].y2);
    }*/
    if(!ui->but_deactivate_display->isChecked())
    {
        m_gl.makeCurrent();
        m_gl.loadVideoTexture(m_proc.get_data().rgb_data);
        m_gl.loadDebugTexture(m_proc.get_data().dbg_data);
        m_gl.loadDepthTexture(m_proc.get_data().depth_data);
        m_gl.repaint();

        m_gl_top_view.makeCurrent();
        m_gl_top_view.repaint();
    }
    if(m_timer_learn.isActive())
    {
        m_proc.set_background_depth(m_z_near);
        ui->log_cflie->append("Learning");
    }
    if(video_count == 100)
    {
        m_fps = video_count/(m_timer.elapsed()/1000.0);
        video_count = 0;
        m_timer.start();
        //qDebug("fps = %f\n", m_fps);
    }
}

void MainWindow::do_calib_changed()
{
	float sx = normalized_slider(ui->sld_scale_x);
	float sy = normalized_slider(ui->sld_scale_y);
	float ox = normalized_slider(ui->sld_offset_x);
	float oy = normalized_slider(ui->sld_offset_y);

	sx = sx*2.0;
	sy = sy*2.0;
	ox = ox*0.2 - 0.1;
	oy = oy*0.2 - 0.1;

	ui->lbl_offset_x->setText(QString("Offset X : ") + QString::number(ox));
	ui->lbl_offset_y->setText(QString("Offset Y : ") + QString::number(oy));
	ui->lbl_scale_x->setText(QString("Scale X : ") + QString::number(sx));
	ui->lbl_scale_y->setText(QString("Scale Y : ") + QString::number(sy));


  m_proc.set_calib(ox,oy,sx,sy);
}

void MainWindow::on_sld_z_near_valueChanged(int value)
{
		float v = normalized_slider(ui->sld_z_near);
    v=v*1024;
		ui->lbl_z_near->setText(QString("Z near : ") + QString::number(v));
		m_settings->setValue("calib_near_z", ui->sld_z_near->value());
    m_z_near=ui->sld_z_near->value();
		m_z_far=ui->sld_z_far->value() - ui->sld_z_near->value();
}

void MainWindow::on_sld_z_far_valueChanged(int value)
{
	float v = normalized_slider(ui->sld_z_far);
  v=v*1024;
	ui->lbl_z_far->setText(QString("Z far : ") + QString::number(v));
	m_settings->setValue("calib_far_z",ui->sld_z_far->value());
	m_z_near=ui->sld_z_near->value();
	m_z_far=ui->sld_z_far->value() - ui->sld_z_near->value();
}


void MainWindow::on_chk_boxes_clicked()
{
		m_gl.m_boxes_visible = ui->chk_boxes->isChecked();
}

void MainWindow::on_lst_boxes_itemSelectionChanged()
{

}

void MainWindow::on_sld_z_near_sliderMoved(int position)
{

}


void MainWindow::on_but_deactivate_display_clicked()
{

}


void MainWindow::on_ui_visible_clicked()
{
	if(ui->ui_visible->isChecked())
	{
		ui->tabs_tools->show();
	}
	else
	{
		ui->tabs_tools->hide();
	}
}

void MainWindow::on_but_background_depth_clicked()
{
    //on_but_reset_background_depth_clicked();
    m_timer_learn.start(9000);
    m_timer_learn.setSingleShot(true);
    ui->log_cflie->append(QString("Start Learning"));
    //m_proc.set_background_depth(m_z_near);
}


void MainWindow::on_but_view_reset_clicked()
{
  m_gl_top_view.m_global_rot_x=-13;
  m_gl_top_view.m_global_rot_y=54;
  m_gl_top_view.m_global_rot_z = 0.0;
  m_gl_top_view.m_look_at_x = 320;
  m_gl_top_view.m_look_at_y = 240;
  m_gl_top_view.m_look_at_z = 500;
  m_gl_top_view.m_perspective=true;
  m_gl_top_view.m_boxes_visible = true;
  m_gl_top_view.m_dots_visible=true;
  m_gl_top_view.m_background_video_type = VIDEO_TYPE_NONE;
  m_gl_top_view.m_viewer_distance=-100;

  m_gl.m_global_rot_x = 0.0;
  m_gl.m_global_rot_y = 0.0;
  m_gl.m_global_rot_z = 0.0;
  m_gl.m_look_at_x = 320;
  m_gl.m_look_at_y = 240;
  m_gl.m_look_at_z = 500;
  m_gl.m_perspective = false;
  m_gl.m_boxes_visible = false;
  m_gl.m_blobs_visible = true;
  m_gl.m_viewer_distance=0.0;

}

void MainWindow::on_but_reset_background_depth_clicked()
{
  m_proc.clear_background_depth();
}

void MainWindow::on_but_view_plongeante_clicked()
{
  m_gl_top_view.m_global_rot_x=-13;
  m_gl_top_view.m_global_rot_y=54;
  m_gl_top_view.m_global_rot_z = 0.0;
  m_gl_top_view.m_look_at_x = 320;
  m_gl_top_view.m_look_at_y = 240;
  m_gl_top_view.m_look_at_z = 500;
  m_gl_top_view.m_perspective=true;
  m_gl_top_view.m_boxes_visible = true;
  m_gl_top_view.m_dots_visible=true;
  m_gl_top_view.m_background_video_type = VIDEO_TYPE_NONE;
  m_gl_top_view.m_viewer_distance=-100;
}

void MainWindow::on_but_view_reset_3_clicked()
{
  m_gl_top_view.m_global_rot_x=0;
  m_gl_top_view.m_global_rot_y=0;
  m_gl_top_view.m_global_rot_z = 0.0;
  m_gl_top_view.m_look_at_x = 320;
  m_gl_top_view.m_look_at_y = 240;
  m_gl_top_view.m_look_at_z = 500;
  m_gl_top_view.m_perspective=true;
  m_gl_top_view.m_boxes_visible = true;
  m_gl_top_view.m_dots_visible=true;
  m_gl_top_view.m_background_video_type = VIDEO_TYPE_NONE;
  m_gl_top_view.m_viewer_distance=-712;
}

void MainWindow::on_but_view_reset_4_clicked()
{
  m_gl_top_view.m_global_rot_x=-90;
  m_gl_top_view.m_global_rot_y=0;
  m_gl_top_view.m_global_rot_z = 0.0;
  m_gl_top_view.m_look_at_x = 320;
  m_gl_top_view.m_look_at_y = 240;
  m_gl_top_view.m_look_at_z = 500;
  m_gl_top_view.m_perspective=true;
  m_gl_top_view.m_boxes_visible = true;
  m_gl_top_view.m_dots_visible=true;
  m_gl_top_view.m_background_video_type = VIDEO_TYPE_NONE;
  m_gl_top_view.m_viewer_distance=-232;
}

void MainWindow::on_but_reset_boxes_clicked()
{
  m_pSharedData->detection_user_max.X1=std::numeric_limits<float>::max();
  m_pSharedData->detection_user_max.Y1=std::numeric_limits<float>::max();
  m_pSharedData->detection_user_max.X2=std::numeric_limits<float>::min();
  m_pSharedData->detection_user_max.Y2=std::numeric_limits<float>::min();
  m_pSharedData->detection_user_max.Z1=std::numeric_limits<float>::max();
  m_pSharedData->detection_user_max.Z2=std::numeric_limits<float>::min();
}

void MainWindow::on_but_record_trajectory_toggled(bool checked)
{
    if(checked)
    {
        m_trajfile.open(QIODevice::WriteOnly | QIODevice::Text);
    }
    else
    {
        m_trajfile.close();
    }
}

void MainWindow::on_but_cflie_stop_clicked()
{
    ui->but_record_trajectory->setChecked(false);
    cout << "STOP" << endl;
}

void MainWindow::on_but_cflie_start1_clicked()
{
    ui->but_record_trajectory->setChecked(true);
    m_pSharedData->trajectory.clear();
    cout << "START" << endl;
}

void MainWindow::on_sld_blobsize_valueChanged(int value)
{
    ui->lbl_blobsize->setText("Blob size: " + QString::number(value));
    m_maxblobsize=value;
}

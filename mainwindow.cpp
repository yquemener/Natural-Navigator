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
#include "ui_mainwindow.h"
#include "math.h"
#include <QTime>
#include "usleep.h"
#include <QNetworkInterface>

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

	m_udpSocket = new QUdpSocket(this);
  m_destAddress.setAddress("127.0.0.1");
  m_UdpPort = 7474;
  //m_udpSocket->bind(QHostAddress("127.0.0.1"), 7475);


	// initialize the shared scene
	m_pSharedData = new SharedStruct::scene();

    ui->setupUi(this);
    ui->layoutForOpenGL->addWidget(&this->m_gl);
    ui->layoutForOpenGL->addWidget(&this->m_gl_top_view);
		connect(&m_clock, SIGNAL(timeout()), this, SLOT(on_refreshVideo()));
		connect(ui->sld_offset_x, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_offset_y, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_scale_x, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_scale_y, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_z_far, SIGNAL(valueChanged(int)), this, SLOT(on_sld_z_far_valueChanged(int)));
		connect(ui->sld_z_near, SIGNAL(valueChanged(int)), this, SLOT(on_sld_z_near_valueChanged(int)));
		//connect(m_gl, SIGNAL(mouse))
    m_clock.start(10.2);

		// Load settings

    m_settings = new QSettings("IV-devs", "NaturalNavigator");
    if(m_settings->contains("calib_offset_x"))
    {
      ui->sld_offset_x->setValue(m_settings->value("calib_offset_x").toInt());
      ui->sld_offset_y->setValue(m_settings->value("calib_offset_y").toInt());
      ui->sld_scale_x->setValue(m_settings->value("calib_scale_x").toInt());
      ui->sld_scale_y->setValue(m_settings->value("calib_scale_y").toInt());
      on_calib_changed();
      ui->sld_z_far->setValue(m_settings->value("calib_far_z").toInt());
      ui->sld_z_near->setValue(m_settings->value("calib_near_z").toInt());
      ui->edt_ipadr->setText(m_settings->value("ip_address").toString());
      ui->edt_port->setText(m_settings->value("udp_port").toString());
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
        ui->lst_boxes->addItem(QString("Box : (")	+
                               QString::number(x)+","+
                               QString::number(y)+","+
                               QString::number(z)+","+
                               QString::number(width)+","+
                               QString::number(height)+","+
                               QString::number(depth)+", code:"+
                               QString::number(udp_code)+")");
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
    m_gl_top_view.m_viewer_distance=-100;
    m_gl_top_view.m_perspective=true;
    m_gl_top_view.m_dots_visible=true;
    m_gl_top_view.m_background_video_type = VIDEO_TYPE_NONE;

    m_z_near=0;
		m_z_far=180;
    on_sld_z_far_valueChanged(ui->sld_z_far->value());
    on_sld_z_near_valueChanged(ui->sld_z_near->value());
    on_edt_ipadr_editingFinished();
    on_edt_port_editingFinished();
}



MainWindow::~MainWindow()
{
	m_settings->setValue("calib_offset_x", ui->sld_offset_x->value());
	m_settings->setValue("calib_offset_y", ui->sld_offset_y->value());
  m_settings->setValue("calib_scale_x", ui->sld_scale_x->value());
  m_settings->setValue("calib_scale_y", ui->sld_scale_y->value());
  m_settings->setValue("calib_far_z", ui->sld_z_far->value());
  m_settings->setValue("calib_near_z", ui->sld_z_near->value());
  m_settings->setValue("ip_address", ui->edt_ipadr->text());
  m_settings->setValue("udp_port", ui->edt_port->text());

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
}

void MainWindow::on_refreshVideo()
{
  int i;
	// Polls different settings

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

  QTime t = QTime::currentTime();
  std::vector<blob> bres;
  m_proc.update();
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
      //send_max_command(+" "+val);
      send_max_command(msg);
      m_pSharedData->user_boxes[i].last_state = m_pSharedData->user_boxes[i].state;
    }
  }


  //blob b = m_gl.m_proc.process_grab_area(m_z_near, m_z_far, 250, 370, 190, 290);
  //blob b = m_gl.m_proc.process_grab_area(m_z_near, m_z_far, 0,640,0,480);
  //blob b;
  blob b = m_proc.process_user_volume(m_z_near, m_z_far, 0, 640, 0, 480, ui->but_lock_boxes->isChecked());
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


  if(!ui->but_lock_boxes->isChecked())
  {
  /** Make 5 navigator boxes accordingly :
    * 0 : go forward, according to the tip's direction
    * 1,2,3,4 : respectively strafe left, right, up and down
    */
    SharedStruct::box forwardb;
    m_pSharedData->nav_boxes[0].behavior = SharedStruct::SIMPLE_BOX;
    float dX =
        m_pSharedData->detection_user_max.X2-m_pSharedData->detection_user_max.X1;
    float dY =
        m_pSharedData->detection_user_max.Y2-m_pSharedData->detection_user_max.Y1;
    float dZ =
        m_pSharedData->detection_user_max.Z2 - m_pSharedData->detection_user_max.Z1;
    m_pSharedData->nav_boxes[0].Z2 =
         m_pSharedData->detection_user_max.Z2 - 0.66*dZ;
    m_pSharedData->nav_boxes[0].Z1 =
         m_pSharedData->detection_user_max.Z2 - 0.83*dZ;
    m_pSharedData->nav_boxes[0].X1 = dX*0.16+m_pSharedData->detection_user_max.X1;
    m_pSharedData->nav_boxes[0].X2 = dX*0.84+m_pSharedData->detection_user_max.X1;
    m_pSharedData->nav_boxes[0].Y1 = dY*0.38+m_pSharedData->detection_user_max.Y1;
    m_pSharedData->nav_boxes[0].Y2 = dY*0.84+m_pSharedData->detection_user_max.Y1;
    forwardb = m_pSharedData->nav_boxes[0];

    SharedStruct::box newb; // Left strafe
    newb.behavior = SharedStruct::SIMPLE_BOX;
    newb.Z1 = forwardb.Z2;
    newb.Z2 = m_pSharedData->detection_user_max.Z2;
    newb.X1 = m_pSharedData->detection_user_max.X1;
    newb.X2 = forwardb.X1;
    newb.Y1 = m_pSharedData->detection_user_max.Y1;
    newb.Y2 = m_pSharedData->detection_user_max.Y2;
    newb.xs = m_pSharedData->nav_boxes[1].xs;
    newb.ys = m_pSharedData->nav_boxes[1].ys;
    newb.zs = m_pSharedData->nav_boxes[1].zs;
    newb.last_state = m_pSharedData->nav_boxes[1].last_state;
    m_pSharedData->nav_boxes[1] = newb;
    // Right strafe
    newb.X1 = forwardb.X2;
    newb.X2 = m_pSharedData->detection_user_max.X2;
    newb.xs = m_pSharedData->nav_boxes[2].xs;
    newb.ys = m_pSharedData->nav_boxes[2].ys;
    newb.zs = m_pSharedData->nav_boxes[2].zs;
    newb.last_state = m_pSharedData->nav_boxes[2].last_state;
    m_pSharedData->nav_boxes[2] = newb;
    // up strafe
    newb.X1 = m_pSharedData->detection_user_max.X1;
    newb.X2 = m_pSharedData->detection_user_max.X2;
    newb.Y1 = m_pSharedData->detection_user_max.Y1;
    newb.Y2 = dY*0.16+m_pSharedData->detection_user_max.Y1;;
    newb.xs = m_pSharedData->nav_boxes[3].xs;
    newb.ys = m_pSharedData->nav_boxes[3].ys;
    newb.zs = m_pSharedData->nav_boxes[3].zs;
    newb.last_state = m_pSharedData->nav_boxes[3].last_state;
    m_pSharedData->nav_boxes[3] = newb;
    // down strafe
    newb.Z1 = m_pSharedData->detection_user_max.Z1;
    newb.Z2 = forwardb.Z2;
    newb.Y1 = forwardb.Y2;
    newb.Y2 = m_pSharedData->detection_user_max.Y2;
    newb.xs = m_pSharedData->nav_boxes[4].xs;
    newb.ys = m_pSharedData->nav_boxes[4].ys;
    newb.zs = m_pSharedData->nav_boxes[4].zs;
    newb.last_state = m_pSharedData->nav_boxes[4].last_state;
    m_pSharedData->nav_boxes[4] = newb;
    // forward
    forwardb.Z2=forwardb.Z1;
    forwardb.Z1=m_pSharedData->detection_user_max.Z1;
    forwardb.last_state = m_pSharedData->nav_boxes[5].last_state;
    m_pSharedData->nav_boxes[5] = forwardb;
    //backward
    forwardb.X1 = m_pSharedData->detection_user_max.X1;
    forwardb.Y1 = m_pSharedData->detection_user_max.Y1;
    forwardb.X2 = m_pSharedData->detection_user_max.X2;
    forwardb.Y2 = m_pSharedData->detection_user_max.Y2;
    forwardb.Z2 = m_pSharedData->detection_user_max.Z2;
    forwardb.Z1 = m_pSharedData->detection_user_max.Z2-0.12*dZ;
    forwardb.last_state = m_pSharedData->nav_boxes[6].last_state;
    m_pSharedData->nav_boxes[6] = forwardb;

  }

  m_proc.process_boxes(m_pSharedData->nav_boxes, false);
	// Copying ZCamProcessing results into the GLWidget
  //m_gl.m_blobs.clear();
  for(i=0;i<bres.size();i++)
	{
		m_gl.m_blobs.push_back(bres[i].cx);
		m_gl.m_blobs.push_back(bres[i].cy);
		m_gl.m_blobs.push_back(bres[i].x1);
		m_gl.m_blobs.push_back(bres[i].y1);
		m_gl.m_blobs.push_back(bres[i].x2);
		m_gl.m_blobs.push_back(bres[i].y2);
		//qDebug("%f %f", bres[i].cx, bres[i].cy);
	}
	if(!ui->but_deactivate_display->isChecked())
	{
    m_gl.makeCurrent();
    m_gl.loadVideoTexture(m_proc.get_data().rgb_data);
    m_gl.loadDebugTexture(m_proc.get_data().dbg_data);
    m_gl.loadDepthTexture(m_proc.get_data().depth_data);
    m_gl.repaint();

    m_gl_top_view.makeCurrent();
    /*m_gl_top_view.loadVideoTexture(m_proc.get_data().rgb_data);
    m_gl_top_view.loadDebugTexture(m_proc.get_data().dbg_data);
    m_gl_top_view.loadDepthTexture(m_proc.get_data().depth_data);*/
    m_gl_top_view.repaint();
	}

  // Sends UDP messages

  // roll
  bool rolling=false;
  {
    SharedStruct::box b1,b2;
    b1 = m_pSharedData->nav_boxes[1];
    b2 = m_pSharedData->nav_boxes[2];
    if((b1.state!=0)&&(b2.state!=0))
    {
      float dx = abs(b1.xs-b2.xs);
      float dy = (b1.ys - b2.ys); // invert here to invert the value of the roll
      float alpha = atan2(dy,dx);
      if(alpha==alpha)            // checking for NaN
      {
        QString cmd;
        if(alpha>0)
          cmd = "100 ";
        else
          cmd = "97 ";
        if(!rolling)
        {
          this->send_max_command("44 0");
          this->send_max_command("46 0");
        }
        rolling=true;
        cmd+=QString::number(fabs(alpha*2.0/M_PI), 'f',2);
        send_max_command(cmd);
      }
    }
    if((b1.state==0)&&(b2.state==0))
    {
      rolling=false;
      if((b1.last_state!=0)||(b2.last_state!=0))
      {
        this->send_max_command("100 0");
        this->send_max_command("97 0");
      }
    }
  }

  // strafe left
  if(m_pSharedData->nav_boxes[1].state!=m_pSharedData->nav_boxes[1].last_state)
  {
    QString v;
    if((m_pSharedData->nav_boxes[1].state!=0)&&(!rolling))
      v="1";
    else
      v="0";
    this->send_max_command("44 "+v);
  }
  //strafe right
  if(m_pSharedData->nav_boxes[2].state!=m_pSharedData->nav_boxes[2].last_state)
  {
    QString v;
    if((m_pSharedData->nav_boxes[2].state!=0)&&(!rolling))
      v="1";
    else
      v="0";
    this->send_max_command("46 "+v);
  }
  //strafe up
  if(m_pSharedData->nav_boxes[3].state!=m_pSharedData->nav_boxes[3].last_state)
  {
    QString v;
    if(m_pSharedData->nav_boxes[3].state!=0)
      v="1";
    else
      v="0";
    this->send_max_command("39 "+v);
  }
  //strafe down
  if(m_pSharedData->nav_boxes[4].state!=m_pSharedData->nav_boxes[4].last_state)
  {
    QString v;
    if(m_pSharedData->nav_boxes[4].state!=0)
      v="1";
    else
      v="0";
    this->send_max_command("47 "+v);
  }
  //forward
  if(m_pSharedData->nav_boxes[5].state!=m_pSharedData->nav_boxes[5].last_state)
  {
    QString v;
    if(m_pSharedData->nav_boxes[5].state!=0)
      v="1";
    else
      v="0";
    this->send_max_command("30 "+v);
  }

  //backward
  if(m_pSharedData->nav_boxes[6].state!=m_pSharedData->nav_boxes[6].last_state)
  {
    QString v;
    if(m_pSharedData->nav_boxes[6].state!=0)
      v="1";
    else
      v="0";
    this->send_max_command("31 "+v);
  }

  for(i=0;i<m_pSharedData->nav_boxes.size();i++)
    m_pSharedData->nav_boxes[i].last_state=m_pSharedData->nav_boxes[i].state;

  //turn
  static bool turn_up=false;
  static bool turn_down=false;
  static bool turn_right=false;
  static bool turn_left=false;
  bool tu = false;
  bool td = false;
  bool tr = false;
  bool tl = false;
  QString xval;
  QString yval;
  if(m_pSharedData->nav_boxes[0].state!=0)
  {
    SharedStruct::box b = m_pSharedData->nav_boxes[0];
    float centerx = (b.X1+b.X2)/2.0;
    float centery = (b.Y1+b.Y2)/2.0;
    float normx = 2.0*fabs((b.xs-centerx)/(b.X1-b.X2));
    float normy = 2.0*fabs((b.ys-centery)/(b.Y1-b.Y2));
    xval = QString::number(normx, 'f', 2);
    yval = QString::number(normy, 'f', 2);
    if(b.xs>centerx) tr=true; else tl=true;
    if(b.ys>centery) tu=true; else td=true;
  }
  if(tu)
    send_max_command("29 "+yval);
  else
  {
    if(tu!=turn_up)
    {
      send_max_command("29 0");
    }
  }

  if(td)
    send_max_command("28 "+yval);
  else
    if(td!=turn_down)
    {
      send_max_command("28 0");
    }

  if(tl)
    send_max_command("120 "+xval);
  else
    if(tl!=turn_left)
    {
      send_max_command("120 0");
    }

  if(tr)
    send_max_command("119 "+xval);
  else
    if(tr!=turn_right)
    {
      send_max_command("119 0");
    }
  turn_right=tr;
  turn_up=tu;
  turn_left=tl;
  turn_down=td;

  // User presence code
  {
    static bool user_was_here=false;
    bool user_is_here;
    if(m_pSharedData->detection_user_max.X1>m_pSharedData->detection_user_max.X2)
      user_is_here=false;
    else
      user_is_here=true;
    if(user_was_here!=user_is_here)
    {
      if(user_is_here)
        send_max_command("40 1");
      else
        send_max_command("40 0");
    }
    user_was_here=user_is_here;
  }

}

void MainWindow::on_calib_changed()
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

void MainWindow::on_rad_ortho_clicked()
{
		m_gl.m_perspective = false;
		m_gl.resizeGL(m_gl.size().width(), m_gl.size().height());
}

void MainWindow::on_rad_perspective_clicked()
{
		m_gl.m_perspective = true;
		m_gl.resizeGL(m_gl.size().width(), m_gl.size().height());
}

void MainWindow::on_chk_boxes_clicked()
{
		m_gl.m_boxes_visible = ui->chk_boxes->isChecked();
}

void MainWindow::on_but_add_clicked()
{
	float x = ui->edt_box_x->text().toFloat();
	float y = ui->edt_box_y->text().toFloat();
	float z = ui->edt_box_z->text().toFloat();
	float w = ui->edt_box_width->text().toFloat();
	float h = ui->edt_box_height->text().toFloat();
	float d = ui->edt_box_depth->text().toFloat();

	//m_gl.add_box(x,y,z,w,h,d);
	SharedStruct::box m;
	m.X1 = x;
	m.X2 = x + w;
	m.Y1 = y;
	m.Y2 = y + h;
	m.Z1 = z;
	m.Z2 = z + d;
	m.state = 0;
  m.behavior = (SharedStruct::behavior_t)ui->cmb_box_type->currentIndex();
  m.udp_code = ui->edt_box_udp_code->text().toInt();
  m_pSharedData->user_boxes.push_back(m);

	ui->lst_boxes->addItem(QString("Box : (")	+
												 QString::number(x)+","+
												 QString::number(y)+","+
												 QString::number(z)+","+
												 QString::number(w)+","+
                         QString::number(h)+","+
                         QString::number(d)+", code:"+
                         QString::number(m.udp_code)+")");
}

void MainWindow::on_lst_boxes_itemSelectionChanged()
{

}

void MainWindow::on_sld_z_near_sliderMoved(int position)
{

}

void MainWindow::on_grab_threshold_valueChanged(int value)
{
}


void MainWindow::on_pushButton_clicked()
{
    if(ui->lst_boxes->selectedItems().count()==0) return;
		int i = ui->lst_boxes->row(ui->lst_boxes->selectedItems()[0]);
		ui->lst_boxes->takeItem(i);
    m_pSharedData->user_boxes.erase(m_pSharedData->user_boxes.begin()+i);
}

void MainWindow::on_lst_boxes_currentRowChanged(int currentRow)
{
  SharedStruct::box m = m_pSharedData->user_boxes[currentRow];

	ui->edt_box_x->setText(QString::number(m.X1));
	ui->edt_box_y->setText(QString::number(m.Y1));
	ui->edt_box_z->setText(QString::number(m.Z1));
  ui->edt_box_width->setText(QString::number(m.X2-m.X1));
	ui->edt_box_height->setText(QString::number(m.Y2-m.Y1));
	ui->edt_box_depth->setText(QString::number(m.Z2-m.Z1));
  ui->edt_box_udp_code->setText(QString::number(m.udp_code));
  ui->cmb_box_type->setCurrentIndex((int)m.behavior);

}

void MainWindow::on_but_deactivate_display_clicked()
{

}

void MainWindow::send_max_command(QString msg)
{
	QByteArray a;
	a.append(msg);
  m_udpSocket->writeDatagram(a, m_destAddress, m_UdpPort);
	m_udpSocket->flush();
}

void MainWindow::on_sld_depth_boxes_actionTriggered(int action)
{

}

void MainWindow::on_sld_depth_boxes_sliderMoved(int position)
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

void MainWindow::on_sld_right_margin_valueChanged(int value)
{
    m_pSharedData->user_boxes[1].X2 = 100-value;
}

void MainWindow::on_sld_left_margin_valueChanged(int value)
{
    m_pSharedData->user_boxes[0].X1 = value;
}

void MainWindow::on_but_background_depth_clicked()
{
  m_proc.set_background_depth(m_z_near);
}

void MainWindow::on_edt_ipadr_editingFinished()
{
  m_destAddress.setAddress(this->ui->edt_ipadr->text());

  quint32 netmask1,netmask2;

  foreach(QNetworkInterface nwi, QNetworkInterface::allInterfaces())
  {
    foreach(QNetworkAddressEntry addr, nwi.addressEntries())
    {
      QString dbg1,dbg2;
      int dbg3 = addr.prefixLength();
      netmask1 = m_destAddress.toIPv4Address() >> (32-addr.prefixLength());
      netmask2 = addr.ip().toIPv4Address() >> (32-addr.prefixLength());
      dbg1 = m_destAddress.toString();
      dbg2 = addr.ip().toString();
      /*if(netmask1 == netmask2)
        m_udpSocket->bind(addr.ip(), 7575);*/

    }
  }
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

void MainWindow::on_edt_port_editingFinished()
{
  m_UdpPort = this->ui->edt_port->text().toInt();
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

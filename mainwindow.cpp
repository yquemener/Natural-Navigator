// (c) 2011 IV-devs, Yves Quemener

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "math.h"
#include <QTime>
#include "usleep.h"


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
  m_udpSocket->bind(QHostAddress("127.0.0.1"), 7475);


	// initialize the shared scene
	m_pSharedData = new SharedStruct::scene();

    ui->setupUi(this);
		ui->layoutForOpenGL->addWidget(&this->m_gl);
		connect(&m_clock, SIGNAL(timeout()), this, SLOT(on_refreshVideo()));
		connect(ui->sld_offset_x, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_offset_y, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_scale_x, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_scale_y, SIGNAL(valueChanged(int)), this, SLOT(on_calib_changed()));
		connect(ui->sld_z_far, SIGNAL(valueChanged(int)), this, SLOT(on_sld_z_far_valueChanged(int)));
		connect(ui->sld_z_near, SIGNAL(valueChanged(int)), this, SLOT(on_sld_z_near_valueChanged(int)));
		connect(ui->sld_grab_threshold, SIGNAL(valueChanged(int)), this, SLOT(on_grab_threshold_valueChanged(int)));
		//connect(m_gl, SIGNAL(mouse))
		m_clock.start(66.3);

		// Load settings

		m_settings = new QSettings("IV-devs", "Virtopia");
		ui->sld_offset_x->setValue(m_settings->value("calib_offset_x").toInt());
		ui->sld_offset_y->setValue(m_settings->value("calib_offset_y").toInt());
		ui->sld_scale_x->setValue(m_settings->value("calib_scale_x").toInt());
		ui->sld_scale_y->setValue(m_settings->value("calib_scale_y").toInt());
		on_calib_changed();
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
			SharedStruct::box b;
			b.X1=x;
			b.Y1=y;
			b.Z1=z;
			b.X2=x+width;
			b.Y2=y+height;
			b.Z2=z+depth;
			m_pSharedData->boxes.push_back(b);
			ui->lst_boxes->addItem(QString("Box : (")	+
														 QString::number(x)+","+
														 QString::number(y)+","+
														 QString::number(z)+","+
														 QString::number(width)+","+
														 QString::number(height)+","+
														 QString::number(depth)+")");
		}
		m_settings->endArray();

		m_gl.m_proc.set_shared_data(m_pSharedData);
		m_gl.set_shared_data(m_pSharedData);

		m_z_near=0;
		m_z_far=180;
}



MainWindow::~MainWindow()
{
	m_settings->setValue("calib_offset_x", ui->sld_offset_x->value());
	m_settings->setValue("calib_offset_y", ui->sld_offset_y->value());
	m_settings->setValue("calib_scale_x", ui->sld_scale_x->value());
	m_settings->setValue("calib_scale_y", ui->sld_scale_y->value());

	m_settings->beginWriteArray("boxes");
	for(int i=0;i<m_pSharedData->boxes.size();i++)
	{
		m_settings->setArrayIndex(i);
		SharedStruct::box b = m_pSharedData->boxes[i];
		m_settings->setValue("x", QString::number(b.X1));
		m_settings->setValue("y", QString::number(b.Y1));
		m_settings->setValue("z", QString::number(b.Z1));
		m_settings->setValue("width", QString::number(b.X2-b.X1));
		m_settings->setValue("height", QString::number(b.Y2-b.Y1));
		m_settings->setValue("depth", QString::number(b.Z2-b.Z1));
	}
	m_settings->endArray();

	m_settings->sync();
	delete ui;
}

void MainWindow::on_refreshVideo()
{
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
	if(ui->rad_ortho->isChecked())

	if(ui->rad_perspective->isChecked())
		m_gl.m_perspective = true;

	m_gl.m_dots_visible = ui->chk_dots_visible->isChecked();
	m_gl.m_proc.m_points_rgb = ui->chk_dots_color->isChecked();
	m_gl.m_boxes_visible = ui->chk_boxes->isChecked();		// AHA

	// Processing

	QTime t = QTime::currentTime();
	std::vector<blob> bres;
	m_gl.m_proc.update();
	m_gl.m_proc.process_boxes();


  //blob b = m_gl.m_proc.process_grab_area(m_z_near, m_z_far, 250, 370, 190, 290);
  blob b = m_gl.m_proc.process_grab_area(m_z_near, m_z_far, 0,640,0,480);
  //blob b;
	QString s = QString("Biggest blob :\n")+
							"CX :"+QString::number(b.cx)+"\n"+
							"CY :"+QString::number(b.cy)+"\n"+
							"area :"+QString::number(b.area)+"\n"+
							"perimeter :"+QString::number(b.perimeter)+"\n"+
							"grab measure :"+QString::number(1000*(b.area/sqr(b.perimeter)))+"\n";

	ui->txt_log->setText(s);
	//qDebug("%d %f %f %d", bres.size(), bres[bres.size()-1].cx, bres[bres.size()-1].cy, bres[bres.size()-1].x1);

	// Copying ZCamProcessing results into the GLWidget
	m_gl.m_blobs.clear();
	for(int i=0;i<bres.size();i++)
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
		m_gl.loadVideoTexture(m_gl.m_proc.get_data().rgb_data);
		m_gl.loadDebugTexture(m_gl.m_proc.get_data().dbg_data);
    //m_gl.loadDepthTexture(m_gl.m_proc.get_data().depth_data);
    m_gl.loadDepthTexture(m_gl.m_proc.get_data().background_depth);
    m_gl.repaint();
	}

	// Test states for space invader commands generation
	if(m_pSharedData->boxes.size()<4) return;
	static int pstate_left=0;
	static int pstate_right=0;
	static int pstate_top=0;
	static int pstate_big=0;
	static bool pstate_hand=false;

	if(m_pSharedData->boxes[0].state!=pstate_left)
	{
		this->send_max_command("123 "+QString::number(m_pSharedData->boxes[0].state));
	}
	if(m_pSharedData->boxes[1].state!=pstate_right)
	{
		this->send_max_command("124 "+QString::number(m_pSharedData->boxes[1].state));
	}
	if(m_pSharedData->boxes[2].state!=pstate_top)
	{
		this->send_max_command("49 "+QString::number(m_pSharedData->boxes[2].state));
	}

	if(m_pSharedData->boxes[3].state!=pstate_big)
	{
		if(m_pSharedData->boxes[3].state==0)
			this->send_max_command("888");
		else
			this->send_max_command("777");
	}

	/*float value = 1000 * b.area/(sqr(b.perimeter));
	if((value<m_gl.m_proc.m_grab_threshold)!=pstate_hand)
	{
		if((value<m_gl.m_proc.m_grab_threshold))
		{
			this->send_max_command("49 1 49 0");
		}
	}*/


	pstate_left = m_pSharedData->boxes[0].state;
	pstate_right = m_pSharedData->boxes[1].state;
	pstate_top = m_pSharedData->boxes[2].state;
	pstate_big = m_pSharedData->boxes[3].state;
	//pstate_hand = (value<m_gl.m_proc.m_grab_threshold);
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


	//m_gl.m_proc.set_calib(ox,oy,sx,sy);
	m_gl.m_proc.set_calib(ox,oy,sx,sy);
}

void MainWindow::on_sld_z_near_valueChanged(int value)
{
		float v = normalized_slider(ui->sld_z_near);
		v=v*256;
		ui->lbl_z_near->setText(QString("Z near : ") + QString::number(v));
		m_settings->setValue("calib_near_z", ui->sld_z_near->value());
		m_z_near=ui->sld_z_near->value();
		m_z_far=ui->sld_z_far->value() - ui->sld_z_near->value();
}

void MainWindow::on_sld_z_far_valueChanged(int value)
{
	float v = normalized_slider(ui->sld_z_far);
	v=v*256;
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
	m_pSharedData->boxes.push_back(m);

	ui->lst_boxes->addItem(QString("Box : (")	+
												 QString::number(x)+","+
												 QString::number(y)+","+
												 QString::number(z)+","+
												 QString::number(w)+","+
												 QString::number(h)+","+
												 QString::number(d)+")");
}

void MainWindow::on_lst_boxes_itemSelectionChanged()
{

}

void MainWindow::on_sld_z_near_sliderMoved(int position)
{

}

void MainWindow::on_grab_threshold_valueChanged(int value)
{
	//float v = value/100.0;
	ui->lbl_grab_threshold->setText(QString("Grab threshold : ") +
																	QString::number(value));
	m_gl.m_proc.m_grab_threshold = value;
}


void MainWindow::on_pushButton_clicked()
{
		if(ui->lst_boxes->selectedItems().count()==0) return;
		int i = ui->lst_boxes->row(ui->lst_boxes->selectedItems()[0]);
		ui->lst_boxes->takeItem(i);
		m_pSharedData->boxes.erase(m_pSharedData->boxes.begin()+i);
}

void MainWindow::on_lst_boxes_currentRowChanged(int currentRow)
{
	SharedStruct::box m = m_pSharedData->boxes[currentRow];

	ui->edt_box_x->setText(QString::number(m.X1));
	ui->edt_box_y->setText(QString::number(m.Y1));
	ui->edt_box_z->setText(QString::number(m.Z1));
	ui->edt_box_width->setText(QString::number(m.X2-m.X1));
	ui->edt_box_height->setText(QString::number(m.Y2-m.Y1));
	ui->edt_box_depth->setText(QString::number(m.Z2-m.Z1));

}

void MainWindow::on_but_deactivate_display_clicked()
{

}

void MainWindow::send_max_command(QString msg)
{
	QByteArray a;
	a.append(msg);
	m_udpSocket->writeDatagram(a, QHostAddress("127.0.0.1"), 7474);
	m_udpSocket->flush();
}

void MainWindow::on_sld_depth_boxes_actionTriggered(int action)
{

}

void MainWindow::on_sld_depth_boxes_sliderMoved(int position)
{
	for(int i=0; i< m_pSharedData->boxes.size(); i++)
	{
	 m_pSharedData->boxes[i].Z2 = position;
	}
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
		m_pSharedData->boxes[1].X2 = 100-value;
}

void MainWindow::on_sld_left_margin_valueChanged(int value)
{
		m_pSharedData->boxes[0].X1 = value;
}

void MainWindow::on_but_background_depth_clicked()
{
  m_gl.m_proc.set_background_depth(m_z_near);
}

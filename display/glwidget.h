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
    

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <vector>
#include <math.h>
#include "../zcam/zcamprocessing.h"

enum enum_video_type {
	VIDEO_TYPE_NONE,
	VIDEO_TYPE_RGB,
	VIDEO_TYPE_DEPTH,
	VIDEO_TYPE_DEBUG
};

class GLWidget : public QGLWidget
{
    Q_OBJECT

protected:
	void initializeGL();
	void paintGL();
	unsigned int m_videoWidth, m_videoHeight;
	SharedStruct::scene * m_shared_scene;
  void loadTexture(GLuint texindex, QImage &img);


public:
    explicit GLWidget(QObject *parent = 0);
		std::vector<GLuint>	m_textures;
		void loadVideoTexture(unsigned char *data);
		void loadDebugTexture(unsigned char *data);
		void loadDepthTexture(short *data);
		void resizeGL(int width, int height);
		void set_shared_data(SharedStruct::scene * sd) {m_shared_scene = sd;}
		void add_box(float x, float y, float z, float w, float h, float d);
		void add_grid(int size_x, int size_y, float zn, float zf, float width, float height);
    void draw_icon(float x1, float x2, float y1, float y2, GLuint tex);
		GLfloat * points;
		GLfloat * points_color;
    ZCamProcessing * m_proc;

		enum_video_type m_background_video_type;
		bool m_dots_visible;
		bool m_perspective;
		bool m_boxes_visible;

		void mouseMoveEvent(QMouseEvent *e);
		void mousePressEvent(QMouseEvent *e);
		void wheelEvent(QWheelEvent *e);
		float m_global_rot_x;
		float m_global_rot_y;
		float m_global_rot_z;
    float m_look_at_x;
    float m_look_at_y;
    float m_look_at_z;
		float m_viewer_distance;

		int m_drag_old_x;
		int m_drag_old_y;

		std::vector<int>	 m_boxes_status;
		std::vector<float> m_boxes;

		std::vector<float> m_blobs;
		bool m_blobs_visible;

		void draw_box(float x1, float x2, float y1, float y2, float z1, float z2,
									bool filled, int color);

signals:

public slots:

};

#endif // GLWIDGET_H

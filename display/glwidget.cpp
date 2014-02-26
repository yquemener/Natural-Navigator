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

#include "glwidget.h"
#include <GL/glu.h>
#include <QMouseEvent>

const GLfloat options_color[10*4] ={
    0.5, 0.5, 0.0, 0.3,
    0.5, 0.0, 0.0, 0.3,
    0.0, 0.5, 0.0, 0.3,
    0.0, 0.0, 0.5, 0.3,
    0.5, 0.5, 0.5, 0.3,
    1.0, 1.0, 0.0, 0.6,
    1.0, 0.0, 0.0, 0.6,
    0.0, 1.0, 0.0, 0.6,
    0.0, 0.0, 1.0, 0.6,
    1.0, 1.0, 1.0, 0.6
};

GLWidget::GLWidget(QObject *parent) :
    QGLWidget()
{
    m_videoWidth=640;
    m_videoHeight=480;
    points = new GLfloat[640*480*3];
    points_color = new GLfloat[640*480*3];
    for(int i=0;i<640*480*3;i++)
        points_color[i]=0.5;
    m_global_rot_x = 0.0;
    m_global_rot_y = 0.0;
    m_global_rot_z = 0.0;
    m_look_at_x = 320;
    m_look_at_y = 240;
    m_look_at_z = 500;
    m_perspective = false;
    m_boxes_visible = false;
    m_blobs_visible = true;
    m_viewer_distance=0.0;
    m_proc=0;
}

void GLWidget::initializeGL()
{
    qglClearColor(QColor::fromCmykF(0.39, 0.39, 0.0, 0.0));
    glEnable( GL_TEXTURE_2D );
    glShadeModel( GL_SMOOTH );
    glClearDepth( 1.0f );
    glEnable( GL_DEPTH_TEST );
    //glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthFunc( GL_LEQUAL );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
    glOrtho(0,100,100,0,-1,1000);
    glMatrixMode( GL_MODELVIEW );
    GLuint tex;
    glGenTextures(1, &tex);
    m_textures.push_back(tex);
    glGenTextures(1, &tex);
    m_textures.push_back(tex);
    glGenTextures(1, &tex);
    m_textures.push_back(tex);
}

void GLWidget::draw_box(float x1, float x2, float y1, float y2, float z1, float z2,
                        bool filled, int color)
{
    float r = options_color[(color%10) * 4];
    float g = options_color[(color%10) * 4 + 1];
    float b = options_color[(color%10) * 4 + 2];
    float a = options_color[(color%10) * 4 + 3];

    if(!filled)
    {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0,0,1.0);
        glBegin(GL_LINE_LOOP);
        glVertex3f(  x1, y1, z1 );
        glVertex3f(  x2, y1, z1 );
        glVertex3f(  x2, y2, z1 );
        glVertex3f(  x1, y2, z1 );
        glEnd();
        glBegin(GL_LINE_LOOP);
        glVertex3f(  x1, y1, z2 );
        glVertex3f(  x2, y1, z2 );
        glVertex3f(  x2, y2, z2 );
        glVertex3f(  x1, y2, z2 );
        glEnd();
        glBegin(GL_LINES);
        glVertex3f(  x1, y1, z1 );
        glVertex3f(  x1, y1, z2 );
        glVertex3f(  x2, y1, z1 );
        glVertex3f(  x2, y1, z2 );
        glVertex3f(  x1, y2, z1 );
        glVertex3f(  x1, y2, z2 );
        glVertex3f(  x2, y2, z1 );
        glVertex3f(  x2, y2, z2 );
        glEnd();
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glColor4f(r,g,b,a);
        glBegin(GL_QUADS);
        glVertex3f(  x1, y1, z1 );
        glVertex3f(  x2, y1, z1 );
        glVertex3f(  x2, y2, z1 );
        glVertex3f(  x1, y2, z1 );

        glVertex3f(  x1, y1, z2 );
        glVertex3f(  x2, y1, z2 );
        glVertex3f(  x2, y2, z2 );
        glVertex3f(  x1, y2, z2 );

        glVertex3f(  x1, y1, z2 );
        glVertex3f(  x2, y1, z2 );
        glVertex3f(  x2, y1, z1 );
        glVertex3f(  x1, y1, z1 );

        glVertex3f(  x1, y2, z2 );
        glVertex3f(  x2, y2, z2 );
        glVertex3f(  x2, y2, z1 );
        glVertex3f(  x1, y2, z1 );

        glVertex3f(  x1, y1, z2 );
        glVertex3f(  x1, y2, z2 );
        glVertex3f(  x1, y2, z1 );
        glVertex3f(  x1, y1, z1 );

        glVertex3f(  x2, y1, z2 );
        glVertex3f(  x2, y2, z2 );
        glVertex3f(  x2, y2, z1 );
        glVertex3f(  x2, y1, z1 );

        glEnd();
    }
}

void GLWidget::draw_icon(float x1, float x2, float y1, float y2, GLuint tex)
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D,tex);
    glBegin(GL_QUADS);
    glTexCoord2d(0,0);
    glVertex3f(  x1, y1, 0 );
    glTexCoord2d(1,0);
    glVertex3f(  x2, y1, 0 );
    glTexCoord2d(1,1);
    glVertex3f(  x2, y2, 0 );
    glTexCoord2d(0,1);
    glVertex3f(  x1, y2, 0 );
    glEnd();
}

void GLWidget::paintGL()
{
    static GLUquadric* stayHere = gluNewQuadric();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity( );

    glTranslatef(0,0,m_viewer_distance);
    glTranslatef(m_look_at_x, m_look_at_y, m_look_at_z);
    glRotatef(m_global_rot_y, 1,0,0);
    glRotatef(m_global_rot_x, 0,1,0);

    glTranslatef(-m_look_at_x,-m_look_at_y,-m_look_at_z);
    glTranslatef(640,0,0);
    //glRotatef(m_global_rot_z, 0,0,1);

    // Draw the video

    glPushMatrix();
    glScalef(-1,1,1);
    if(m_background_video_type!=VIDEO_TYPE_NONE)
    {
        glEnable(GL_TEXTURE_2D);
        if(m_background_video_type==VIDEO_TYPE_RGB)
        {
            glBindTexture( GL_TEXTURE_2D, m_textures[0] );
        }
        if(m_background_video_type==VIDEO_TYPE_DEPTH)
        {
            glBindTexture( GL_TEXTURE_2D, m_textures[1] );
        }
        if(m_background_video_type==VIDEO_TYPE_DEBUG)
        {
            glBindTexture( GL_TEXTURE_2D, m_textures[2] );
        }

        glColor3f(1.0,1.0,1.0);
        glTranslatef(0,0,800);
        glDisable(GL_DEPTH);
        glBegin(GL_QUADS);
        glTexCoord2f( 0.0f, 1.0f ); glVertex3f(  0, 480, 1024.0f );
        glTexCoord2f( 0.0f, 0.0f ); glVertex3f(  0, 0, 1024.0f );
        glTexCoord2f( 1.0f, 0.0f ); glVertex3f(  640, 0, 1024.0f );
        glTexCoord2f( 1.0f, 1.0f ); glVertex3f(  640, 480, 1024.0f );
        glEnd( );
        glEnable(GL_DEPTH);
    }

    glPopMatrix();

    if(m_dots_visible)
    {
        glPushMatrix();
        glScalef(-1,1,1);
        glDisable(GL_TEXTURE_2D);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);
        glScalef(1,1,4);

        glColorPointer(3, GL_FLOAT, 0, m_proc->get_data().dots_3d_colors);
        glVertexPointer(3, GL_FLOAT, 0, m_proc->get_data().dots_3d);
        glDrawArrays(GL_POINTS, 0, 640*480);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glPopMatrix();
    }

    // Draw modules

    if(m_boxes_visible)
    {
        glPushMatrix();
        glTranslatef(-640,0,0);
        glScalef(6.4,4.8,1);
        glScalef(1,1,4);
        glEnable(GL_BLEND);
        int imax = m_shared_scene->user_boxes.size();
        for(int i=0;i<imax;i++)
        {
            SharedStruct::box m = m_shared_scene->user_boxes[i];
            float x1 = m.X1;
            float y1 = m.Y1;
            float z1 = m.Z1;
            float x2 = m.X2;
            float y2 = m.Y2;
            float z2 = m.Z2;
            draw_box(x1,x2,y1,y2,z1,z2,(m.state!=0),1);
        }
        glPopMatrix();
    }

    // Draw trajectory
    {
        glPushMatrix();
        glScalef(-1,1,1);
        for(int i=0;i<m_shared_scene->trajectory.size();i++)
        {
            SharedStruct::P3D p = m_shared_scene->trajectory[i];
            glColor3f(1,0,0);
            glPushMatrix();
            glTranslatef(p.x, p.y, p.z*4);
            gluSphere(stayHere, 5, 20,10);
            glPopMatrix();
        }
        glBegin(GL_LINE_STRIP);
        for(int i=0;i<m_shared_scene->trajectory.size();i++)
        {
            SharedStruct::P3D p = m_shared_scene->trajectory[i];
            glVertex3f(p.x, p.y, p.z*4);
        }
        glEnd();
        glPopMatrix();
    }

    //Draw the user volume
    if(m_boxes_visible)
    {
        glPushMatrix();
        glScalef(-1,1,4);

        SharedStruct::box b=m_shared_scene->detection_user;
        if(b.state!=0)
            draw_box(b.X1,b.X2,b.Y1,b.Y2,b.Z1,b.Z2,1,3);


        glPopMatrix();
    }

}

void GLWidget::resizeGL(int width, int height)
{
    glMatrixMode( GL_PROJECTION );
    if(m_perspective)
    {
        glLoadIdentity();
        gluPerspective(90, 4.0/3.0, 1.0, 10000.0);
        gluLookAt(320,240,-240,   320,240,0,    0,-100,0);
    }
    else
    {
        glLoadIdentity( );
        glOrtho(-320,320,-240,240,-1,10000);
        gluLookAt(320,240,-240,   320,240,0,    0,-100,0);
    }
    glViewport(0,0, width, height);
    glMatrixMode( GL_MODELVIEW );
    //qDebug("%d", this->width());
}

void GLWidget::loadVideoTexture(unsigned char *data)
{
    if (data==NULL) return;

    glBindTexture( GL_TEXTURE_2D, m_textures[0] );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_videoWidth, m_videoHeight,0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
    return;
}

void GLWidget::loadTexture(GLuint texindex, QImage &img)
{
    glBindTexture(GL_TEXTURE_2D, texindex);

    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, 4, img.width(),
                 img.height(), 0, GL_BGRA,
                 GL_UNSIGNED_BYTE, img.bits());
}

void GLWidget::loadDepthTexture(short *data)
{
    if (data==NULL) return;

    glBindTexture( GL_TEXTURE_2D, m_textures[1] );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_videoWidth, m_videoHeight,0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
    return;
}

void GLWidget::loadDebugTexture(unsigned char *data)
{
    if (data==NULL) return;

    glBindTexture( GL_TEXTURE_2D, m_textures[2] );
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_videoWidth, m_videoHeight,0, GL_BGR_EXT, GL_UNSIGNED_BYTE, data);
    return;
}


void GLWidget::mouseMoveEvent(QMouseEvent *e)
{
    m_global_rot_x += (e->x()-m_drag_old_x)/10.0;
    m_global_rot_y += (e->y()-m_drag_old_y)/10.0;
    //printf("rot x=%f\nrot y=%f\n", m_global_rot_x, m_global_rot_y);
    //fflush(stdout);
    m_drag_old_x = e->x();
    m_drag_old_y = e->y();
}

void GLWidget::mousePressEvent(QMouseEvent *e)
{
    m_drag_old_x = e->x();
    m_drag_old_y = e->y();
}

void GLWidget::wheelEvent(QWheelEvent *e)
{
    m_viewer_distance += e->delta()/10.0;
}

void GLWidget::add_box(float x, float y, float z, float w, float h, float d)
{
    m_boxes.push_back(x);
    m_boxes.push_back(y);
    m_boxes.push_back(z);
    m_boxes.push_back(w);
    m_boxes.push_back(h);
    m_boxes.push_back(d);

    m_boxes_status.push_back(0);
}

void GLWidget::add_grid(int size_x, int size_y, float zn, float zf, float width=640, float height=480)
{

    for(int y=0; y<size_y; y++)
    {
        for(int x=0; x<size_x; x++)
        {
            add_box(x*width/size_x, y*height/size_y, zn,
                    width/size_x, height/size_y, zf-zn );
        }
    }
}

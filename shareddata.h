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
    


#ifndef SHAREDDATA_H
#define SHAREDDATA_H

#include <list>
#include <stdio.h>
#include <vector>

namespace SharedStruct
{
    typedef enum{ SIMPLE_BOX, HORIZONTAL_SLIDER, VERTICAL_SLIDER, TOGGLE_BOX }
    behavior_t;

struct P3D_t{
    float x,y,z;

    P3D_t(float a, float b, float c)
    {
        x=a;y=b;z=c;
    }
};

typedef P3D_t P3D;

typedef struct {
	float X1,Y1,Z1,X2,Y2,Z2;
	float R,G,B;
	int state;
  int last_state;
  int xs,ys,zs;
  behavior_t behavior;
  int udp_code;
} box;

typedef struct {
  std::vector<box> user_boxes;
  std::vector<box> nav_boxes;
  box detection_user;
  box detection_user_max;
  float tip_x,tip_y,tip_z;  // Coordinates of the closest point of the user volume
  float head_x,head_y,head_z; // Coordinates of the highest point of the user volume

  std::vector<P3D> trajectory;
} scene;

}
#endif // SHAREDDATA_H

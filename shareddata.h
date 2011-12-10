#ifndef SHAREDDATA_H
#define SHAREDDATA_H

#include <list>
#include <stdio.h>
#include <vector>

namespace SharedStruct
{
    typedef enum{ SIMPLE_BOX, HORIZONTAL_SLIDER, VERTICAL_SLIDER, TOGGLE_BOX }
    behavior_t;

typedef struct {
	float X1,Y1,Z1,X2,Y2,Z2;
	float R,G,B;
	int state;
  int last_state;
  int xs,ys,zs;
  behavior_t behavior;
} box;

typedef struct {
  std::vector<box> user_boxes;
  std::vector<box> nav_boxes;
  box detection_user;
  box detection_user_max;
  float tip_x,tip_y,tip_z;  // Coordinates of the closest point of the user volume
  float head_x,head_y,head_z; // Coordinates of the highest point of the user volume
} scene;

}
#endif // SHAREDDATA_H

#ifndef SHAREDDATA_H
#define SHAREDDATA_H

#include <list>
#include <stdio.h>
#include <vector>

namespace SharedStruct
{

typedef struct {
	float X1,Y1,Z1,X2,Y2,Z2;
	float R,G,B;
	int state;
} box;

typedef struct {
	std::vector<box> boxes;
} scene;

}
#endif // SHAREDDATA_H

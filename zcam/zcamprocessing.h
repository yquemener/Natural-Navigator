// (c) 2011 IV-devs, Yves Quemener

#ifndef ZCAMPROCESS_H
#define ZCAMPROCESS_H

#include <vector>
#include <map>
#include "../shareddata.h"

/*
 * General ideas around this lib :
 * Its goal is to process data coming from zcams.
 * It should keep all the internal data necessary to communicate with the device
 * It also is responsible for the allocation and refresh of the 3D data. It also
 * keeps the calibration parameters necessary to make 3D data and RGB data
 * match.
 *
 * On request, it can use the 3D points in some process but it should never
 * overwrite them. The functions for these processings should be state-less :
 * they should not modify the internal state of ZCamProcessing. Any state should
 * be passed through parameters. It is the caller's duty to keep track of them
 * and to allocate the necessary memory for the output structures.
 *
 */

typedef struct
{
	float * dots_3d;
	float * dots_3d_colors;

	unsigned char		*	rgb_data;
	short						* depth_data;
	int rgb_width;
	int rgb_height;

	unsigned char		*	dbg_data;
  short * background_depth;
} raw_data;

typedef struct {
	int area;
	int perimeter;
	float cx;
	float cy;
	int x1;
	int y1;
	int x2;
	int y2;
} blob;

class ZCamProcessing
{
private:
	raw_data						m_out_data;
  short             * m_background_depth;
	float								m_offset_x, m_offset_y;
	float								m_scale_x, m_scale_y;
	SharedStruct::scene	*	m_shared_scene;


	/**/std::vector<float>	m_boxes;
	float								m_grid_z_far, m_grid_z_near;
	int									m_grid_size_x;
	int									m_grid_size_y;
	float								m_grid_elem_width;
	float								m_grid_elem_height;

public:
    ZCamProcessing();

		void update();

		raw_data& get_data();
		void set_calib(float off_x, float off_y, float scale_x, float scale_y);
		bool m_points_rgb;

		void process_boxes();
		blob process_grab_area(const float z_near, const float z_far,
														const float x1, const float x2,
														const float y1, const float y2);
		void set_shared_data(SharedStruct::scene * sd) { m_shared_scene = sd; }
    void set_background_depth(float);

		float m_grab_threshold;
};

#endif // ZCAMPROCESS_H


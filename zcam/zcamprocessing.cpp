// (c) 2011 IV-devs, Yves Quemener

#include "zcamprocessing.h"
#include "libfreenect.h"
#include "libfreenect_sync.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits>

bool inline is_in_box(float xb, float yb, float zb,
											float width, float height, float depth,
											float x, float y, float z)
{
	return ((x>xb)&&(x<xb+width)&&
					(y>yb)&&(y<yb+height)&&
					(z>zb)&&(z<zb+depth));
}

void inline clamp(float &a, float min, float max)
{
	if (a>max) a=max;
	if (a<min) a=min;
}

int inline max(int a, int b)
{
  if(a>b)
    return a;
  else
    return b;
}

int inline min(int a, int b)
{
  if(a<b)
    return a;
  else
    return b;
}

int inline max(float a, float b)
{
  if(a>b)
    return a;
  else
    return b;
}

int inline min(float a, float b)
{
  if(a<b)
    return a;
  else
    return b;
}

ZCamProcessing::ZCamProcessing()
{
	m_out_data.rgb_data = 0; //malloc(640*480*3);
	m_out_data.rgb_height = 480;
	m_out_data.rgb_width = 640;

	m_out_data.dots_3d = new float[640*480*3];
	m_out_data.dots_3d_colors = new float[640*480*3];

	m_offset_x = 0.0;
	m_offset_y = 0.0;
	m_scale_x = 1.0;
	m_scale_y = 1.0;

	m_grid_z_far = 80.0;
	m_grid_z_near = 40.0;

	m_points_rgb = false;

	m_out_data.dbg_data = new unsigned char[640*480*3];

	m_grab_threshold = 0.1;

  m_background_depth = 0;
  m_out_data.background_depth=m_background_depth;
  static const char cpyrightrot13[] = "(P) Lirf Dhrzrare";
	static const char cpyright[] = "Virtopia interface\n(C) 2011 Yves Quemener\n";
}

void ZCamProcessing::update()
{
	short *depth = 0;
	char *rgb = 0;
	uint32_t ts;
	int i,j;

	if (freenect_sync_get_video((void**)&(m_out_data.rgb_data), &ts, 0, FREENECT_VIDEO_RGB) < 0)
	{
		return;
	}
	if (freenect_sync_get_depth((void**)&(m_out_data.depth_data), &ts, 0, FREENECT_DEPTH_11BIT) < 0)
	{
		return;
	}

	for(int i=0; i<640*480-3; i++ )
	{
		char c;
		c=m_out_data.rgb_data[i*3];
		m_out_data.rgb_data[i*3] = m_out_data.rgb_data[i*3+2];
		m_out_data.rgb_data[i*3+2]=c;
	}

	for(i=0; i<640*480-3; i++ )
	{
        int v;
        v=m_out_data.depth_data[i];
        if((m_background_depth)&&(v>=m_background_depth[i]))
        {
          v=0;
          m_out_data.depth_data[i]=0;
        }
        v=v*v*v;

//		if((v/4000000.0<m_z_far)&&
//			 (v/4000000.0>m_z_near))

		if(v/4000000.0>0)
		{
			m_out_data.dots_3d[i*3] = ((i%640))*m_scale_x + m_offset_x*640;
			m_out_data.dots_3d[i*3+1] = ((i/640))*m_scale_y + m_offset_y*480;
			m_out_data.dots_3d[i*3+2] = v/4000000.0;
		}
		else
    {
			m_out_data.dots_3d[i*3] = 0;
			m_out_data.dots_3d[i*3+1] = 0;
			m_out_data.dots_3d[i*3+2] = 0;
		}


		if(m_points_rgb)
		{
			int ind = (i%640)*m_scale_x + m_offset_x*640 +
								(int)((i/640)*m_scale_y + m_offset_y*480 ) * 640;
			if((ind>0)&&(ind<640*480))
			{
				m_out_data.dots_3d_colors[i*3] = m_out_data.rgb_data[ind*3+2]/256.0;
				m_out_data.dots_3d_colors[i*3+1] = m_out_data.rgb_data[ind*3+1]/256.0;
				m_out_data.dots_3d_colors[i*3+2] = m_out_data.rgb_data[ind*3]/256.0;
			}
		}
		else
		{
			m_out_data.dots_3d_colors[i*3] = v/256000000.0;
			m_out_data.dots_3d_colors[i*3+1] = v/256000000.0;
			m_out_data.dots_3d_colors[i*3+2] = v/256000000.0;
		}
  }

  // Denoise

  /*for(i=641;i<640*480-641;i++)
  {
    // Denoise by connect-8 erosion

    if((m_out_data.depth_data[i-641]<0)||
       (m_out_data.depth_data[i-640]<0)||
       (m_out_data.depth_data[i-639]<0)||
       (m_out_data.depth_data[i-1]<0)||
       (m_out_data.depth_data[i+1]<0)||
       (m_out_data.depth_data[i+639]<0)||
       (m_out_data.depth_data[i+640]<0)||
       (m_out_data.depth_data[i+641]<0))
    {
      m_out_data.dots_3d[i*3] = 0;
      m_out_data.dots_3d[i*3+1] = 0;
      m_out_data.dots_3d[i*3+2] = 0;
    }
  }*/

}

raw_data& ZCamProcessing::get_data()
{
	return m_out_data;
}

void ZCamProcessing::set_calib(float off_x, float off_y,
															 float scale_x, float scale_y)
{
	m_offset_x = off_x;
	m_offset_y = off_y;
	m_scale_x = scale_x;
	m_scale_y = scale_y;
}



void ZCamProcessing::process_boxes(std::vector<SharedStruct::box> &boxes,
                                   const bool rescale = true)
{

  int imax = boxes.size();
	std::vector<int> result;
	for(int i=0;i<imax;i++)
	{
		result.push_back(0);
    boxes[i].xs=0;
    boxes[i].ys=0;
    boxes[i].zs=0;
    /*xs.push_back(0);
		ys.push_back(0);
    zs.push_back(0);*/
	}

	for(int i=0;i<imax;i++)
	{
    SharedStruct::box m = boxes[i];
    if(rescale)
    {
      m.X1 = ((100-m.X1)*6.4 - m_offset_x*640)/m_scale_x;
      m.Y1 = (m.Y1*4.8 - m_offset_y*480)/m_scale_y;
      m.X2 = ((100-m.X2)*6.4 - m_offset_x*640)/m_scale_x;
      m.Y2 = (m.Y2*4.8 - m_offset_y*480)/m_scale_y;
      float c;
      c=m.X2;
      m.X2=m.X1;
      m.X1=c;
    }
    else
    {
      m.X1 = (m.X1 - m_offset_x*640)/m_scale_x;
      m.Y1 = (m.Y1 - m_offset_y*480)/m_scale_y;
      m.X2 = (m.X2 - m_offset_x*640)/m_scale_x;
      m.Y2 = (m.Y2 - m_offset_y*480)/m_scale_y;
    }

		//printf("after : %f %f %f %f \n", m.X1, m.X2, m.Y1, m.Y2);

		clamp(m.X1,0,640);
		clamp(m.X2,0,640);
		clamp(m.Y1,0,480);
		clamp(m.Y2,0,480);

		for(int y=m.Y1; y<(int)m.Y2; y++)
		{
			int index = y*640+(int)m.X1;
			for(int x=(int)m.X1; x<(int)m.X2; x++)
      {
				if((m_out_data.dots_3d[index*3+2]>m.Z1)&&
					 (m_out_data.dots_3d[index*3+2]<m.Z2))
				{
					result[i]++;
          m.xs+=x;
          m.ys+=y;
          m.zs+=m_out_data.dots_3d[index*3+2];
				}
				index++;
			}
		}
    const int threshold = 50;
    if(result[i]>0)
    {
      m.xs/=result[i];
      m.ys/=result[i];
      m.zs/=result[i];
    }

    switch(m.behavior)
    {
    case(SharedStruct::SIMPLE_BOX):
      if(result[i]>threshold)
      {
        m.state = 1;
      }
      else
      {
        m.state=0;
      }
      break;
    case(SharedStruct::TOGGLE_BOX):
      if(result[i]>threshold)
      {
        if(m.last_state<threshold)
          m.state = 1-m.state;
      }
      m.last_state = result[i];
      break;
    case(SharedStruct::HORIZONTAL_SLIDER):
      if(result[i]>threshold)
      {
        if(rescale)
        {
          float x1 = (m.X1*6.4 - m_offset_x*640)/m_scale_x;
          float x2 = (m.X2*6.4 - m_offset_x*640)/m_scale_x;
          m.state =  (((m.xs)-x1)/(x2-x1)) * 100.0;
        }
        else
        {
          float x1 = (m.X1 - m_offset_x*640)/m_scale_x;
          float x2 = (m.X2 - m_offset_x*640)/m_scale_x;
          m.state =  (((m.xs)-x1)/(x2-x1)) * 100.0;
        }
      }
      break;
    case(SharedStruct::VERTICAL_SLIDER):
      if(result[i]>threshold)
      {
        if(rescale)
        {
          float y1 = (m.Y1*4.8 - m_offset_y*480)/m_scale_y;
          float y2 = (m.Y2*4.8 - m_offset_y*480)/m_scale_y;
          m.state =  (((m.ys)-y1)/(y2-y1)) * 100.0;
        }
        else
        {
          float y1 = (m.Y1 - m_offset_y*480)/m_scale_y;
          float y2 = (m.Y2 - m_offset_y*480)/m_scale_y;
          m.state =  (((m.ys)-y1)/(y2-y1)) * 100.0;
        }
      }
      break;
    }
    boxes[i] = m;
	}
}

blob ZCamProcessing::process_user_volume(const float z_near, const float z_far,
                                       const float x1, const float x2,
                                       const float y1, const float y2)
{
      std::vector<blob> results;
      blob b;
      results.clear();

      process_blobs(z_near,z_far,x1,x2,y1,y2,results);


      b.cx=0;
      b.cy=0;
      b.x1=10000;
      b.x2=0;
      b.y1=10000;
      b.y2=0;
      b.tip_z=std::numeric_limits<int>::max();
      b.z2=std::numeric_limits<int>::min();
      b.area=0;
      b.perimeter=0;

      // Consider all blobs bigger than 1000 pixels to be part of the user
      for(int i=0;i<results.size();i++)
      {
        if(results[i].area>1000)
        {
          b.x1=min(b.x1, results[i].x1);
          if(b.y1 > results[i].y1)
          {
            b.y1 = results[i].y1;
            b.head_x = results[i].head_x;
            b.head_y = results[i].head_y;
            b.head_z = results[i].head_z;
          }
          b.x2=max(b.x2, results[i].x2);
          b.y2=max(b.y2, results[i].y2);
          if(b.tip_z > results[i].tip_z)
          {
            b.tip_z = results[i].tip_z;
            b.tip_y = results[i].tip_y;
            b.tip_x = results[i].tip_x;
          }
          b.z2 = max(b.z2,results[i].z2);
        }
      }

      m_shared_scene->detection_user.X1 = b.x1;
      m_shared_scene->detection_user.X2 = b.x2;
      m_shared_scene->detection_user.Y1 = b.y1;
      m_shared_scene->detection_user.Y2 = b.y2;
      m_shared_scene->detection_user.Z1 = float(b.tip_z)*b.tip_z*b.tip_z/4000000.0;
      m_shared_scene->detection_user.Z2 = float(b.z2)*b.z2*b.z2/4000000.0;

      m_shared_scene->tip_x = b.tip_x;
      m_shared_scene->tip_y = b.tip_y;
      m_shared_scene->tip_z = m_shared_scene->detection_user.Z1;

      m_shared_scene->head_x = b.head_x;
      m_shared_scene->head_y = b.head_y;
      m_shared_scene->head_z = float(b.head_z)*b.head_z*b.head_z/4000000.0;

      m_shared_scene->detection_user.X1 =
          (m_shared_scene->detection_user.X1)*m_scale_x + m_offset_x*640;
      m_shared_scene->detection_user.Y1 =
          (m_shared_scene->detection_user.Y1)*m_scale_y + m_offset_y*480;
      m_shared_scene->detection_user.X2 =
          (m_shared_scene->detection_user.X2)*m_scale_x + m_offset_x*640;
      m_shared_scene->detection_user.Y2 =
          (m_shared_scene->detection_user.Y2)*m_scale_y + m_offset_y*480;

      m_shared_scene->tip_x =
          (m_shared_scene->tip_x)*m_scale_x + m_offset_x*640;
      m_shared_scene->tip_y =
          (m_shared_scene->tip_y)*m_scale_y + m_offset_y*480;

      m_shared_scene->head_x =
          (m_shared_scene->head_x)*m_scale_x + m_offset_x*640;
      m_shared_scene->head_y =
          (m_shared_scene->head_y)*m_scale_y + m_offset_y*480;

      if((m_shared_scene->detection_user.X1 > 1000)||
         (m_shared_scene->detection_user.Y1 > 1000))
      {
        m_shared_scene->detection_user_max.X1=std::numeric_limits<float>::max();
        m_shared_scene->detection_user_max.Y1=std::numeric_limits<float>::max();
        m_shared_scene->detection_user_max.X2=std::numeric_limits<float>::min();
        m_shared_scene->detection_user_max.Y2=std::numeric_limits<float>::min();
        m_shared_scene->detection_user_max.Z1=std::numeric_limits<float>::max();
        m_shared_scene->detection_user_max.Z2=std::numeric_limits<float>::min();
      }
      else
      {
        m_shared_scene->detection_user_max.X1 = min(
              m_shared_scene->detection_user_max.X1,
              m_shared_scene->detection_user.X1);
        m_shared_scene->detection_user_max.X2 = max(
              m_shared_scene->detection_user_max.X2,
              m_shared_scene->detection_user.X2);
        m_shared_scene->detection_user_max.Y1 = min(
              m_shared_scene->detection_user_max.Y1,
              m_shared_scene->detection_user.Y1);
        m_shared_scene->detection_user_max.Y2 = max(
              m_shared_scene->detection_user_max.Y2,
              m_shared_scene->detection_user.Y2);
        m_shared_scene->detection_user_max.Z1 = min(
              m_shared_scene->detection_user_max.Z1,
              m_shared_scene->detection_user.Z1);
        m_shared_scene->detection_user_max.Z2 = max(
              m_shared_scene->detection_user_max.Z2,
              m_shared_scene->detection_user.Z2);
      }

      // Make a histogram of the points in the user volume
      {
        int x,y;
        int vertical_histogram[480];
        float vertical_histogram_x[480];
        float vertical_histogram_z[480];
        int depth_histogram[1024];
        float depth_histogram_x[1024];
        float depth_histogram_y[1024];
        const int nextline=(640-b.x2+b.x1)*3;
        int index = (b.y1*640+b.x1)*3+2;
        memset(vertical_histogram, 0, 480*sizeof(int));
        memset(depth_histogram, 0, 1024*sizeof(int));
        memset(vertical_histogram_x, 0, 480*sizeof(float));
        memset(vertical_histogram_z, 0, 480*sizeof(float));
        memset(depth_histogram_x, 0, 1024*sizeof(float));
        memset(depth_histogram_y, 0, 1024*sizeof(float));
        for(y=b.y1;y<b.y2;y++)
        {
          for(x=b.x1;x<b.x2;x++)
          {
            if((m_out_data.dots_3d[index]<b.z2)||
               (m_out_data.dots_3d[index]>b.tip_z))
            {
              float pz = m_out_data.dots_3d[index];
              float py = m_out_data.dots_3d[index-1];
              float px = m_out_data.dots_3d[index-2];
              clamp(pz,0,1023);
              clamp(py,0,479);
              clamp(px,0,639);
              vertical_histogram[(int)(py)]++;
              depth_histogram[(int)(pz)]++;
              vertical_histogram_x[(int)(py)]+=px;
              vertical_histogram_z[(int)(py)]+=pz;
              depth_histogram_x[(int)(pz)]+=px;
              depth_histogram_y[(int)(pz)]+=py;
            }
            index+=3;
          }
          index+=nextline;
        }

        // Use the histogram to find the tip : centroid of the 300 nearest points
        int tip_tot=0;
        float tip_x=0;
        float tip_y=0;
        float tip_z=0;
        int i=1;
        while ((tip_tot<50)&&(i<1024))
        {
          tip_tot+=depth_histogram[i];
          tip_x+=depth_histogram_x[i];
          tip_y+=depth_histogram_y[i];
          tip_z+=i*depth_histogram[i];
          i++;
        }
        m_shared_scene->tip_x=tip_x/tip_tot;
        m_shared_scene->tip_y=tip_y/tip_tot;
        m_shared_scene->tip_z=(float)(tip_z)/tip_tot;

        // Use the histogram to find the head : centroid of the 300 highest points
        int head_tot=0;
        float head_x=0;
        float head_y=0;
        float head_z=0;
        i=1;
        while ((head_tot<300)&&(i<480))
        {
          head_tot+=vertical_histogram[i];
          head_x+=vertical_histogram_x[i];
          head_z+=vertical_histogram_z[i];
          head_y+=i*vertical_histogram[i];
          i++;
        }
        m_shared_scene->head_x=head_x/head_tot;
        m_shared_scene->head_y=head_y/head_tot;
        m_shared_scene->head_z=(float)(head_z)/head_tot;
      }

      return b;
}

void ZCamProcessing::process_blobs(const float z_near, const float z_far,
                                   const float x1, const float x2,
                                   const float y1, const float y2,
                                   std::vector<blob> &results)
{
  results.clear();

  static uint16_t tmp_img[640*480];
  static int bag[640*480];

  int bag_size = 0;
  int i;
  int found_one = 0;
  int cur_blob_index = 0;

  const uint16_t zn16 = (uint16_t)(pow(z_near*4000000.0,0.333333));
  const uint16_t zf16 = (uint16_t)(pow(z_far*4000000.0,0.333333));

  blob b;

  b.cx=0;
  b.cy=0;
  b.x1=10000;
  b.x2=0;
  b.y1=10000;
  b.y2=0;
  b.tip_z=std::numeric_limits<int>::max();
  b.z2=std::numeric_limits<int>::min();;
  b.area=0;
  b.perimeter=0;

  memset(tmp_img, 0, sizeof(uint16_t)*640*480);
  memset(m_out_data.dbg_data, 0, sizeof(unsigned char)*3*640*480);

  for(int y=y1;y<y2;y++)
  {
    int i = y*640+x1;
    for(int x=x1;x<x2;x++)
    {
      if((m_out_data.depth_data[i]>zn16)
          && (m_out_data.depth_data[i]<zf16)
          && (tmp_img[i]==0))
      {
        found_one=1;
        bag_size=1;
        bag[0]=i;
        cur_blob_index++;

        while(bag_size>0)
        {
          int cur_i;
          cur_i = bag[bag_size-1];
          m_out_data.dbg_data[cur_i*3]=0;//(cur_blob_index*513)%255;
          m_out_data.dbg_data[cur_i*3+1]=128;//(cur_blob_index*12513)%255;
          m_out_data.dbg_data[cur_i*3+2]=0;//(cur_blob_index*551513)%255;
          bag_size--;
          b.area++;
          b.cx += cur_i%640;
          b.cy += cur_i/640;
          b.x1 = b.x1>cur_i%640?cur_i%640:b.x1;
          b.x2 = b.x2<cur_i%640?cur_i%640:b.x2;
          b.y2 = b.y2<cur_i/640?cur_i/640:b.y2;
          if(b.y1>cur_i/640)
          {
            b.y1 = cur_i/640;
            b.head_x = cur_i%640;
            b.head_y = cur_i/640;
            b.head_z = m_out_data.depth_data[cur_i];
          }

          if(m_out_data.depth_data[cur_i]<b.tip_z)
          {
            b.tip_x = cur_i%640;
            b.tip_y = cur_i/640;
            b.tip_z = m_out_data.depth_data[cur_i];
          }
          if(m_out_data.depth_data[cur_i]>b.z2)
          {
            b.z2 = m_out_data.depth_data[cur_i];
          }

          bool perimeter=false;

          if((cur_i<640)||(cur_i>=479*640)) continue;

          if((cur_i%640>x1)
              && (m_out_data.depth_data[(cur_i-1)]>zn16)
              && (m_out_data.depth_data[(cur_i-1)]<zf16))
          {
            if(tmp_img[cur_i-1]==0)
            {
              bag[bag_size++]=cur_i-1;
              tmp_img[cur_i-1]=cur_blob_index;
            }
          }
          else perimeter = true;

          if((cur_i%640<x2)
              && (m_out_data.depth_data[(cur_i+1)]>zn16)
              && (m_out_data.depth_data[(cur_i+1)]<zf16))
          {
            if(tmp_img[cur_i+1]==0)
            {
              bag[bag_size++]=cur_i+1;
              tmp_img[cur_i+1]=cur_blob_index;
            }
          }
          else perimeter = true;

          if((cur_i/640>y1)
              && (m_out_data.depth_data[(cur_i-640)]>zn16)
              && (m_out_data.depth_data[(cur_i-640)]<zf16))
          {
            if(tmp_img[cur_i-640]==0)
            {
              bag[bag_size++]=cur_i-640;
              tmp_img[cur_i-640]=cur_blob_index;
            }
          }
          else perimeter = true;

          if((cur_i/640<y2)
              && (m_out_data.depth_data[(cur_i+640)]>zn16)
              && (m_out_data.depth_data[(cur_i+640)]<zf16))
          {
            if(tmp_img[cur_i+640]==0)
            {
              bag[bag_size++]=cur_i+640;
              tmp_img[cur_i+640]=cur_blob_index;
            }
          }
          else perimeter = true;

          if(perimeter)
          {
            b.perimeter++;
            m_out_data.dbg_data[cur_i*3]=0;
            m_out_data.dbg_data[cur_i*3+1]=254;
            m_out_data.dbg_data[cur_i*3+2]=0;
          }
        }

        b.cx /= b.area;
        b.cy /= b.area;
        if(b.area>15)
          results.push_back(b);
        b.cx=0;
        b.cy=0;
        b.x1=10000;
        b.x2=0;
        b.y1=10000;
        b.y2=0;
        b.tip_z=std::numeric_limits<int>::max();
        b.z2=std::numeric_limits<int>::min();;
        b.area=0;
        b.perimeter=0;
      }
      i++;
    }
  }
}

void ZCamProcessing::set_background_depth(float zdebug)
{
  const float MULTIPLY_FACTOR = 0.99f;
  short * buf;
  int i;
  uint32_t ts;

  buf = new short[640*480];

  if (freenect_sync_get_depth((void**)&(m_out_data.depth_data), &ts, 0, FREENECT_DEPTH_11BIT) < 0)
  {
    return;
  }

  if(m_background_depth==0)
  {
    m_background_depth = new short[sizeof(short)*640*480];
    m_out_data.background_depth = m_background_depth;
  }
  memcpy(m_background_depth, m_out_data.depth_data, sizeof(short)*640*480);
  i=1;
  int firsti=0;
  while(i<640*480)
  {
    if(m_background_depth[i]>1024)
    {
      while((i<640*480-1)&&(m_background_depth[i]>1024))
      {
        i++;
      }
      int j;
      short v = min(m_background_depth[i], m_background_depth[firsti]);
      for(j=firsti;j<i;j++)
      {
        m_background_depth[j]=v;
      }
      firsti=i;
    }
    else
    {
      firsti=i;
    }
    i++;
  }
  for(i=0;i<640*480;i++)
  {
    m_background_depth[i]*=MULTIPLY_FACTOR;
  }

  for(int k=0;k<10;k++)
  {
    for(i=641;i<640*480-641;i++)
    {
      short v = m_background_depth[i];
      v=min(
            min(
              min(v,m_background_depth[i-641]),
              min(m_background_depth[i-640], m_background_depth[i-639])),
            min(
              min(m_background_depth[i-1], m_background_depth[i+1]),
              min(m_background_depth[i+639], m_background_depth[i+640])
              ));
      v=min(v,m_background_depth[i+641]);
      buf[i] = v;
    }
    memcpy(m_background_depth,buf,640*480*sizeof(short));
  }
  delete buf;
}

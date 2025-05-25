#include "camera.h"
#include <fcntl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/ioctl.h>

CameraError CameraInit(const char* camera_path, const CameraConfig* config) 
{
    if (camera_path == NULL) return kErrorInvalidArgument;
    
    // 打开设备
    int device_fd = open(camera_path, O_RDWR);
    if (device_fd < 0) 
    {
        printf("open camera error!\r\n");
        return kErrorOpen;
    }
    
    // 配置摄像头
    const CameraConfig* active_config;
    if (config != NULL)//判断用户是否传入配置还是使用默认配置
    {
        active_config = config;
    }
    else
    {
        // 默认配置
        CameraConfig default_config = 
        {
            .width = 640,
            .height = 480,
            .pixel_format = V4L2_PIX_FMT_MJPEG,
            .buffer_count = 4
        };
        active_config = &default_config;
    }

    //查询设备是否是捕获设备
    struct v4l2_capability camera_cap;
    memset(&camera_cap, 0, sizeof(struct v4l2_capability));
    if (ioctl(device_fd, VIDIOC_QUERYCAP, &camera_cap))
    {
        printf("device is not a captuer!\r\n");
        return kErrorCapability;
    }
    // 参数配置
    struct v4l2_format camera_format;
    memset(&camera_format, 0, sizeof(struct v4l2_format));
    camera_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_format.fmt.pix.width = active_config->width; //设置分辨率
    camera_format.fmt.pix.height = active_config->height;
    camera_format.fmt.pix.pixelformat = active_config->pixel_format;// 选择像素格式
    camera_format.fmt.pix.field = V4L2_FIELD_NONE;// 逐行扫描
    if (ioctl(device_fd, VIDIOC_S_FMT, &camera_format))
    {
        printf("camera set format error!\r\n");
        return kErrorFormat;
    }
    
    // 申请缓冲区
    struct v4l2_requestbuffers camaera_reqbuffs;
    memset(&camaera_reqbuffs, 0, sizeof(struct v4l2_requestbuffers));
    camaera_reqbuffs.count = 4;//申请四个缓冲区
    camaera_reqbuffs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camaera_reqbuffs.memory = V4L2_MEMORY_MMAP; //使用mmap内存映射模式
    if (ioctl(device_fd,VIDIOC_REQBUFS, &camaera_reqbuffs)) //在这个地方如果申请不了4个缓冲区的话会进行修改
    {
        printf("camera request buff error!\r\n");
        return kErrorReq;
    }
    
    //内存映射
    struct v4l2_buffer camera_buf;
    void *buffers[camaera_reqbuffs.count];
    if (camera_cap.capabilities & V4L2_CAP_STREAMING)//如果摄像头支持mmap
    {
        for (int i = 0; i < camaera_reqbuffs.count; i++)
        {
            memset(&camera_buf,0,sizeof(struct v4l2_buffer));
            camera_buf.index = i;
        	camera_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	camera_buf.memory = V4L2_MEMORY_MMAP;
            if (ioctl(device_fd,VIDIOC_QUERYBUF,&camera_buf))
            {
                printf("query buffer error!\r\n");
                return -1;
            }
            //映射到用户空间
            buffers[i] = mmap(0 /* start anywhere */ ,
                camera_buf.length, PROT_READ, MAP_SHARED, device_fd,
                camera_buf.m.offset);
            if (buffers[i] == MAP_FAILED)
            {
                printf("map to buffer error!\r\n");
                return -1;
            } 
        }
        /* Queue the buffers. */ //将缓冲区加入队列
        for (int i = 0; i < camaera_reqbuffs.count; i++) 
        {
        	memset(&camera_buf, 0, sizeof(struct v4l2_buffer));
        	camera_buf.index = i;
        	camera_buf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	camera_buf.memory = V4L2_MEMORY_MMAP;
        	if (ioctl(device_fd, VIDIOC_QBUF, &camera_buf))
            {
        	    printf("queue buffer error\r\n");
        	    return -1;
        	}
        }
    }
    printf("init success!\r\n");
    return kOk;
}

int main()
{
    CameraInit("/dev/video0",NULL);
    return 0;
}

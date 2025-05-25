#include "camera.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

CameraError CameraInit(const char* camera_path, const CameraConfig* config, CameraDevice *out_dev) 
{
    if (camera_path == NULL || out_dev == NULL) return kErrorInvalidArgument;
    
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
    out_dev->bufs = (struct v4l2_buffer*)malloc(camaera_reqbuffs.count * sizeof(struct v4l2_buffer));//使用malloc避免函数结束之后内存被自动释放
    out_dev->mmap_buffers = (void**)malloc(camaera_reqbuffs.count * sizeof(void*));
    if (!out_dev->bufs || !out_dev->mmap_buffers)//判断malloc是否成功
    {
        free(out_dev->bufs);
        free(out_dev->mmap_buffers);
        close(device_fd);
        return kErrorMemory;
    }
    
    if (camera_cap.capabilities & V4L2_CAP_STREAMING)//如果摄像头支持mmap
    {
        for (int i = 0; i < camaera_reqbuffs.count; i++)
        {
            memset(&out_dev->bufs[i],0,sizeof(struct v4l2_buffer));
            out_dev->bufs[i].index = i;
        	out_dev->bufs[i].type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	out_dev->bufs[i].memory = V4L2_MEMORY_MMAP;
            if (ioctl(device_fd,VIDIOC_QUERYBUF,&out_dev->bufs[i]))
            {
                printf("query buffer error!\r\n");
                for (int j = 0; j < i; j++) munmap(out_dev->mmap_buffers[j], out_dev->bufs[j].length);
                free(out_dev->bufs);
                free(out_dev->mmap_buffers);
                close(device_fd);
                return kErrorBuffer;
            }
            //映射到用户空间
            out_dev->mmap_buffers[i] = mmap(0 /* start anywhere */ ,
                                       out_dev->bufs[i].length, PROT_READ, MAP_SHARED, device_fd,
                                       out_dev->bufs[i].m.offset);
            if (out_dev->mmap_buffers[i] == MAP_FAILED)
            {
                printf("map to buffer error!\r\n");
                // 清理已映射的缓冲区
                for (int j = 0; j < i; j++) munmap(out_dev->mmap_buffers[j], out_dev->bufs[j].length);
                free(out_dev->bufs);
                free(out_dev->mmap_buffers);
                close(device_fd);
                return kErrorMap;
            } 
        }
        /* Queue the buffers. */ //将缓冲区加入队列
        for (int i = 0; i < camaera_reqbuffs.count; i++) 
        {
        	memset(&out_dev->bufs[i], 0, sizeof(struct v4l2_buffer));
        	out_dev->bufs[i].index = i;
        	out_dev->bufs[i].type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	out_dev->bufs[i].memory = V4L2_MEMORY_MMAP;
        	if (ioctl(device_fd, VIDIOC_QBUF, &out_dev->bufs[i]))
            {
        	    printf("queue buffer error\r\n");
        	    // 清理所有资源
                for (int j = 0; j < camaera_reqbuffs.count; j++) munmap(out_dev->mmap_buffers[j], out_dev->bufs[j].length);
                free(out_dev->bufs);
                free(out_dev->mmap_buffers);
                close(device_fd);
                return kErrorQueue;
        	}
        }
    }
    // 填充输出结构体
    out_dev->fd = device_fd;
    out_dev->width = active_config->width;
    out_dev->height = active_config->height;
    out_dev->pixel_format = active_config->pixel_format;
    out_dev->buf_count = camaera_reqbuffs.count;
    printf("init success!\r\n");
    return kOk;
}

CameraError CameraStartCapture(int device_fd)
{
    // (3)开始采集
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(device_fd, VIDIOC_STREAMON, &type)) 
    {
        printf("Failed to start streaming\r\n");
        close(device_fd);
        return kErrorStreamOn;
    }
    printf("stat streaming\r\n");
    return kOk;
}

CameraError CameraCaptureFrame(CameraDevice *dev, const char *output_path)//采集图片
{
    if (dev == NULL || output_path == NULL) 
    {
        return kErrorInvalidArgument;
    }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    // 从队列取出已填充数据的缓冲区
    if (ioctl(dev->fd, VIDIOC_DQBUF, &buf) < 0) {
        printf("VIDIOC_DQBUF failed\r\n");
        return kErrorDQBuf;
    }

    // 处理数据（保存为文件）
    FILE *fp = fopen(output_path, "wb");
    if (fp == NULL) {
        printf("Failed to open output file\r\n");
        // 即使出错也要把缓冲区重新加入队列
        ioctl(dev->fd, VIDIOC_QBUF, &buf);
        return kErrorFile;
    }

    fwrite(dev->mmap_buffers[buf.index], 1, buf.bytesused, fp);
    fclose(fp);

    // 将缓冲区重新加入队列
    if (ioctl(dev->fd, VIDIOC_QBUF, &buf) < 0) {
        printf("VIDIOC_QBUF failed\r\n");
        return kErrorQBuf;
    }

    return kOk;
}

int main()
{
    CameraDevice cam = {0};
    CameraInit("/dev/video0",NULL, &cam);
    CameraStartCapture(cam.fd);
    while (1)
    {
        CameraCaptureFrame(&cam,"../img/frame.jpg");
    }
    
    
    return 0;
}

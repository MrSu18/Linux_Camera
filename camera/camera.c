#include "camera.h"
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

/***************************************************************************
* @brief : 摄像头初始化，进行参数配置，申请缓冲区
* @param : const char* camera_path：设备路径
*          const CameraConfig* config：摄像头参数配置，NULL参数表示默认
*          CameraDevice *out_dev：相机设备结构体，存储相机分配到的资源信息
* @return: CameraError 错误码
* @date  : 2025.5.27
* @author: sushizhou
****************************************************************************/
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
    out_dev->fd = device_fd;

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
        CameraClose(out_dev);
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
        CameraClose(out_dev);
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
        CameraClose(out_dev);
        return kErrorReq;
    }
    
    //内存映射
    out_dev->bufs = (struct v4l2_buffer*)malloc(camaera_reqbuffs.count * sizeof(struct v4l2_buffer));//使用malloc避免函数结束之后内存被自动释放
    out_dev->mmap_buffers = (void**)malloc(camaera_reqbuffs.count * sizeof(void*));
    if (!out_dev->bufs || !out_dev->mmap_buffers)//判断malloc是否成功
    {
        // free(out_dev->bufs);
        // free(out_dev->mmap_buffers);
        // close(device_fd);
        CameraClose(out_dev);
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
                // for (int j = 0; j < i; j++) munmap(out_dev->mmap_buffers[j], out_dev->bufs[j].length);
                // free(out_dev->bufs);
                // free(out_dev->mmap_buffers);
                // close(device_fd);
                CameraClose(out_dev);
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
                // for (int j = 0; j < i; j++) munmap(out_dev->mmap_buffers[j], out_dev->bufs[j].length);
                // free(out_dev->bufs);
                // free(out_dev->mmap_buffers);
                // close(device_fd);
                CameraClose(out_dev);
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
                // for (int j = 0; j < camaera_reqbuffs.count; j++) munmap(out_dev->mmap_buffers[j], out_dev->bufs[j].length);
                // free(out_dev->bufs);
                // free(out_dev->mmap_buffers);
                // close(device_fd);
                CameraClose(out_dev);
                return kErrorQueue;
        	}
        }
        out_dev->buf_count++;//这里先让dev的buf_count跟着i自加是为了方便出错之后，便于用cameraclose函数进行资源释放
    }
    // 填充输出结构体
    out_dev->width = active_config->width;
    out_dev->height = active_config->height;
    out_dev->pixel_format = active_config->pixel_format;
    out_dev->buf_count = camaera_reqbuffs.count;
    printf("init success!\r\n");
    return kOk;
}

/***************************************************************************
* @brief  : 启动视频流采集，开始摄像头连续帧捕获
* @param  : int device_fd - 已初始化的摄像头设备文件描述符
* @return : CameraError - 错误码
* @date   : 2025.5.25
* @author : sushizhou
* @note   : 需在CameraInit成功后调用，与CameraStopCapture配对使用
****************************************************************************/
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

/***************************************************************************
* @brief  : 捕获单帧图像并保存到指定路径
* @param  : CameraDevice *dev - 已初始化的摄像头设备结构体指针
*           const char *output_path - 图像保存路径（需确保目录存在）
* @return : CameraError - 错误码
* @date   : 2025.5.25
* @author : sushizhou
* @note   : 1.需在CameraStartCapture之后调用
*           2.会自动完成缓冲区出队->保存->重新入队流程
*           3.支持JPEG/MJPEG等常见格式
****************************************************************************/
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
    if (ioctl(dev->fd, VIDIOC_DQBUF, &buf) < 0) 
    {
        printf("VIDIOC_DQBUF failed\r\n");
        return kErrorDQBuf;
    }

    // 处理数据（保存为文件）
    FILE *fp = fopen(output_path, "wb");
    if (fp == NULL) 
    {
        printf("Failed to open output file\r\n");
        // 即使出错也要把缓冲区重新加入队列
        ioctl(dev->fd, VIDIOC_QBUF, &buf);
        return kErrorFile;
    }
    fwrite(dev->mmap_buffers[buf.index], 1, buf.bytesused, fp);
    fclose(fp);

    // 将缓冲区重新加入队列
    if (ioctl(dev->fd, VIDIOC_QBUF, &buf) < 0) 
    {
        printf("VIDIOC_QBUF failed\r\n");
        return kErrorQBuf;
    }

    return kOk;
}

/***************************************************************************
* @brief  : 关闭摄像头释放资源
* @param  : CameraDevice *dev - 需要关闭的摄像头设备结构体指针
* @return : CameraError - 错误码
* @date   : 2025.5.27
* @author : sushizhou
* @note   : NULL
****************************************************************************/
CameraError CameraClose(CameraDevice *dev)//关闭摄像头释放摄像头资源
{
    if (dev == NULL)
    {
        printf("camera close illegal parameter!\r\n");
        return kErrorInvalidArgument;
    }
    // 1. 停止数据流（如V4L2的streamoff）
    if (dev->fd >= 0) //只关闭有效的fd，只有fd是个非负整数才是有效的，表示成功打开的资源
    {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(dev->fd, VIDIOC_STREAMOFF, &type); // 忽略错误，确保继续释放其他资源
    }
    
    // 2. 释放内存缓冲区
    if (dev->mmap_buffers)
    {
        for (int i = 0; i < dev->buf_count; i++)
        {
            if (dev->mmap_buffers[i] != MAP_FAILED)
            {
                munmap(dev->mmap_buffers[i],dev->bufs[i].length);
            }
            
        }
        free(dev->mmap_buffers);
    }
    free(dev->bufs); //释放缓冲区信息数组
    // 3. 关闭文件描述符
    close(dev->fd);
    // 4. 清零结构体（避免悬空指针）
    memset(dev, 0, sizeof(CameraDevice));
    printf("camera close success!\r\n");
    return kOk;
}

/***************************************************************************
* @brief  : 枚举摄像头的支持信息
* @param  : const char* camera_path - 摄像头设备路径
* @return : CameraError - 错误码
* @date   : 2025.6.16
* @author : sushizhou
* @note   : NULL
****************************************************************************/
CameraError CameraEnumFmt(const char* camera_path)
{
    //1. open
    int fd = open(camera_path, O_RDWR);
    if (fd < 0)
    {
        printf("camera open error--CameraEnumFmt\r\n");
        close(fd);
        return kErrorOpen;        
    }
    //2. ioctl VIDIOC_ENUM_FMT
    struct v4l2_fmtdesc fmtdesc;
    struct v4l2_frmsizeenum fsenum;
    int fmt_index = 0;
    int frame_index = 0;
    while (1)
    {
        //枚举摄像头支持的格式
        fmtdesc.index = fmt_index;//因为这个索引相当于是链表的地址
        fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;//指定type为捕获
        if (0 != ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc))
        {
            break;
        }  
        printf("format %s,%d:\r\n",fmtdesc.description,fmtdesc.pixelformat);
        //3. ioctl VIDIOC_ENUM_FRAMESIZES
        frame_index = 0;
        while (1)
        {
            //枚举这种格式所支持的帧大小
            memset(&fsenum,0,sizeof(fsenum));
            fsenum.pixel_format = fmtdesc.pixelformat;
            fsenum.index = frame_index;
            if (0 != ioctl(fd,VIDIOC_ENUM_FRAMESIZES,&fsenum))
                break;
            else
            {
                printf("\tframessize: %d: %d x %d\r\n",fsenum.index,fsenum.discrete.width,fsenum.discrete.height);
            }
            frame_index++;
        }
        fmt_index++;
    }
    // 4. close
    close(fd);
    printf("camera enum fmt end!\r\n");
    return kOk;
}

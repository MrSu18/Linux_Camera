#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/videodev2.h>
#include <poll.h>
#include "camera_manager.h"

//这个应用程序支持的图像格式
const uint32_t support_formats[]={V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_MJPEG, V4L2_PIX_FMT_RGB565};
//这个可以看作是一个临时变量,当一个设备需要初始化的时候,就会对这个进行修改,如果有不支持的比如mmap和r/w进行修改之后,再对这个操作集进行注册,然后并且让设备指向这个操作集
CameraOperation v4l2_camera_opration;

FunctionStatus V4l2GetFrameForReadWrite();
FunctionStatus V4l2PutFrameForReadWrite();

/***************************************************************************
* @brief : 检查是否支持指定的像素格式  
* @param : uint32_t pixel_format : 需要检查的像素格式（如 V4L2_PIX_FMT_YUYV 等）  
* @return: FunctionStatus : 返回 kSuccess 表示支持，kERROR 表示不支持  
* @date  : 2025.6.24  
* @author: sushizhou  
****************************************************************************/ 
FunctionStatus IsSupportThisFormat(uint32_t pixel_format)
{
    int i;
    for (i = 0; i < sizeof(support_formats)/sizeof(support_formats[0]); i++)
    {
        if (support_formats[i] == pixel_format)
            return kSuccess;
    }
    return kERROR;
}

/***************************************************************************
 * @brief  : 初始化V4L2摄像头设备
 * @param  : const char* camera_path       : 摄像头设备节点路径(如"/dev/video0")
 *           CameraDevicePtr camera_device : 摄像头设备结构体指针，用于保存初始化后的设备信息
 * @return : FunctionStatus               : 返回kSuccess表示成功，kERROR表示失败
 * @note   : 1. 支持自动检测设备能力(mmap或read/write)
 *           2. 自动匹配支持的像素格式
 *           3. 缓冲区申请失败时会自动回滚资源
 * @date   : 2025.6.24
 * @author : sushizhou
 ***************************************************************************/
FunctionStatus V4l2InitDevice(const char *camera_path, CameraDevicePtr camera_device)
{
    if (camera_path==NULL || camera_device==NULL) 
    {
        printf("error: path or device is NULL!\r\n");
        goto err_exit;
    }
    // 1.打开设备 open
    int device_fd = open(camera_path, O_RDWR);
    if (device_fd < 0) 
    {
        printf("error: can't open camera!\r\n");
        goto err_exit;
    }
    camera_device->fd = device_fd;

    //2.查询设备是否是捕获设备 VIDIOC_QUERYCAP
    struct v4l2_capability camera_cap;
    memset(&camera_cap, 0, sizeof(struct v4l2_capability));
    if (ioctl(device_fd, VIDIOC_QUERYCAP, &camera_cap))
    {
        printf("error: device is not a captuer!\r\n");
        goto err_exit;
    }

    // 3.查询摄像头支持哪种格式 VIDIOC_ENUM_FMT
    struct v4l2_fmtdesc fmt_desc;
    memset(&fmt_desc, 0, sizeof(fmt_desc));
	fmt_desc.index = 0;
	fmt_desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	while (ioctl(camera_device->fd, VIDIOC_ENUM_FMT, &fmt_desc) == 0) 
    {
        if (IsSupportThisFormat(fmt_desc.pixelformat)==kSuccess)
        {
            camera_device->pixel_format = fmt_desc.pixelformat;
            break;
        }
		fmt_desc.index++;
	}
    if (!camera_device->pixel_format) //上面遍历完了,发现摄像头支持的格式跟我们写的支持的都匹配不上
    {
    	printf("error: can't support the format of this device\n");
        goto err_exit;        
    }

    //4. 设置摄像头使用哪种格式 VIDIOC_S_FMT
    uint32_t lcd_width=640,lcd_height=480,lcd_bpp=120;//这里还没写先自定义
    //先获取屏幕的分辨率,设置分辨率跟屏幕一样
    struct v4l2_format camera_format;
    memset(&camera_format, 0, sizeof(struct v4l2_format));
    camera_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_format.fmt.pix.pixelformat=camera_device->pixel_format;//像素格式选上面支持的
    camera_format.fmt.pix.width = lcd_width; //设置分辨率
    camera_format.fmt.pix.height = lcd_height;
    camera_format.fmt.pix.field = V4L2_FIELD_ANY;//设置扫描方式
    if (ioctl(device_fd, VIDIOC_S_FMT, &camera_format))
    {
        printf("error: unable to set format !\r\n");
        goto err_exit;
    }
    camera_device->width=camera_format.fmt.pix.width;//这里驱动程序如果发现不能满足想设定的参数,会调整参数,所以这里接收最后的参数
    camera_device->height=camera_format.fmt.pix.height;

    // 5. 申请缓冲区 VIDIOC_REQBUFS
    struct v4l2_requestbuffers camera_reqbuffs;
    memset(&camera_reqbuffs, 0, sizeof(struct v4l2_requestbuffers));
    camera_reqbuffs.count = BUFFER_NUM;//申请四个缓冲区,但是能申请多少个最后是用驱动程序给出
    camera_reqbuffs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    camera_reqbuffs.memory = V4L2_MEMORY_MMAP; //使用mmap内存映射模式
    if (ioctl(device_fd,VIDIOC_REQBUFS, &camera_reqbuffs)) //向设备申请缓存区
    {
        printf("error: Unable to allocate buffers.\n");
        goto err_exit;
    }
    camera_device->buf_count=camera_reqbuffs.count;

    //如果摄像头支持的是MMAP
    struct v4l2_buffer camera_buf;
    if (camera_cap.capabilities & V4L2_CAP_STREAMING)
    {
        //(1) 查询每个buffer的信息,进行内存映射mmap VIDIOC_QUERYBUF
        for (int i = 0; i < camera_device->buf_count; i++)
        {
            memset(&camera_buf,0,sizeof(struct v4l2_buffer));
            camera_buf.index = i;
        	camera_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	camera_buf.memory = V4L2_MEMORY_MMAP;
            if (ioctl(device_fd,VIDIOC_QUERYBUF,&camera_buf))//获取缓存帧的地址、长度;mmap映射的时候有长度什么的信息需要知道所以这里有查询buf的信息
            {
                printf("query buffer error!\r\n");
                goto err_exit;
            }
            camera_device->buf_maxlen=camera_buf.length;
            //映射到用户空间
            camera_device->mmap_buffers[i] = mmap(0 /* start anywhere */ ,
                            camera_buf.length, PROT_READ, MAP_SHARED, device_fd,
                            camera_buf.m.offset);
            if (camera_device->mmap_buffers[i] == MAP_FAILED)
            {
                printf("map to buffer error!\r\n");
                goto err_exit;
            } 
        }
        // (2) 将缓冲区放入队列 VIDIOC_QBUF
        for (int i = 0; i < camera_device->buf_count; i++) 
        {
        	memset(&camera_buf, 0, sizeof(struct v4l2_buffer));
        	camera_buf.index = i;
        	camera_buf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        	camera_buf.memory = V4L2_MEMORY_MMAP;
        	if (ioctl(device_fd, VIDIOC_QBUF, &camera_buf))//放入队列
            {
        	    printf("queue buffer error\r\n");
        	    goto err_exit;
        	}
        }
    }
    //如果摄像头支持的是R/W
    else if (camera_cap.capabilities & V4L2_CAP_READWRITE)
    {
        v4l2_camera_opration.GetFrame = V4l2GetFrameForReadWrite;
        v4l2_camera_opration.PutFrame = V4l2PutFrameForReadWrite;
        /* read(fd, buf, size) */
        camera_device->buf_count  = 1;
        /* 在这个程序所能支持的格式里, 一个象素最多只需要4字节 */
        camera_device->buf_maxlen = camera_device->width * camera_device->height * 4;
        camera_device->mmap_buffers[0] = malloc(camera_device->buf_maxlen);
    }
    camera_device->pt_opr=&v4l2_camera_opration;//暂时用一下后续还要修改,我觉得这样使用不是很好.
    return kSuccess;

err_exit:
    close(device_fd);
    return kERROR;
}

/***************************************************************************
 * @brief  : 关闭并释放V4L2摄像头设备资源
 * @param  : CameraDevicePtr camera_device : 摄像头设备结构体指针
 * @return : FunctionStatus                : 返回kSuccess表示成功，kERROR表示失败
 * @note   : 1. 会解除所有mmap映射的缓冲区
 *           2. 关闭设备文件描述符
 *           3. 不释放camera_device结构体本身的内存
 * @date   : 2025.6.24
 * @author : sushizhou
 ***************************************************************************/
FunctionStatus V4l2ExitDevice(CameraDevicePtr camera_device)
{
    int i;
    for (i = 0; i < camera_device->buf_count; i++)
    {
        if (camera_device->mmap_buffers[i])
        {
            munmap(camera_device->mmap_buffers[i], camera_device->buf_maxlen);
            camera_device->mmap_buffers[i] = NULL;
        }
    }
        
    close(camera_device->fd);
    return kSuccess;
}

/***************************************************************************
 * @brief  : 从V4L2摄像头设备获取一帧数据用于流式传输
 * @param  : CameraDevicePtr camera_device : 摄像头设备结构体指针
 * @param  : CameraBufPtr camera_buf       : 输出帧数据存储结构体指针
 * @return : FunctionStatus                : 返回kSuccess表示成功，kERROR表示失败
 * @note   : 1. 使用poll机制等待摄像头数据就绪
 *           2. 通过VIDIOC_DQBUF从队列取出缓冲区
 *           3. 填充输出参数包括分辨率/格式/像素数据指针等
 *           4. 像素数据内存由驱动mmap映射，调用者无需额外分配
 * @date   : 2025.6.25
 * @author : sushizhou
 ***************************************************************************/
FunctionStatus V4l2GetFrameForStreaming(CameraDevicePtr camera_device, CameraBufPtr camera_buf)
{
    //1. poll等待有数据可读
    struct pollfd fds[1];//数组每个成员描述一个要监视的fd
    fds[0].fd=camera_device->fd;
    fds[0].events=POLLIN;//要监视的事件是:数据可读
    if (poll(fds,1,-1)<=0)//监视fds数组的前1个fd,超出时间-1表示一直阻塞等待
    {
        printf("error: poll error!\r\n");
        return kERROR;
    }

    //2. 从队列中取出 VIDIOC_DQBUF
    struct v4l2_buffer v4l2_buf;
    memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    if(ioctl(camera_device->fd, VIDIOC_DQBUF, &v4l2_buf))
    {
        printf("error: Unable to dequeue buffer.\r\n");
    	return -1;
    }
    camera_device->buf_cur_index=v4l2_buf.index;
    //3. 把缓冲区的数据记录下来到camera_buf中
    camera_buf->pixel_format=camera_device->pixel_format;
    camera_buf->width=camera_device->width;
    camera_buf->height=camera_device->height;
    camera_buf->bpp=(camera_device->pixel_format==V4L2_PIX_FMT_YUYV)? 16 : \
                    (camera_device->pixel_format == V4L2_PIX_FMT_MJPEG) ? 0 :  \
                    (camera_device->pixel_format == V4L2_PIX_FMT_RGB565) ? 16 :  \
                    0;
    camera_buf->line_bytes    = camera_device->width * camera_buf->bpp / 8;
    camera_buf->total_bytes   = v4l2_buf.bytesused;
    camera_buf->auc_pixel_datas = camera_device->mmap_buffers[v4l2_buf.index]; 
    
    return kSuccess;
}

/***************************************************************************
 * @brief  : 将处理后的视频帧缓冲区重新放回V4L2摄像头队列
 * @param  : CameraDevicePtr camera_device : 摄像头设备结构体指针
 * @param  : CameraBufPtr camera_buf       : 帧数据存储结构体指针(未直接使用)
 * @return : FunctionStatus                : 返回kSuccess表示成功，kERROR表示失败 
 * @note   : 1. 必须与V4l2GetFrameForStreaming配对使用
 *           2. 通过VIDIOC_QBUF将缓冲区重新加入驱动队列
 *           3. 使用camera_device->buf_cur_index确定缓冲区位置
 *           4. 这是V4L2视频采集流程的关键步骤(取帧->处理->还帧)
 * @date   : 2025.06.25
 * @author : sushizhou
 ***************************************************************************/
FunctionStatus V4l2PutFrameForStreaming(CameraDevicePtr camera_device, CameraBufPtr camera_buf)
{
    /*处理完之后把缓冲区再放回队列 VIDIOC_QBUF */
    struct v4l2_buffer v4l2_buf;
	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf.index  = camera_device->buf_cur_index;
	v4l2_buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;
	if (ioctl(camera_device->fd, VIDIOC_QBUF, &v4l2_buf)) 
    {
	    printf("Unable to queue buffer.\n");
	    return kERROR;
	}
    return kSuccess;
}
FunctionStatus V4l2GetFrameForReadWrite()
{
    return kSuccess;
}
FunctionStatus V4l2PutFrameForReadWrite()
{
    return kSuccess;
}
FunctionStatus V4l2StartDevice(CameraDevicePtr camera_device)
{
    // (3)开始采集
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(camera_device->fd, VIDIOC_STREAMON, &type)) 
    {
        printf("Failed to start streaming\r\n");
        return kERROR;
    }
    return kSuccess;
}
FunctionStatus V4l2StopDevice(CameraDevicePtr camera_device)
{
    return kSuccess;
}
FunctionStatus V4l2GetFormat()
{
    return kSuccess;
}

//这个可以看作是一个临时变量,当一个设备需要初始化的时候,就会对这个进行修改,如果有不支持的比如mmap和r/w进行修改之后,再对这个操作集进行注册,然后并且让设备指向这个操作集
CameraOperation v4l2_camera_opration =
{
    .name        = "v4l2_mmap",
    .InitDevice  = V4l2InitDevice,
    .ExitDevice  = V4l2ExitDevice,
    .GetFormat   = V4l2GetFormat,
    .GetFrame    = V4l2GetFrameForStreaming,
    .PutFrame    = V4l2PutFrameForStreaming,
    .StartDevice = V4l2StartDevice,
    .StopDevice  = V4l2StopDevice,
};

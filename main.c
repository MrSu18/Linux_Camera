#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include "camera.h"

int main()
{
    CameraDevice cam = {0};
    CameraInit("/dev/video0",NULL, &cam);
    CameraStartCapture(cam.fd);
    while (1)
    {
        CameraCaptureFrame(&cam,"img/frame.jpg");
    }
    
    
    return 0;
}

// int main()
// {
//     char dev_str[20] = "/dev/video0";
//     struct v4l2_capability tV4l2Cap;//查询设备支持
    

//     // 1. 设备初始化
//     //打开摄像头
//     int fd = open(dev_str,O_RDWR);
//     if (fd < 0)
//     {
//         printf("打开设备失败\r\n");
//         return -1;
//     }
//     //ioctl VIDIOC_QUERYCAP Query Capbility，查询设备支持什么 我的输出结果/dev/video supports streaming i/o
//     memset(&tV4l2Cap, 0, sizeof(struct v4l2_capability));
//     int ret = ioctl(fd, VIDIOC_QUERYCAP, &tV4l2Cap);
//     if (ret) 
//     {
//     	printf("Error opening device %s: unable to query device.\n", dev_str);
//     	return -1;
//     }
//     if (!(tV4l2Cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) //查询是否是捕获设备
//     {
//     	printf("%s is not a video capture device\n", dev_str);
//         return -1;
//     }

// 	if (tV4l2Cap.capabilities & V4L2_CAP_STREAMING) 
//     {
// 	    printf("%s supports streaming i/o\n", dev_str);
// 	}
    
// 	if (tV4l2Cap.capabilities & V4L2_CAP_READWRITE) 
//     {
// 	    printf("%s supports read i/o\n", dev_str);
// 	}
//     // 2. 参数配置
//     struct v4l2_format  tV4l2Fmt;
//     memset(&tV4l2Fmt, 0, sizeof(struct v4l2_format));
//     tV4l2Fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     tV4l2Fmt.fmt.pix.width = 640;      // 设置分辨率
//     tV4l2Fmt.fmt.pix.height = 480;
//     tV4l2Fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;  // 选择像素格式
//     tV4l2Fmt.fmt.pix.field = V4L2_FIELD_NONE;           // 逐行扫描
//     ret = ioctl(fd, VIDIOC_S_FMT, &tV4l2Fmt); 
//     if (ret) 
//     {
//     	printf("Unable to set format\n");
//         return -1;        
//     }
//     // 3. 数据采集
//     // (1) 申请缓冲区
//     struct v4l2_requestbuffers tV4l2ReqBuffs;
//     memset(&tV4l2ReqBuffs, 0, sizeof(struct v4l2_requestbuffers));
//     tV4l2ReqBuffs.count = 4;//申请四个缓冲区
//     tV4l2ReqBuffs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     tV4l2ReqBuffs.memory = V4L2_MEMORY_MMAP; //使用mmap内存映射模式
//     ret = ioctl(fd,VIDIOC_REQBUFS, &tV4l2ReqBuffs);//在这个地方如果申请不了4个缓冲区的话会进行修改
//     if (ret<0)
//     {
//         printf("request buffer error!\r\n");
//         return -1;
//     }
//     // (2)内存映射
//     struct v4l2_buffer tV4l2Buf;
//     void *buffers[tV4l2ReqBuffs.count];
//     if (tV4l2Cap.capabilities & V4L2_CAP_STREAMING)//如果摄像头支持mmap
//     {
//         for (int i = 0; i < tV4l2ReqBuffs.count; i++)
//         {
//             memset(&tV4l2Buf,0,sizeof(struct v4l2_buffer));
//             tV4l2Buf.index = i;
//         	tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//         	tV4l2Buf.memory = V4L2_MEMORY_MMAP;
//             ret = ioctl(fd,VIDIOC_QUERYBUF,&tV4l2Buf);
//             if (ret < 0 )
//             {
//                 printf("query buffer error!\r\n");
//                 return -1;
//             }
//             //映射到用户空间
//             buffers[i] = mmap(0 /* start anywhere */ ,
//                 tV4l2Buf.length, PROT_READ, MAP_SHARED, fd,
//                 tV4l2Buf.m.offset);
//             if (buffers[i] == MAP_FAILED)
//             {
//                 printf("map to buffer error!\r\n");
//                 return -1;
//             } 
//         }
//         /* Queue the buffers. */ //将缓冲区加入队列
//         for (int i = 0; i < tV4l2ReqBuffs.count; i++) 
//         {
//         	memset(&tV4l2Buf, 0, sizeof(struct v4l2_buffer));
//         	tV4l2Buf.index = i;
//         	tV4l2Buf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//         	tV4l2Buf.memory = V4L2_MEMORY_MMAP;
//         	ret = ioctl(fd, VIDIOC_QBUF, &tV4l2Buf);
//         	if (ret)
//             {
//         	    printf("queue buffer error\r\n");
//         	    return -1;
//         	}
//         }
//     }
//     // (3)开始采集
//     enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     ret = ioctl(fd, VIDIOC_STREAMON, &type);
//     if (ret < 0) 
//     {
//         printf("VIDIOC_STREAMON failed\r\n");
//         close(fd);
//         return -1;
//     }
//     // (4)读取数据
//     while (1)
//     {
//         memset(&tV4l2Buf,0,sizeof(struct v4l2_buffer));
//         tV4l2Buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//         tV4l2Buf.memory = V4L2_MEMORY_MMAP;
//         // 从队列取出已填充数据的缓冲区
//         if (ioctl(fd, VIDIOC_DQBUF, &tV4l2Buf) < 0) 
//         {
//             printf("VIDIOC_DQBUF failed\r\n");
//             break;
//         }
//         // 处理数据（例如保存为文件）
//         FILE *fp = fopen("img/frame.jpg", "wb");
//         fwrite(buffers[tV4l2Buf.index], 1, tV4l2Buf.bytesused, fp);
//         fclose(fp);

//         // 将缓冲区重新加入队列
//         if (ioctl(fd, VIDIOC_QBUF, &tV4l2Buf) < 0) 
//         {
//             printf("VIDIOC_QBUF failed\r\n");
//             break;
//         }
//     }
    
//     // 4. 关闭摄像头
//     fclose(fd);
//     return 0;
// }


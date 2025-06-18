#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include "camera.h"
#include <pthread.h>

pthread_mutex_t cam_mutex;//摄像头操作的互斥量

static void *thread_control_camera_brightness (void *args);

int main()
{
    CameraEnumFmt("/dev/video0");
    CameraDevice cam = {0};
    if(kOk != CameraInit("/dev/video0",NULL, &cam)) 
    {
        printf("camera init error!\r\n");
        return -1;
    }
    CameraStartCapture(cam.fd);

    //创建线程
    pthread_t ctr_thread;
    pthread_mutex_init(&cam_mutex, NULL);  // 初始化互斥量
    pthread_create(&ctr_thread,NULL,thread_control_camera_brightness,(void*)cam.fd);

    while (1)
    {
        CameraCaptureFrame(&cam,"img/frame.jpg");
    }
    pthread_mutex_destroy(&cam_mutex);     // 在程序退出前销毁
    return 0;
}


static void *thread_control_camera_brightness (void *args)
{
    int fd = (int)args;
    //查询亮度信息
    struct v4l2_queryctrl ctrl_information;
    memset(&ctrl_information,0,sizeof(ctrl_information));
    ctrl_information.id=V4L2_CID_BRIGHTNESS;
    if(ioctl(fd,VIDIOC_QUERYCTRL,&ctrl_information))
    {
        printf("can't query brightness\r\n");
        return NULL;
    }
    printf("camera brightness min: %d, max: %d\r\n",ctrl_information.minimum,ctrl_information.maximum);

    //获得当前亮度值
    struct v4l2_control now_brightness;
    now_brightness.id=V4L2_CID_BRIGHTNESS;
    ioctl(fd,VIDIOC_G_CTRL,&now_brightness);

    //开始控制亮度
    __u_char commond=0;
    int deta=((ctrl_information.maximum-ctrl_information.minimum))/10;
    while (1)
    {
        commond=getchar();
        if (commond == 'u' || commond == 'U')
        {
            now_brightness.value += deta;
        }
        else if (commond == 'd' || commond == 'D')
        {
            now_brightness.value -= deta;
        }
        if (now_brightness.value > ctrl_information.maximum)
            now_brightness.value = ctrl_information.maximum;
        if (now_brightness.value < ctrl_information.minimum)
            now_brightness.value = ctrl_information.minimum;
        pthread_mutex_lock(&cam_mutex);//加锁避免读帧和控制亮度进行竞争导致出错
        ioctl(fd,VIDIOC_S_CTRL,&now_brightness);
        pthread_mutex_unlock(&cam_mutex);//解锁
    }
    return NULL;
    
}
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "camera_manager.h"
#include "img_convert_manager.h"
#include <string.h>
#include "dispaly_frame_buffer.h"
#include "color.h"

int main()
{
    ImgConvertPtr image_convert_opr;

    CameraBufPtr cur_image;
    CameraBuf camera_image;
    memset(&camera_image,0,sizeof(CameraBuf));
    CameraBuf convert_image;
    memset(&convert_image,0,sizeof(CameraBuf));
    convert_image.pixel_format=V4L2_PIX_FMT_RGB32;
    CameraBuf zoom_image;
    memset(&zoom_image,0,sizeof(CameraBuf));
    CameraBuf frame_image;
    memset(&frame_image,0,sizeof(CameraBuf));
    //摄像头部分模块初始化
    CameraInit("/dev/video1");
    memset(&camera_usb_buf,0,sizeof(camera_usb_buf));
    if (camera_main_usb.pixel_format==V4L2_PIX_FMT_YUYV)
    {
        printf("V4L2_PIX_FMT_YUYV\r\n");
    }
    
    //图像格式转换部分模块初始化
    ImgConvertInit();
    if(GetVideoConvertForFormats(camera_main_usb.pixel_format,convert_image.pixel_format,&image_convert_opr)==kERROR)
    {
        return -1;
    }
    if (LCDInit("/dev/fb0")==kERROR)
    {
        printf("lcd init error!\r\n");
        return -1;
    }
    //开启摄像头
    camera_main_usb.pt_opr->StartDevice(&camera_main_usb);
    while (1)
    {
        
        //采集数据
        if (camera_main_usb.pt_opr->GetFrame(&camera_main_usb,&camera_usb_buf)==kERROR)
        {
            return -1;
        }
        // printf("Y0=%d,U=%d,Y1=%d,V=%d\r\n",camera_usb_buf.auc_pixel_datas[0],camera_usb_buf.auc_pixel_datas[1],camera_usb_buf.auc_pixel_datas[2],camera_usb_buf.auc_pixel_datas[3]);
        memcpy(&camera_image,&camera_usb_buf,sizeof(CameraBuf));
        // 图像格式转换
        if (image_convert_opr->Convert(&camera_image,&convert_image)==kERROR)
        {
            return -1;
        }
        LCDShowPage(convert_image,10,10);

        //存放帧
        camera_main_usb.pt_opr->PutFrame(&camera_main_usb,&camera_usb_buf);
    }
    camera_main_usb.pt_opr->StopDevice(&camera_main_usb);
    camera_main_usb.pt_opr->ExitDevice(&camera_main_usb);
    return 0;
}



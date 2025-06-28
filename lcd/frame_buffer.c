#include "frame_buffer.h"
#include <linux/fb.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <unistd.h>

FunctionStatus FrameBufferInitDevice(const char *device_path, LcdDevicePtr device)
{
    //1. open打开设备节点
    int fd = open(device_path, O_RDWR);
    if (fd < 0)
	{
		printf("error: can't open %s\r\n",device_path);
		return kERROR;
	}
    device->fd=fd;

    //2. 读取lcd信息 FBIOGET_VSCREENINFO
    struct fb_var_screeninfo var;
    memset(&var,0,sizeof(var));
    if (ioctl(device->fd, FBIOGET_VSCREENINFO, &var))
	{
		printf("error: %s can't get var\n",device_path);
		return kERROR;
	}
    device->width=var.xres;
    device->height=var.yres;
    device->bpp=var.bits_per_pixel;
    device->screen_size=device->width*device->height*device->bpp/8;
    // printf("%d,%d,%d,%d\r\n",device->screen_size,device->width,device->height,device->bpp);
    //3. 内存映射mmap
    uint8_t* fb_base=(uint8_t*)mmap(NULL , device->screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, device->fd, 0);
    if (fb_base == MAP_FAILED)
	{
		printf("frame buffer can't mmap\n");
		return kERROR;
	}
    device->framebuffer=fb_base;
    
    //4.清屏 全部设为白色
    memset(device->framebuffer, 0xff, device->screen_size);

    //5. 获取设备的操作集
    LcdDisplayPtr lcd_operation=NULL;
    GetLcdDisplayNode(lcd_display_head,"framebuffer",&lcd_operation);
    if (lcd_operation==NULL)
    {
        printf("error: %s can't get operations!\r\n",device_path);
        return kERROR;
    }
    device->opr=lcd_operation;

    return kSuccess;
}
FunctionStatus FrameBufferExitDevice(LcdDevicePtr device)
{
    if (device->framebuffer!=NULL)
    {
        munmap(device->framebuffer,device->screen_size);
        device->framebuffer=NULL;
    }
    close(device->fd);
    return kSuccess;
}
FunctionStatus FrameBufferShowPixel(uint32_t x,uint32_t y,uint32_t color)
{
    return kSuccess;
}
FunctionStatus FrameBufferCleanScreen(uint32_t color)
{
    return kSuccess;
}
FunctionStatus FrameBufferShowPage(CameraBuf data,LcdDevice device)
{
    uint8_t* lcd_ptr=device.framebuffer;
    uint8_t* img_ptr=data.auc_pixel_datas;
    uint8_t lcd_byte_per_pixel=lcd.bpp/8;
    uint8_t img_byte_per_pixel=data.bpp/8;
    for(int i = 0; i <data.height; i++) 
    {
        for(int j = 0; j < data.width; j++) 
        {
            // *(lcd_ptr+j*4)=*(img_ptr+j*4);
            // *(lcd_ptr+j*4+1)=*(img_ptr+j*4+1);
            // *(lcd_ptr+j*4+2)=*(img_ptr+j*4+2);
            memcpy(lcd_ptr+j*lcd_byte_per_pixel,img_ptr+j*img_byte_per_pixel,4);//rgb用3个字节表示一个像素点
        }
        lcd_ptr += (device.width*lcd_byte_per_pixel);
        img_ptr += (data.width*img_byte_per_pixel);
    }
    return kSuccess;
}
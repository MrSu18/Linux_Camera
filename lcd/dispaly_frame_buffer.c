#include "dispaly_frame_buffer.h"
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

uint8_t* frambuff_base=NULL;
struct fb_var_screeninfo var;
int frambuff_fd;

FunctionStatus LCDInit(const char* device)//open("/dev/fb0", O_RDWR);
{
    frambuff_fd=open(device, O_RDWR);
    if (frambuff_fd < 0)
	{
		printf("error: can't open %s\r\n",device);
		return kERROR;
	}
    
    memset(&var,0,sizeof(var));
    if (ioctl(frambuff_fd, FBIOGET_VSCREENINFO, &var))
	{
		printf("error: %s can't get var\n",device);
		return kERROR;
	}
    uint32_t screen_size = var.xres * var.yres * var.bits_per_pixel / 8;
	frambuff_base= (uint8_t*)mmap(NULL , screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, frambuff_fd, 0);
	if (frambuff_base == MAP_FAILED)
	{
		printf("frame buffer can't mmap\n");
		return kERROR;
	}
    /* 清屏: 全部设为白色 */
	memset(frambuff_base, 0xff, screen_size);
    printf("LCD Format: %d bpp, R(%d,%d) G(%d,%d) B(%d,%d)\n",
       var.bits_per_pixel,
       var.red.offset, var.red.length,
       var.green.offset, var.green.length,
       var.blue.offset, var.blue.length);
    return kSuccess;
}

FunctionStatus LCDShowPage(CameraBuf image, int start_x, int start_y)
{
    int line_width  = var.xres * (var.bits_per_pixel / 8);
    int pixel_width = var.bits_per_pixel / 8;

    uint8_t* lcd_ptr=frambuff_base;
    // uint32_t *lcd_ptr = (uint32_t*)frambuff_base;
    uint8_t *img_ptr = image.auc_pixel_datas;

    for(int i = 0; i <image.height; i++) 
    {
        for(int j = 0; j < image.width; j++) 
        {
            *(lcd_ptr+j*4)=*(img_ptr+j*4);
            *(lcd_ptr+j*4+1)=*(img_ptr+j*4+1);
            *(lcd_ptr+j*4+2)=*(img_ptr+j*4+2);
            // memset(lcd_ptr+j,0,4);//rgb用3个字节表示一个像素点
            // memcpy(lcd_ptr+j,img_ptr+j*4,4);//rgb用3个字节表示一个像素点
        }
        lcd_ptr += (var.xres*4);
        img_ptr += (image.width*4);
    }

    // for (int y = 0; y < image.height; y++) 
    // {
    //     // 计算当前行在LCD显存中的目标地址
    //     uint8_t *lcd_line_ptr = (uint8_t*)(frambuff_base + (start_y + y) * line_width + start_x * pixel_width);
    //     // 拷贝一行图像数据
    //     memcpy(lcd_line_ptr, img_ptr + y * image.width, image.line_bytes);
    // }
    close(frambuff_fd);
    return kSuccess;
}



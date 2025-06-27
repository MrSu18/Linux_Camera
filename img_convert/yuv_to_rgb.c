#include "yuv_to_rgb.h"
#include <linux/videodev2.h>
#include <stdlib.h>
#include <stdio.h>

FunctionStatus isSupportYuv2Rgb (int pixel_format_in, int pixel_format_out)
{
    //支持yuv格式转换成rgb565或者rgb32
    if(pixel_format_in == V4L2_PIX_FMT_YUYV && (pixel_format_out == V4L2_PIX_FMT_RGB565 || pixel_format_out == V4L2_PIX_FMT_RGB32))
    {
        return kSuccess;
    }
    return kERROR;
}

FunctionStatus Yuv422ToRgb565(uint8_t* image_in, uint8_t* image_out, uint32_t width,uint32_t height)
{
    uint8_t Y=0,Y1=0,U=0,V=0;
    uint8_t* buff=image_in;
    uint8_t* out_buff=image_out;
    uint32_t r=0, g=0, b=0;
    uint32_t color=0;
    //YUV422的存储格式是Y0 U0 Y1 V0这样排列的,Y0 Y1共用U0 V0,Y代表亮度UV代表色度,所以它两个像素会共用UV,所以一次遍历是遍历了2个像素
    uint32_t size = width*height/2;
    //遍历图像,一个循环走两个像素,一个像素16bit
    for (uint32_t i = 0; i < size; i++)
    {
        // 每次处理2个像素(Y0和Y1)
        Y = buff[0];  // 第一个像素的Y分量
        U = buff[1];  // 共享的U分量
        Y1 = buff[2]; // 第二个像素的Y分量
        V = buff[3];  // 共享的V分量
        buff += 4;    // 移动指针到下一组数据

        //YUV->RGB 调用color中的宏定义来完成YUV到RGB的转换公式
        r = R_FROMYV(Y,V);
		g = G_FROMYUV(Y,U,V); //b
		b = B_FROMYU(Y,U); //v

        //RGB->rgb565 把RGB三色构造成RGB565的16位值
        r = r >> 3;  // 8位R→5位(取高5位)
        g = g >> 2;  // 8位G→6位(取高6位)
        b = b >> 3;  // 8位B→5位(取高5位)
        color = (r << 11) | (g << 5) | b;  // 组合成16位RGB565
        *out_buff++ = color & 0xff; //取color的低8位
        *out_buff++ = (color >> 8) & 0xff;//取color的高8位

        r = R_FROMYV(Y1,V);
		g = G_FROMYUV(Y1,U,V); //b
		b = B_FROMYU(Y1,U); //v

        r = r >> 3;
        g = g >> 2;
        b = b >> 3;
        color = (r << 11) | (g << 5) | b;
        *out_buff++ = color & 0xff;
        *out_buff++ = (color >> 8) & 0xff;
    }
    return kSuccess;
}

FunctionStatus Yuv422ToRgb32(uint8_t* image_in, uint8_t* image_out, uint32_t width,uint32_t height)
{
	uint8_t Y=0,Y1=0,U=0,V=0;
    uint8_t* buff=image_in;
    uint32_t* out_buff=(uint32_t*)image_out;
    uint32_t r=0, g=0, b=0;
    uint32_t color=0;
    //YUV422的存储格式是Y0 U0 Y1 V0这样排列的,Y0 Y1共用U0 V0,Y代表亮度UV代表色度,所以它两个像素会共用UV,所以一次遍历是遍历了2个像素
    uint32_t size = width*height/2;
    //遍历图像,一个循环走两个像素,一个像素16bit
    for (uint32_t i = 0; i < size; i++)
    {
		Y = buff[0] ;
		U = buff[1] ;
		Y1 = buff[2];
		V = buff[3];
		buff += 4;

        r = R_FROMYV(Y,V);
		g = G_FROMYUV(Y,U,V); //b
		b = B_FROMYU(Y,U); //v
		/* rgb888 */
		color = (r << 16) | (g << 8) | b;
        *out_buff++ = color;
			
		r = R_FROMYV(Y1,V);
		g = G_FROMYUV(Y1,U,V); //b
		b = B_FROMYU(Y1,U); //v
		color = (r << 16) | (g << 8) | b;
        *out_buff++ = color;
	}
	
	return kSuccess;
} 

FunctionStatus Yuv2RgbConvert (CameraBufPtr img_in, CameraBufPtr img_out)
{
    img_out->width=img_in->width;
    img_out->height=img_in->height;
    switch (img_out->pixel_format)
    {
    case V4L2_PIX_FMT_RGB565:
        img_out->bpp=16;//每个像素的位数bits per pixel
        img_out->line_bytes=img_out->width*(img_out->bpp/8);//宽度*每个像素的字节数
        img_out->total_bytes=img_out->line_bytes*img_out->height;
        if (img_out->auc_pixel_datas==NULL)
        {
            img_out->auc_pixel_datas=malloc(img_out->total_bytes);
            if (img_out->auc_pixel_datas==NULL)
            {
                printf("error: convert malloc error!\r\n ");
                return kERROR;
            }
        }
        Yuv422ToRgb565(img_in->auc_pixel_datas,img_out->auc_pixel_datas,img_in->width,img_in->height);
        break;
    case V4L2_PIX_FMT_RGB32:
        img_out->bpp=32;//每个像素的位数bits per pixel
        img_out->line_bytes=img_out->width*(img_out->bpp/8);//宽度*每个像素的字节数
        img_out->total_bytes=img_out->line_bytes*img_out->height;
        if (img_out->auc_pixel_datas==NULL)
        {
            img_out->auc_pixel_datas=malloc(img_out->total_bytes);
            if (img_out->auc_pixel_datas==NULL)
            {
                printf("error: convert malloc error!\r\n ");
                return kERROR;
            }
        }
        Yuv422ToRgb32(img_in->auc_pixel_datas,img_out->auc_pixel_datas,img_in->width,img_in->height);
        break;
    
    default:
        break;
    }
    
    return kSuccess;
}

FunctionStatus Yuv2RgbConvertExit (CameraBufPtr img)
{
    if (img->auc_pixel_datas!=NULL)
    {
        free(img->auc_pixel_datas);
        img->auc_pixel_datas=NULL;
    }
    return kSuccess;
}


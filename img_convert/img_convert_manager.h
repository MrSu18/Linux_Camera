#ifndef IMG_CONVERT_MANAGER_H
#define IMG_CONVERT_MANAGER_H

#include <linux/videodev2.h>
#include "camera_manager.h"

typedef struct ImgConvert 
{
    char *name;
    FunctionStatus (*isSupport)(int pixel_format_in, int pixel_format_out);
    FunctionStatus (*Convert)(CameraBufPtr img_in, CameraBufPtr img_out);
    FunctionStatus (*ConvertExit)(CameraBufPtr img);
    struct ImgConvert *next;
}ImgConvert, *ImgConvertPtr;

typedef struct ImgCvtLHead //操作集链表表头
{
    ImgConvertPtr next;
    uint8_t list_length;//链表长度
}ImgCvtLHead,*ImgCvtLHeadPtr;

FunctionStatus ImgConvertInit(void);//初始化操作集链表
FunctionStatus RegisterImgConvert(ImgConvert in_img_convert,const char*name,ImgConvertPtr *out_img_convert);//遇到新的相机设备需要一套新的或者不同的驱动函数时,将此操作节点插入到全局操作集中
int GetImgConvertNode(ImgCvtLHeadPtr L, const char* name,ImgConvertPtr *node);//查找元素是否在链表中
FunctionStatus GetVideoConvertForFormats(int pixel_format_in, int pixel_format_out, ImgConvertPtr *node);//遍历链表返回支持格式转换的操作集通过node返回

#endif

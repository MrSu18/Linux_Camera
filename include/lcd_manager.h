#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include "camera_manager.h"

struct LcdDisplay;
typedef struct LcdDisplay LcdDisplay,*LcdDisplayPtr;

typedef struct LcdDevice
{
    int fd;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
    uint32_t screen_size;
    uint8_t* framebuffer;
    LcdDisplayPtr opr;
}LcdDevice,*LcdDevicePtr;

typedef struct LcdDisplay
{
    uint8_t* name;
    FunctionStatus (*InitDevice)(const char *device_path, LcdDevicePtr device);
    FunctionStatus (*ExitDevice)(LcdDevicePtr device);
    FunctionStatus (*ShowPixel)(uint32_t x,uint32_t y,uint32_t color);
    FunctionStatus (*CleanScreen)(uint32_t color);
    FunctionStatus (*ShowPage)(CameraBuf data,LcdDevice device);
    struct LcdDisplay* next;
}LcdDisplay,*LcdDisplayPtr;

typedef struct LcdDisLHead //操作集链表表头
{
    LcdDisplayPtr next;
    uint8_t list_length;//链表长度
}LcdDisLHead,*LcdDisLHeadPtr;

extern LcdDisLHeadPtr lcd_display_head;//摄像头操作集的表头
extern LcdDevice lcd;//LCD

FunctionStatus LcdDisplayInit(const char *device_path);
FunctionStatus RegisterLcdDisplay(LcdDisplay in_node,const char*name,LcdDisplayPtr *out_node);//遇到新的相机设备需要一套新的或者不同的驱动函数时,将此操作节点插入到全局操作集中
int GetLcdDisplayNode(LcdDisLHeadPtr L, const char* name,LcdDisplayPtr *node);//查找元素是否在链表中

#endif

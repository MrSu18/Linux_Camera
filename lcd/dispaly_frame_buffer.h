#include "camera_manager.h"

extern uint8_t* frambuff_base;
extern struct fb_var_screeninfo var;
extern int frambuff_fd;

FunctionStatus LCDInit(const char* device);
FunctionStatus LCDShowPixel(uint32_t x,uint32_t y,uint32_t color);
FunctionStatus LCDShowPage(CameraBuf image, int start_x, int start_y);


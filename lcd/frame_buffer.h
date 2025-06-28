#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include "lcd_manager.h"

FunctionStatus FrameBufferInitDevice(const char *device_path, LcdDevicePtr device);
FunctionStatus FrameBufferExitDevice(LcdDevicePtr device);
FunctionStatus FrameBufferShowPixel(uint32_t x,uint32_t y,uint32_t color);
FunctionStatus FrameBufferCleanScreen(uint32_t color);
FunctionStatus FrameBufferShowPage(CameraBuf data,LcdDevice device);

#endif

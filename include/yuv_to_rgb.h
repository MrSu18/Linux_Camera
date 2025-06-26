#ifndef YUV_TO_RGB_H
#define YUV_TO_RGB_H

#include "su_typedef.h"
#include "camera_manager.h"

FunctionStatus isSupportYuv2Rgb (int pixel_format_in, int pixel_format_out);
FunctionStatus Yuv2RgbConvert (CameraBufPtr img_in, CameraBufPtr img_out);
FunctionStatus Yuv2RgbConvertExit (CameraBufPtr img);

#endif

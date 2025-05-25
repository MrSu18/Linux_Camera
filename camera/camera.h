#ifndef SU_CAMERA_H
#define SU_CAMERA_H

#include <sys/types.h>

typedef enum CameraErrors //相机错误码枚举
{
    kOk = 0,                // 成功
    kErrorOpen,             // 打开设备失败
    kErrorCapability,       // 设备能力不支持
    kErrorFormat,           // 格式设置失败
    kErrorReq,              //缓冲区申请失败
    kErrorInvalidArgument,  // 参数无效
}CameraError;

typedef struct CameraConfig// 配置参数结构体
{
    int width;          // 分辨率宽（默认640）
    int height;         // 分辨率高（默认480）
    u_int32_t pixel_format; // 像素格式（默认V4L2_PIX_FMT_MJPEG）
    int buffer_count;   // 缓冲区数量（默认4）
} CameraConfig;

CameraError CameraInit(const char* camera_path, const CameraConfig* config);//摄像头初始化

#endif

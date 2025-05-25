#ifndef SU_CAMERA_H
#define SU_CAMERA_H

#include <sys/types.h>
#include <linux/videodev2.h>

typedef enum CameraErrors //相机错误码枚举
{
    kOk = 0,                // 成功
    kErrorOpen,             // 打开设备失败
    kErrorCapability,       // 设备能力不支持
    kErrorFormat,           // 格式设置失败
    kErrorReq,              // 缓冲区申请失败
    kErrorStreamOn,         // 数据流启动失败
    kErrorInvalidArgument,  // 参数无效
    kErrorMemory,           // 内存不足malloc失败
    kErrorMap,              // 内存映射失败
    kErrorQueue,            // 缓冲区加入队列失败
    kErrorBuffer,           // 内存映射失败
    kErrorDQBuf,            // 
    kErrorFile,             
    kErrorQBuf,             
}CameraError;

typedef struct CameraConfig// 配置参数结构体
{
    int width;          // 分辨率宽（默认640）
    int height;         // 分辨率高（默认480）
    u_int32_t pixel_format; // 像素格式（默认V4L2_PIX_FMT_MJPEG）
    int buffer_count;   // 缓冲区数量（默认4）
} CameraConfig;

typedef struct CameraDevice    //摄像头设备结构体
{
    int fd;                    // 设备文件描述符
    int width;                 // 分辨率宽（默认640）
    int height;                // 分辨率高（默认480）
    u_int32_t pixel_format;    // 像素格式（默认V4L2_PIX_FMT_MJPEG）
    struct v4l2_buffer *bufs;  // 缓冲区信息数组
    void **mmap_buffers;       // 内存映射指针数组
    u_int32_t buf_count;        // 缓冲区数量
}CameraDevice;

CameraError CameraInit(const char* camera_path, const CameraConfig* config, CameraDevice *out_dev);//摄像头初始化
CameraError CameraStartCapture(int device_fd);//开始采集启动视频流
CameraError CameraCaptureFrame(CameraDevice *dev, const char *output_path); //采集图片

#endif

#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "su_typedef.h"

#define BUFFER_NUM 4 //申请缓冲区的个数

//因为在cameradevice结构液体中有用到opr的结构体当时那时候还没声明,所以在前面提前声明一下
struct CameraOperation;
typedef struct CameraOperation T_CameraOperation,*PT_CameraOperation;

//只关注于硬件层（如设备文件、分辨率、原始缓冲区）
typedef struct CameraDevice //相机设备(管理硬件)
{
    int fd;                  //文件描述符
    uint32_t width;               // 分辨率宽（默认640）
    uint32_t height;              // 分辨率高（默认480）
    uint32_t pixel_format;  // 像素格式（默认V4L2_PIX_FMT_MJPEG）

    uint32_t buf_count;     // 缓冲区数量
    uint32_t buf_maxlen;    //记录mmap时候的长度,用于umap
    uint32_t buf_cur_index;//当前的缓冲区索引值
    uint8_t  *mmap_buffers[BUFFER_NUM];//内存映射指针数组
    //函数
    PT_CameraOperation pt_opr;//指向对应的操作表
}T_CameraDevice,*PT_CameraDevice;

//只关注数据层(如像素格式转换,图像处理)
typedef struct CameraBuf //相机帧数据(管理图像处理)
{
    uint32_t width;   /* 宽度: 一行有多少个象素 */
	uint32_t height;  /* 高度: 一列有多少个象素 */
	uint32_t bpp;     /* 一个象素用多少位来表示 */
	uint32_t line_bytes;  /* 一行数据有多少字节 */
	uint32_t total_bytes; /* 所有字节数 */ 
	uint8_t  *auc_pixel_datas;  /* 象素数据存储的地方 */
    uint32_t pixel_format; //像素格式
}T_CameraBuf,*PT_CameraBuf;

typedef struct CameraOperation //摄像头操作集
{
    char *name;
    FunctionStatus (*InitDevice)(const char *camera_path, PT_CameraDevice camera_device);
    FunctionStatus (*ExitDevice)(PT_CameraDevice camera_device);
    FunctionStatus (*GetFrame)(PT_CameraDevice camera_device, PT_CameraBuf camera_buf);
    FunctionStatus (*GetFormat)(PT_CameraDevice camera_device);
    FunctionStatus (*PutFrame)(PT_CameraDevice camera_device, PT_CameraBuf camera_buf);
    FunctionStatus (*StartDevice)(PT_CameraDevice camera_device);
    FunctionStatus (*StopDevice)(PT_CameraDevice camera_device);
    struct CameraOperation *pt_next;
}T_CameraOperation,*PT_CameraOperation;

typedef struct CamOprLHead //操作集链表表头
{
    PT_CameraOperation next;
    uint8_t list_length;//链表长度
}CamOprLHead,*CamOprLHeadPtr;

extern CamOprLHeadPtr camera_opr_head;//摄像头操作集的表头
extern T_CameraDevice camera_main_usb;//主usb摄像头
extern T_CameraBuf camera_usb_buf;//USB摄像头的BUF

FunctionStatus RegisterCameraOpr(T_CameraOperation in_camera_opr,const char*name,PT_CameraOperation *out_camera_opr);//遇到新的相机设备需要一套新的或者不同的驱动函数时,将此操作节点插入到全局操作集中
FunctionStatus CameraInit(const char *camera_path);//初始化操作集链表,并且完成设备的初始化

#endif

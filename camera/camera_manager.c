#include "camera_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

CamOprLHeadPtr camera_opr_head=NULL;//摄像头操作集的表头
CameraDevice camera_main_usb;//主usb摄像头
CameraBuf camera_usb_buf;//USB摄像头的BUF
extern CameraOperation v4l2_camera_opration;//v4l2.c里面的全局变量,v4l2摄像头的操作表

/***************************************************************************
 * @brief  : 注册摄像头操作集到全局链表
 * @param  : in_camera_opr   : 待注册的操作集结构体（值拷贝）
 *           name            : 操作集唯一标识名
 *           out_camera_opr  : 输出参数，返回最终生效的操作集指针（可能是已存在的或新建的）,如果不需要可填NULL
 * @return : FunctionStatus  : kSuccess表示成功，kERROR表示失败
 * @note   : 1. 会自动检查重名，避免重复注册
 *           2. 调用者需保证name的唯一性
 *           3. 内存由链表管理，调用Exit函数时需统一释放
 * @date   : 2025.6.25
 * @author : sushizhou
 ***************************************************************************/
FunctionStatus RegisterCameraOpr(CameraOperation in_camera_opr,const char*name,CameraOperationPtr *out_camera_opr)
{
    //1. 判断是否为空表
    if (camera_opr_head==NULL)
    {
        printf("error: camera_opr_list isn't initialized!");
        return kERROR;
    }
    
    //2. 先查看操作集中是否有需要注册的操作函数,如果有就不需要了
    CameraOperationPtr temp_ptr;
    int ret = GetCameraOprNode(camera_opr_head,name,&temp_ptr);
    switch (ret)
    {
    case 0:   //若链表中有这个元素
        *out_camera_opr=temp_ptr;
        return kSuccess;
        break;
    case 2:
        temp_ptr=NULL;
        break;
    case -1:
        printf("error: search list error!\r\n");
        return kERROR;
    default:
        break;
    }

    //3. 创建新节点
    CameraOperationPtr new_node =malloc(sizeof(CameraOperation));
    if (new_node == NULL) 
    {
        printf("Error: Memory allocation failed for camera operation!\n");
        return kERROR;
    }
    
    // 4.拷贝数据并插入链表尾部
    memcpy(new_node,&in_camera_opr,sizeof(CameraOperation));
    if (temp_ptr!=NULL) //这里要判断头节点的后面是否有数据,没数据的话插到头节点后面
    {
        temp_ptr->next=new_node;
    }
    else
    {
        camera_opr_head->next=new_node;
    }
    new_node->next=NULL;
    camera_opr_head->list_length++;
    
    //5. 返回新节点
    if (out_camera_opr!=NULL)
    {
        *out_camera_opr = new_node;
    }
    return kSuccess;
}

/***************************************************************************
* @brief : 进行设备初始化
* @param : const char* camera_path：设备路径
*          CameraDevicePtr camera_device：摄像头硬件参数,用于得到摄像头参数
* @return: FunctionStatus 错误码
* @date  : 2025.6.24
* @author: sushizhou
****************************************************************************/
FunctionStatus CameraDeviceInit(const char *camera_path,CameraDevicePtr camera_device)
{
    CameraOperationPtr temp_opr_node=camera_opr_head->next;
    while (temp_opr_node)//遍历操作集链表,当temp==null的时候会跳出
    {
        //如果找到了适合这个摄像头的操作集,那初始化就会成功,当初始化成功的适合就能为这个摄像头匹配到对应的操作集了
        if (temp_opr_node->InitDevice(camera_path,camera_device)==kSuccess)
        {
            return kSuccess;
        }
        
        temp_opr_node=temp_opr_node->next;
    }
    return kERROR;
}

/*****************************************************************************
 * @brief  初始化摄像头系统（操作集链表和设备）
 * @param  camera_path : 摄像头设备节点路径（如"/dev/video0"）
 * @return FunctionStatus : 返回kSuccess表示成功，kERROR表示失败
 * @note   执行流程：
 *          1. 初始化操作集链表头
 *          2. 注册预定义的操作集（如V4L2）
 *          3. 初始化指定路径的摄像头设备
 * @warning 
 *          - 需确保camera_path有效性
 *          - 该函数应在系统启动时调用一次
* @date  : 2025.6.25
* @author: sushizhou
 *****************************************************************************/
FunctionStatus CameraInit(const char *camera_path)
{
    //1. 操作集链表初始化
    camera_opr_head=malloc(sizeof(CamOprLHead));
    if (camera_opr_head==NULL)
    {
        printf("error: camera_opr_list init error!\r\n ");
        return kERROR;
    }
    else
    {
        camera_opr_head->list_length=0;
        camera_opr_head->next=NULL;
    }
    
    //2. 插入现有的操作集
    if(RegisterCameraOpr(v4l2_camera_opration,v4l2_camera_opration.name,NULL)!=kSuccess)
    {
        printf("error: camera_opr_list insert error!\r\n ");
        return kERROR;
    }

    //3. 完成设备初始化
    if(CameraDeviceInit(camera_path,&camera_main_usb)!=kSuccess)
    {
        printf("error: camera_device init error!\r\n ");
        return kERROR;
    }

    return kSuccess;
}

/***************************************************************************
* @brief : 查找元素是否在链表中
* @param : CamOprLHeadPtr L: 链表表头
*          const char*name：需要查找的元素
*          CameraOperationPtr *node: 若找到返回节点地址,否则返回NULL
* @return: -1: 链表未初始化 0:链表中有这个元素,返回这个元素节点 1:链表中没这个元素,并且链表的尾部通过节点返回 2:链表为空
* @date  : 2025.6.25
* @author: sushizhou
****************************************************************************/
int GetCameraOprNode(CamOprLHeadPtr L, const char* name, CameraOperationPtr *node)
{
    if (L==NULL)
    {
        printf("error: list isn't initlized!\r\n");
        return -1;
    }
    else if (L->next==NULL)
    {
        printf("info: list is empty");
        node = NULL;
        return 2;
    }
    CameraOperationPtr temp = L->next;
    for (uint8_t i = 0; i < L->list_length; i++)
    {
        if (strcmp(temp->name,name)==0)//有这个元素
        {
            *node=temp;
            return 0;
        }
        else if(temp->next==NULL)
        {
            *node=temp;
            return 1;
        }
        temp=temp->next;
    }

    return -1;//正常情况不会运行到这,上面那个循环就遍历完量表跳出了
}

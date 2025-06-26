#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "img_convert_manager.h"
#include "yuv_to_rgb.h"

ImgCvtLHeadPtr img_convert_head=NULL;//图像格式转换操作集的表头

/*****************************************************************************
 * @brief  初始化图像格式转换操作集链表
 * @param  void
 * @return FunctionStatus : 返回kSuccess表示成功，kERROR表示失败
 * @note   执行流程：
 *          1. 初始化操作集链表头
 *          2. 注册预定义的操作集（如V4L2）
 * @warning - 该函数应在系统启动时调用一次
* @date  : 2025.6.25
* @author: sushizhou
 *****************************************************************************/
FunctionStatus ImgConvertInit()
{
    //1. 操作集链表初始化
    img_convert_head=malloc(sizeof(ImgCvtLHead));
    if (img_convert_head==NULL)
    {
        printf("error: img_convert_list init error!\r\n ");
        return kERROR;
    }
    else
    {
        img_convert_head->list_length=0;
        img_convert_head->next=NULL;
    }
    
    //2. 插入现有的操作集

    ImgConvert Yuv2Rgb = 
    {
        .name        = "yuv2rgb",
        .isSupport   = isSupportYuv2Rgb,
        .Convert     = Yuv2RgbConvert,
        .ConvertExit = Yuv2RgbConvertExit,
    };

    if(RegisterImgConvert(Yuv2Rgb,Yuv2Rgb.name,NULL)!=kSuccess)
    {
        printf("error: img_convert_list insert error!\r\n ");
        return kERROR;
    }

    return kSuccess;
}

/***************************************************************************
 * @brief  : 注册图像格式转换操作集到全局链表
 * @param  : in_img_convert   : 待注册的操作集结构体（值拷贝）
 *           name            : 操作集唯一标识名
 *           out_img_convert  : 输出参数，返回最终生效的操作集指针（可能是已存在的或新建的）,如果不需要可填NULL
 * @return : FunctionStatus  : kSuccess表示成功，kERROR表示失败
 * @note   : 1. 会自动检查重名，避免重复注册
 *           2. 调用者需保证name的唯一性
 *           3. 内存由链表管理，调用Exit函数时需统一释放
 * @date   : 2025.6.26
 * @author : sushizhou
 ***************************************************************************/
FunctionStatus RegisterImgConvert(ImgConvert in_img_convert,const char*name,ImgConvertPtr *out_img_convert)
{
    //1. 判断是否为空表
    if (img_convert_head==NULL)
    {
        printf("error: img_convert_list isn't initialized!");
        return kERROR;
    }
    
    //2. 先查看操作集中是否有需要注册的操作函数,如果有就不需要了
    ImgConvertPtr temp_ptr;
    int ret = GetImgConvertNode(img_convert_head,name,&temp_ptr);
    switch (ret)
    {
    case 0:   //若链表中有这个元素
        *out_img_convert=temp_ptr;
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
    ImgConvertPtr new_node =malloc(sizeof(ImgConvert));
    if (new_node == NULL) 
    {
        printf("Error: Memory allocation failed for image convert!\n");
        return kERROR;
    }
    
    // 4.拷贝数据并插入链表尾部
    memcpy(new_node,&in_img_convert,sizeof(ImgConvert));
    if (temp_ptr!=NULL) //这里要判断头节点的后面是否有数据,没数据的话插到头节点后面
    {
        temp_ptr->next=new_node;
    }
    else
    {
        img_convert_head->next=new_node;
    }
    new_node->next=NULL;
    img_convert_head->list_length++;
    
    //5. 返回新节点
    if (out_img_convert!=NULL)
    {
        *out_img_convert = new_node;
    }
    return kSuccess;
}

/***************************************************************************
* @brief : 查找元素是否在链表中
* @param : ImgCvtLHeadPtr L: 链表表头
*          const char*name：需要查找的元素
*          ImgConvertPtr *node: 若找到返回节点地址,否则返回NULL
* @return: -1: 链表未初始化 0:链表中有这个元素,返回这个元素节点 1:链表中没这个元素,并且链表的尾部通过节点返回 2:链表为空
* @date  : 2025.6.26
* @author: sushizhou
****************************************************************************/
int GetImgConvertNode(ImgCvtLHeadPtr L, const char* name,ImgConvertPtr *node)
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
    ImgConvertPtr temp = L->next;
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


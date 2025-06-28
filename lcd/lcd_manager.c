#include "lcd_manager.h"
#include "frame_buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

LcdDisLHeadPtr lcd_display_head=NULL;//摄像头操作集的表头
LcdDevice lcd;//LCD

FunctionStatus LcdDeviceInit(const char *device_path,LcdDevicePtr device)
{
    LcdDisplayPtr temp_opr_node=lcd_display_head->next;
    while (temp_opr_node)//遍历操作集链表,当temp==null的时候会跳出
    {
        //如果找到了适合这个摄像头的操作集,那初始化就会成功,当初始化成功的适合就能为这个摄像头匹配到对应的操作集了
        if (temp_opr_node->InitDevice(device_path,device)==kSuccess)
        {
            return kSuccess;
        }
        
        temp_opr_node=temp_opr_node->next;
    }
    return kERROR;
}

FunctionStatus LcdDisplayInit(const char *device_path)
{
    //1. 操作集链表初始化
    lcd_display_head=malloc(sizeof(LcdDisLHead));
    if (lcd_display_head==NULL)
    {
        printf("error: lcd_display_list init error!\r\n ");
        return kERROR;
    }
    else
    {
        lcd_display_head->list_length=0;
        lcd_display_head->next=NULL;
    }
    
    //2. 插入现有的操作集
    LcdDisplay frame_buffer = 
    {
        .name       = "framebuffer",
       .InitDevice  = FrameBufferInitDevice,
       .ExitDevice  = FrameBufferExitDevice,
       .ShowPixel   = FrameBufferShowPixel,
       .CleanScreen = FrameBufferCleanScreen,
       .ShowPage    = FrameBufferShowPage,
    };

    if(RegisterLcdDisplay(frame_buffer,frame_buffer.name,NULL)!=kSuccess)
    {
        printf("error: lcd_display_list insert error!\r\n ");
        return kERROR;
    }

    //3. 完成设备初始化
    if(LcdDeviceInit(device_path,&lcd)!=kSuccess)
    {
        printf("error: lcd_device init error!\r\n ");
        return kERROR;
    }

    return kSuccess;
}
FunctionStatus RegisterLcdDisplay(LcdDisplay in_node,const char*name,LcdDisplayPtr *out_node)
{
    //1. 判断是否为空表
    if (lcd_display_head==NULL)
    {
        printf("error: lcd_display_list isn't initialized!");
        return kERROR;
    }
    
    //2. 先查看操作集中是否有需要注册的操作函数,如果有就不需要了
    LcdDisplayPtr temp_ptr;
    int ret = GetLcdDisplayNode(lcd_display_head,name,&temp_ptr);
    switch (ret)
    {
    case 0:   //若链表中有这个元素
        *out_node=temp_ptr;
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
    LcdDisplayPtr new_node =malloc(sizeof(LcdDisplay));
    if (new_node == NULL) 
    {
        printf("Error: Memory allocation failed for lcd display!\n");
        return kERROR;
    }
    
    // 4.拷贝数据并插入链表尾部
    memcpy(new_node,&in_node,sizeof(LcdDisplay));
    if (temp_ptr!=NULL) //这里要判断头节点的后面是否有数据,没数据的话插到头节点后面
    {
        temp_ptr->next=new_node;
    }
    else
    {
        lcd_display_head->next=new_node;
    }
    new_node->next=NULL;
    lcd_display_head->list_length++;
    
    //5. 返回新节点
    if (out_node!=NULL)
    {
        *out_node = new_node;
    }
    return kSuccess;
}
int GetLcdDisplayNode(LcdDisLHeadPtr L, const char* name,LcdDisplayPtr *node)
{
    if (L==NULL)
    {
        printf("error: list isn't initlized!\r\n");
        return -1;
    }
    else if (L->next==NULL)
    {
        printf("info: list is empty\r\n");
        node = NULL;
        return 2;
    }
    LcdDisplayPtr temp = L->next;
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
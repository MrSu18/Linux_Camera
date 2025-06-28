#ifndef SU_TYPEDEF_H
#define SU_TYPEDEF_H

// 数据类型声明
typedef signed char             int8_t;     //有符号8bits     
typedef signed short int        int16_t;    //有符号16bits         
typedef signed int              int32_t;    //有符号32bits                                              

typedef unsigned char           uint8_t;    //无符号8bits                                             
typedef unsigned short int      uint16_t;   //无符号16bits                                           
typedef unsigned int            uint32_t;   //无符号32bits  

typedef enum
{
    kSuccess=0,
    kERROR=1
}FunctionStatus;

#endif

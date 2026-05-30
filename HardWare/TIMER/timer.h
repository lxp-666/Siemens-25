/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Function.h
 * 作者: Jialei Zhao 
 * 平台: 2025CIMC IHD-V04
 * 版本: Jialei Zhao     2026/1/4     V0.01    original
************************************************************/
#ifndef __TIMER_H__
#define __TIMER_H__

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************* 宏定义 *************************/


/************************ 变量定义 ************************/
extern volatile uint16_t glfage_1ms;   // 仅声明，无初始值
extern volatile uint8_t glfage_5ms;
extern volatile uint8_t glfage_10ms;
extern volatile uint8_t glfage_20ms;
extern volatile uint8_t glfage_500ms;
extern volatile uint8_t glfage_1000ms;

/************************ 函数定义 ************************/

void my_timer_init(void);


#endif // !__TIMER_H__


/****************************End*****************************/

/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：usart0.h
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/3/10     V0.01    original
************************************************************/


#ifndef __USART_H
#define __USART_H

/************************* 头文件 *************************/

#include "HeaderFiles.h"

/************************ 变量定义 ************************/

#define USART_RX_BUF_SIZE    128    // 接收缓冲区大小（足够存所有命令）
extern uint8_t  usart_rx_buf[USART_RX_BUF_SIZE];  // 接收缓冲区
extern uint16_t usart_rx_len;                     // 接收数据长度
extern uint8_t  usart_rx_done;                    // 接收完成标志（1=收到完整命令）

/************************ 函数定义 ************************/

void USART0_Config(void);  							// 串口功能配置
void USART0_SendData(uint16_t *buf,uint16_t len);   // 发送数据
//void process_data(uint8_t data);					// 数据处理
		 				    
#endif


/****************************End*****************************/


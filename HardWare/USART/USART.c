/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：usart0.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/3/10     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include "USART.h"
#include "string.h"

/************************* 宏定义 *************************/

/************************ 变量定义 ************************/

uint8_t  usart_rx_buf[USART_RX_BUF_SIZE] = {0};  
uint16_t usart_rx_len = 0;                   
uint8_t  usart_rx_done = 0;                   
uint8_t  data_recv = 0; 

/************************ 函数定义 ************************/

int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART0, (uint8_t)ch);
    while(RESET == usart_flag_get(USART0, USART_FLAG_TBE));
    return ch;
}

/************************************************************ 
 * Function :       USART0_Config
 * Comment  :       用于初始化usart0 相关配置
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-10 V0.1 original
************************************************************/
void USART0_Config(void)
{
  rcu_periph_clock_enable(RCU_GPIOA);
  rcu_periph_clock_enable(RCU_USART0);

  /* PA9 先保持 GPIO 输出高电平 (SystemInit 已设好，这里确保不被意外覆盖) */
  gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_9);
  gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
  GPIO_BOP(GPIOA) = GPIO_PIN_9;

  /* RX 引脚 (PA10) 先配好 */
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);

  /* 在 PA9 仍然是 GPIO 输出高电平时，完成 UART 外设初始化 */
  usart_deinit(USART0);
  usart_word_length_set(USART0, USART_WL_8BIT);
  usart_stop_bit_set(USART0, USART_STB_1BIT);
  usart_parity_config(USART0, USART_PM_NONE);
  usart_baudrate_set(USART0, 115200U);
  usart_receive_config(USART0, USART_RECEIVE_ENABLE);
  usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
  usart_hardware_flow_rts_config(USART0, USART_RTS_DISABLE);
  usart_hardware_flow_cts_config(USART0, USART_CTS_DISABLE);
  usart_enable(USART0);

  /* UART 外设已就绪，此时才把 PA9 切换到 AF 模式(UART TX) */
  gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9 | GPIO_PIN_10);
  gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
  gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

  /* 中断配置 */
  nvic_irq_enable(USART0_IRQn, 1, 0);
  usart_interrupt_enable(USART0, USART_INT_RBNE);
  while(RESET == usart_flag_get(USART0, USART_FLAG_TC));
  usart_flag_clear(USART0, USART_FLAG_TC);
}


/************************************************************ 
 * Function :       USART0_SendData
 * Comment  :       用usart0发送数据
 * Parameter:       数据指针，数据长度
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-14 V0.2 original
************************************************************/
void USART0_SendData(uint16_t *buf,uint16_t len)
 {
     uint16_t t;
     for(t=0;t<len;t++)      
     {           
         while(usart_flag_get(USART0, USART_FLAG_TC) == RESET);  
         usart_data_transmit(USART0,buf[t]);
     }     
     while(usart_flag_get(USART0, USART_FLAG_TC) == RESET);          
}

/************************************************************ 
 * Function :       USART0_IRQHandler
 * Comment  :       串口中断服务函数，用来接收串口数据
 * Parameter:       null
 * Return   :       null
 * Author   :       Lingyu Meng
 * Date     :       2025-03-14 V0.2 original
************************************************************/
void USART0_IRQHandler(void)
{
    if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE))
    {
        data_recv = usart_data_receive(USART0);
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_RBNE);

        if(usart_rx_done == 0) 
        {
            if(data_recv == '\r' || data_recv == '\n')
            {
                if(usart_rx_len > 0)
                {
                    usart_rx_buf[usart_rx_len] = '\0'; 
                    usart_rx_done = 1;                 
                }
            }
            else if(usart_rx_len < USART_RX_BUF_SIZE - 1)
            {
                usart_rx_buf[usart_rx_len++] = data_recv;
            }
        }
    }
}

///************************************************************ 
// * Function :       process_data
// * Comment  :       数据为 ‘a’的时候点亮LED2
//					数据为'b'的时候关断LED2
//					其他数据则直接串口打印出来
// * Parameter:       char型的数据，只支持单字符
// * Return   :       null
// * Author   :       Lingyu Meng
// * Date     :       2025-03-14 V0.2 original
//************************************************************/
//void process_data(uint8_t data)
//{
//	if (data == 'a')
//	{
//		LED2_ON();
//	}
//	else if(data == 'b')
//	{
//		LED2_OFF();
//	}
//	else
//	{
//		usart_data_transmit(USART0, data_recv);    //  发送数据 
//	}

//}
/************************************************************
 * 版权：2025CIMC Copyright。 
 * 文件：Function.c
 * 作者: Lingyu Meng
 * 平台: 2025CIMC IHD-V04
 * 版本: Lingyu Meng     2025/2/16     V0.01    original
************************************************************/


/************************* 头文件 *************************/

#include <string.h>
#include "Function.h"
#include "LED.h"
#include "USART.h"
#include "SPI_FLASH.h"
#include "RTC.h"
#include "ff.h"			/* FatFs configurations and declarations */
#include "diskio.h"		/* Declarations of low level disk I/O functions */
#include "sdcard.h"
#include "oled.h"

/************************* 宏定义 *************************/
#define  SFLASH_ID                     0x123456789
#define BUFFER_SIZE                    256
#define TX_BUFFER_SIZE                 BUFFER_SIZE
#define RX_BUFFER_SIZE                 BUFFER_SIZE
#define  FLASH_BASE_ADDR               0x000000
#define  FLASH_RATIO_ADDR              (FLASH_BASE_ADDR)           /* ratio 16 bytes, offset 0  */
#define  FLASH_LIMIT_ADDR              (FLASH_BASE_ADDR + 0x10)    /* limit 16 bytes, offset 16 */

/************************ 变量定义 ************************/
uint32_t flash_id = 0;
uint8_t  tx_buffer[TX_BUFFER_SIZE];
uint8_t  rx_buffer[TX_BUFFER_SIZE];
uint16_t i = 0;
uint8_t  is_successful = 0;

uint8_t rtc_wait_input = 0;

FATFS fs;

/************************ 函数定义 ************************/
ErrStatus memory_compare(uint8_t* src,uint8_t* dst,uint16_t length);

typedef void (*CmdFunction)(void);
typedef struct {
    char *cmd_str;       
    CmdFunction func;    
} Cmd_TypeDef;

void Cmd_Parse(void);
void Cmd_RTC_Config(void);
void Cmd_RTC_now(void);
void Cmd_test(void);
void Cmd_conf(void);
void Cmd_ratio(void);
void Cmd_limit(void);
void Cmd_config_read(void);
void Cmd_config_save(void);
void Cmd_start(void);
void Cmd_stop(void);

const Cmd_TypeDef Cmd_Table[] = {
    {"RTC Config",  	Cmd_RTC_Config},
	{"RTC now",   		Cmd_RTC_now},
	{"test",   			Cmd_test},
	{"conf",   			Cmd_conf},
	{"limit",   		Cmd_limit},
	{"ratio",   		Cmd_ratio},
	{"config save",   	Cmd_config_save},
	{"config read",   	Cmd_config_read},
	{"start",   		Cmd_start},
	{"stop",   			Cmd_stop},

};

//十进制转为BCD
uint8_t BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}


/************************ 主要函数 ************************/

void System_Init(void)
{
	systick_config();     // 时钟配置
	
	LED_Init();			//LED初始化
	USART0_Config();	//USART初始化
	RTC_Init();			//RTC初始化
	spi_flash_init();	//FLASH初始化
	OLED_Init();		//OLED初始化
	
	f_mount(0, &fs);
}

void UsrFunction(void)
{
	usart_rx_len = 0;
	usart_rx_done = 0;
	memset(usart_rx_buf, 0, sizeof(usart_rx_buf));
	
	/////////////////////清空flash///////////////////////////////////
	char empty[16] = "";
	spi_flash_sector_erase(FLASH_BASE_ADDR);
	spi_flash_buffer_write((uint8_t *)empty, FLASH_RATIO_ADDR, 16);
	spi_flash_buffer_write((uint8_t *)empty, FLASH_LIMIT_ADDR, 16);
	
	///////////// 上电串口同步：发送 BREAK 清除接收端噪声 ////////////////////
	// 上电瞬间 USB转串口芯片可能因TX引脚电平跳变收到乱码，
	// 主动发送一个 BREAK (10+ bit时间的低电平) 可使接收端状态机复位
	usart_enable(USART0);                                       // 确保 UART 已使能
	usart_transmit_config(USART0, USART_TRANSMIT_DISABLE);      // 暂时关闭UART发送
	gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO_PIN_9);
	GPIO_BOP(GPIOA) = GPIO_PIN_9;                               // 先输出高
	delay_1ms(2);                                               // 稳定2ms
	gpio_bit_reset(GPIOA, GPIO_PIN_9);                          // 拉低 TX (BREAK 开始)
	delay_1ms(10);                                              // 保持低电平10ms (>1字符帧)
	gpio_bit_set(GPIOA, GPIO_PIN_9);                            // 拉高 TX (BREAK 结束)
	delay_1ms(5);                                               // 保持高电平稳定5ms
	gpio_af_set(GPIOA, GPIO_AF_7, GPIO_PIN_9);                  // 切回 UART TX 功能
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
	usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);       // 重新使能UART发送
	
	/////////////系统上电初始化测试//////////////////
	printf("====system selftest====\r\n");
    flash_id = spi_flash_read_id();
	printf("\n\rDevice_ID:2025-CIMC-%X\n\r",flash_id);
	printf("\n\r====system ready====\r\n");
	OLED_ShowString(0,0,"system idle",16);
	OLED_Refresh();
	while(1)
	{
		Cmd_Parse();
		
		
		
		
	
	}
}

void Cmd_RTC_Config(void)
  {
      extern rtc_parameter_struct rtc_initpara;
      extern __IO uint32_t prescaler_a;
      extern __IO uint32_t prescaler_s;

      int year, month, day, hour, minute, second;

      printf("\r\nInput Datetime\r\n");

      usart_rx_done = 0;
      usart_rx_len  = 0;
      memset(usart_rx_buf, 0, USART_RX_BUF_SIZE);
      while (usart_rx_done == 0);

      if (sscanf((char *)usart_rx_buf, "%d-%d-%d %d:%d:%d",
                 &year, &month, &day, &hour, &minute, &second) != 6)
      {
          printf("\r\nFormat error! Use: YYYY MM DD HH:MM:SS\r\n");
          printf("Example: 2025 01 01 12:00:30\r\n");
          return;
      }

      if (year < 2000 || year > 2099 ||
          month < 1  || month > 12   ||
          day < 1    || day > 31     ||
          hour > 23  || minute > 59  || second > 59)
      {
          printf("\r\nValue out of range!\r\n");
          return;
      }

      rtc_initpara.year           = BCD(year % 100);
      rtc_initpara.month          = BCD(month);
      rtc_initpara.date           = BCD(day);
      rtc_initpara.hour           = BCD(hour);
      rtc_initpara.minute         = BCD(minute);
      rtc_initpara.second         = BCD(second);
      rtc_initpara.display_format = RTC_24HOUR;
      rtc_initpara.am_pm          = RTC_AM;
      rtc_initpara.day_of_week    = RTC_SATURDAY;
      rtc_initpara.factor_asyn    = prescaler_a;
      rtc_initpara.factor_syn     = prescaler_s;

      if (ERROR == rtc_init(&rtc_initpara))
      {
          printf("\r\nRTC Config failed\r\n");
      }
      else
      {
          printf("\r\nRTC Config success\r\n");
          rtc_show_time();
          RTC_BKP0 = 0x32F0;     
      }
  }

void Cmd_RTC_now(void)
{
	rtc_current_time_get(&rtc_initpara);
    printf("\r\nCurrent Time: 20%0.2x-%0.2x-%0.2x", \
           rtc_initpara.year, rtc_initpara.month, rtc_initpara.date);

    printf(" : %0.2x:%0.2x:%0.2x \r\n", \
           rtc_initpara.hour, rtc_initpara.minute, rtc_initpara.second);
}

void Cmd_test(void)
  {
      DWORD fre_clust;
      FRESULT res;

      /* Flash ID */
      flash_id = spi_flash_read_id();
      printf("flash..........ok");
      printf("\r\nFlash ID: 0x%06X\r\n", flash_id);

      /* TF Card Memory */
      f_mount(0, NULL);                       // 先完全卸载
      res = f_mount(0, &fs);                  // 重新注册工作区
      if (res == FR_OK) {
          res = f_getfree("0:", &fre_clust, NULL);
      }

      /* 冷启动首次访问偶发失败，重试一次 */
      if (res != FR_OK) {
          delay_1ms(200);
          f_mount(0, NULL);
          f_mount(0, &fs);
          res = f_getfree("0:", &fre_clust, NULL);
      }

      if (res == FR_OK)
      {
          DWORD tot_sect = (fs.n_fatent - 2) * fs.csize;
          printf("TF card..........ok\r\n");
          printf("TF card memory: %lu KB\r\n", tot_sect / 2);
      }
      else
      {
          printf("TF card..........error (code=%d)\r\n", res);
      }
  }



void Cmd_conf(void)
{
	FIL fil;
	UINT bytes_read;
	char buf[128], radio_val[16] = "", limit_val[16] = "";
	char *p;

	if (f_open(&fil, "0:/config.ini", FA_READ) != FR_OK)
	{
	  printf("\r\nconfig.ini file not found\r\n");
	  return;
	}

	if (f_read(&fil, buf, sizeof(buf) - 1, &bytes_read) != FR_OK || bytes_read == 0)
	{
	  f_close(&fil);
	  return;
	}
	f_close(&fil);
	buf[bytes_read] = '\0';

	/* 定位 [Ratio] -> Ch0 = 值 */
	p = strstr(buf, "[Ratio]");
	if (p && (p = strstr(p, "Ch0")) && (p = strchr(p, '=')))
	  sscanf(p + 1, "%15s", radio_val);

	/* 定位 [Limit] -> Ch0 = 值 */
	p = strstr(buf, "[Limit]");
	if (p && (p = strstr(p, "Ch0")) && (p = strchr(p, '=')))
	  sscanf(p + 1, "%15s", limit_val);

	printf("\r\nRadio=%s\r\n", radio_val);
	printf("\r\nlimit=%s\r\n", limit_val);
	printf("\r\nconfig read success\r\n");
	
	spi_flash_sector_erase(FLASH_BASE_ADDR);
	spi_flash_buffer_write((uint8_t *)radio_val, FLASH_RATIO_ADDR, 16);
	spi_flash_buffer_write((uint8_t *)limit_val, FLASH_LIMIT_ADDR, 16);
	
}
void Cmd_ratio(void)
{
	char ratio_str[16] = "";
	char limit_backup[16] = "";       /* 备份 limit，防擦除丢失 */
	float ratio = 0.0f;
	float original_ratio = 0.0f;

	/* 先读出当前 ratio */
	spi_flash_buffer_read((uint8_t *)ratio_str, FLASH_RATIO_ADDR, 16);
	ratio_str[15] = '\0';
	sscanf(ratio_str, "%f", &ratio);
	original_ratio = ratio;
	printf("\r\nRatio: %.3f\r\n", ratio);
	printf("Input value(0-100): \r\n");

	/* 等待用户输入 */
	usart_rx_done = 0;
	usart_rx_len  = 0;
	memset(usart_rx_buf, 0, USART_RX_BUF_SIZE);
	while (usart_rx_done == 0);

	if (sscanf((char *)usart_rx_buf, "%f", &ratio) != 1)
	{
	  return;
	}

	if (ratio < 0.0f || ratio > 100.0f)
	{
	  printf("\r\nratio invalid\r\n");
	  printf("Ratio=%.3f\r\n", original_ratio);
	  return;
	}

	/* ★ 擦除前：先把 limit 数据备份出来 ★ */
	spi_flash_buffer_read((uint8_t *)limit_backup, FLASH_LIMIT_ADDR, 16);

	/* 擦除扇区，然后同时写回 ratio 和 limit */
	sprintf(ratio_str, "%.3f", ratio);
	spi_flash_sector_erase(FLASH_BASE_ADDR);
	spi_flash_buffer_write((uint8_t *)ratio_str,    FLASH_RATIO_ADDR, 16);
	spi_flash_buffer_write((uint8_t *)limit_backup, FLASH_LIMIT_ADDR, 16);

	printf("\r\nratio modified success\r\n");
	printf("Ratio= %.3f\r\n", ratio);
}

void Cmd_limit(void)
{
	char limit_str[16] = "";
	char ratio_backup[16] = "";       /* 备份 ratio，防擦除丢失 */
	float limit = 0.0f;
	float original_limit = 0.0f;

	/* 先读出当前 limit */
	spi_flash_buffer_read((uint8_t *)limit_str, FLASH_LIMIT_ADDR, 16);
	limit_str[15] = '\0';
	sscanf(limit_str, "%f", &limit);
	original_limit = limit;
	printf("\r\nlimit= %.3f\r\n", limit);
	printf("Input value(0-500): \r\n");

	/* 等待用户输入 */
	usart_rx_done = 0;
	usart_rx_len  = 0;
	memset(usart_rx_buf, 0, USART_RX_BUF_SIZE);
	while (usart_rx_done == 0);

	if (sscanf((char *)usart_rx_buf, "%f", &limit) != 1)
	{
	  return;
	}

	if (limit < 0.0f || limit > 500.0f)
	{
	  printf("limit invalid\r\n");
	  printf("limit= %.3f\r\n", original_limit);
	  return;
	}

	/* ★ 擦除前：先把 ratio 数据备份出来 ★ */
	spi_flash_buffer_read((uint8_t *)ratio_backup, FLASH_RATIO_ADDR, 16);

	/* 擦除扇区，然后同时写回 ratio 和 limit */
	sprintf(limit_str, "%.3f", limit);
	spi_flash_sector_erase(FLASH_BASE_ADDR);
	spi_flash_buffer_write((uint8_t *)ratio_backup, FLASH_RATIO_ADDR, 16);
	spi_flash_buffer_write((uint8_t *)limit_str,    FLASH_LIMIT_ADDR, 16);

	printf("Limit= %.3f\r\n", limit);
}

void Cmd_config_read(void)
{
	char ratio_str[16] = "";
	char limit_str[16] = "";
	float ratio = 0.0f;
	float limit = 0.0f;

	/* 从 Flash 读取 ratio */
	spi_flash_buffer_read((uint8_t *)ratio_str, FLASH_RATIO_ADDR, 16);
	ratio_str[15] = '\0';
	sscanf(ratio_str, "%f", &ratio);

	/* 从 Flash 读取 limit */
	spi_flash_buffer_read((uint8_t *)limit_str, FLASH_LIMIT_ADDR, 16);
	limit_str[15] = '\0';
	sscanf(limit_str, "%f", &limit);

	/* 串口打印当前配置参数 */
	printf("\r\nread parameters from flash");
	printf("ratio: %.3f\r\n", ratio);
	printf("limit: %.3f\r\n", limit);
}

void Cmd_config_save(void)
{
	char ratio_str[16] = "";
	char limit_str[16] = "";
	float ratio = 0.0f;
	float limit = 0.0f;

	/* 从 Flash 读取 ratio */
	spi_flash_buffer_read((uint8_t *)ratio_str, FLASH_RATIO_ADDR, 16);
	ratio_str[15] = '\0';
	sscanf(ratio_str, "%f", &ratio);

	/* 从 Flash 读取 limit */
	spi_flash_buffer_read((uint8_t *)limit_str, FLASH_LIMIT_ADDR, 16);
	limit_str[15] = '\0';
	sscanf(limit_str, "%f", &limit);

	/* 串口打印 */
	printf("\r\nRatio: %.3f\r\n", ratio);
	printf("Limit: %.3f\r\n", limit);

	/* 存储到 Flash */
	sprintf(ratio_str, "%.3f", ratio);
	sprintf(limit_str, "%.3f", limit);
	spi_flash_sector_erase(FLASH_BASE_ADDR);
	spi_flash_buffer_write((uint8_t *)ratio_str, FLASH_RATIO_ADDR, 16);
	spi_flash_buffer_write((uint8_t *)limit_str, FLASH_LIMIT_ADDR, 16);

	printf("\r\nsave parameters to flash\r\n");

}


#define CMD_NUM  (sizeof(Cmd_Table)/sizeof(Cmd_TypeDef))  	
	
void Cmd_Parse(void)
{
    if(usart_rx_done == 0) return;

    uint8_t i;
    uint8_t cmd_found = 0;
    for(i=0; i<CMD_NUM; i++)
    {
        if(strcmp((char*)usart_rx_buf, Cmd_Table[i].cmd_str) == 0)
        {
            Cmd_Table[i].func();
            cmd_found = 1;
            break;
        }
    }

    if(!cmd_found)
    {
        printf("Unknown Command: %s\r\n", usart_rx_buf);
    }

    usart_rx_len = 0;
    usart_rx_done = 0;
    memset(usart_rx_buf, 0, USART_RX_BUF_SIZE);
}
/****************************End*****************************/


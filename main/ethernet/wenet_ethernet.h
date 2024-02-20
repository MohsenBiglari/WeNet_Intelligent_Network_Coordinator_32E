/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ETHERNET_H
#define __ETHERNET_H

/* Includes ------------------------------------------------------------------*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_eth.h"
/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
extern bool eth_connected;
extern QueueHandle_t eth_receive_queue;
extern esp_eth_handle_t eth_handle;
/* Exported functions prototypes ---------------------------------------------*/
void ethernet_init(void);
#endif /* __ETHERNET_H */

/************************ (C) COPYRIGHT WeNet *****END OF FILE****************/
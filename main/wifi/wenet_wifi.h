/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WIFI_H
#define __WIFI_H

/* Includes ------------------------------------------------------------------*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_private/wifi.h"
/* Exported macro ------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct raw_data
{
    void *data;
    void* eb;
    uint32_t data_length;
} raw_data_t;
/* Exported constants --------------------------------------------------------*/
extern bool ap_connected;

/* Exported functions prototypes ---------------------------------------------*/
void wifi_init(void);
void wifi_uninit(void);
void wifi_change_mode(wifi_mode_t new_mode);
void wifi_change_ssid_pass(char *new_ssid, char *new_pass);
void wifi_set_raw_mode(bool mode);
#endif /* __WIFI_H */

/************************ (C) COPYRIGHT WeNet *****END OF FILE****************/
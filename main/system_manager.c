/* Includes ------------------------------------------------------------------*/
#include "system_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wenet_ethernet.h"
#include "wenet_wifi.h"
#include "esp_private/wifi.h"
/* Private macro -------------------------------------------------------------*/
#define ETH_LOG "MANAGER"

/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/
/**
 * @brief Initialize the system manager
 * @param None
 * @retval
 */
void system_manager_init(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // // Initialize the ethernet module
    ethernet_init();

    // Initialize the wifi module
    wifi_init();
    wifi_change_ssid_pass("WeNet", "diplomatic10");
    wifi_set_raw_mode(true);

    // // Change the wifi mode to access point
    // wifi_change_mode(WIFI_MODE_AP);

    
    // Change the wifi mode to access point
    wifi_change_mode(WIFI_MODE_STA);
}

/************************ (C) COPYRIGHT WeNet *****END OF FILE****************/
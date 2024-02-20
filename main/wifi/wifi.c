/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "esp_log.h"
#include "wenet_wifi.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "esp_private/wifi.h"

/* Private macro -------------------------------------------------------------*/
#define WIFI_LOG "WIFI"

/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static wifi_mode_t current_wifi_mode = WIFI_MODE_NULL;
static char ssid[32];
static char password[64];
static bool raw_mode = false;
/* Private function prototypes -----------------------------------------------*/
static void wifi_start_AP(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void wifi_receive_raw_callback(void *buffer, uint16_t len, void *eb);
/* Private user code ---------------------------------------------------------*/
/**
 * @brief Initialize the wifi module
 * @param None
 * @retval
 */
void wifi_init(void)
{
    // TODO: Read the wifi last mode and last data
}

/**
 * @brief Uinitialize the wifi module
 * @param None
 * @retval
 */
void wifi_uninit(void)
{
    switch (current_wifi_mode)
    {
    case WIFI_MODE_NULL:
        break;
    case WIFI_MODE_STA:
        break;
    case WIFI_MODE_AP:
        break;
    default:
        break;
    }
}

void wifi_change_mode(wifi_mode_t new_mode)
{
    wifi_uninit();
    current_wifi_mode = new_mode;
    switch (current_wifi_mode)
    {
    case WIFI_MODE_NULL:
        break;
    case WIFI_MODE_STA:
        break;
    case WIFI_MODE_AP:
        wifi_start_AP();
        break;
    default:
        break;
    }
}

void wifi_change_ssid_pass(char *new_ssid, char *new_pass)
{
    memset(ssid, 0, sizeof(ssid));
    memset(password, 0, sizeof(password));
    strcpy(ssid, new_ssid);
    strcpy(password, new_pass);
}

void wifi_set_raw_mode(bool mode)
{
    raw_mode = mode;
}

static void wifi_start_AP(void)
{
    // Initialize wifi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // Initialize wifi AP configuration
    wifi_config_t wifi_configuration = {
        .ap = {
            .ssid = "",
            .ssid_len = 0,
            .password = "",
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = true,
            },
        },
    };
    strcpy((char *)wifi_configuration.ap.ssid, ssid);
    strcpy((char *)wifi_configuration.ap.password, password);
    wifi_configuration.ap.ssid_len = strlen(ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_configuration));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_AP_START:
        ESP_LOGI(WIFI_LOG, "Wifi has been started in SoftAP. SSID:%s password:%s", ssid, password);
        break;
    case WIFI_EVENT_AP_STOP:
        ESP_LOGI(WIFI_LOG, "Wifi softAP stopped");
        break;
    case WIFI_EVENT_AP_STACONNECTED:
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(WIFI_LOG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        if (raw_mode)
        {
            esp_wifi_internal_reg_rxcb(WIFI_IF_AP, wifi_receive_raw_callback);
        }
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(WIFI_LOG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        if (raw_mode)
        {
            esp_wifi_internal_reg_rxcb(WIFI_IF_AP, NULL);
        }
        break;
    }
    }
}

static void wifi_receive_raw_callback(void *buffer, uint16_t len, void *eb)
{
}

/************************ (C) COPYRIGHT WeNet *****END OF FILE****************/
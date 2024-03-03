/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"

#include "wenet_wifi.h"
#include "wenet_ethernet.h"

/* Private macro -------------------------------------------------------------*/
#define LOG_WIFI_TAG "WIFI"
/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t wifi_eth_addr[6] = {0x00, 0xE0, 0x4C, 0x68, 0x08, 0x7A};
uint8_t wifi_sta_addr[6];
uint8_t wifi_ap_addr[6];
static bool wifi_connected = false;
static TaskHandle_t wifi_send_raw_taskHandler;
static wifi_mode_t current_wifi_mode = WIFI_MODE_NULL;
static char ssid[32];
static char password[64];
static bool raw_mode = false;
/* Private function prototypes -----------------------------------------------*/
static void wifi_start_station(void);
static void wifi_start_AP(void);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static esp_err_t wifi_callback_receive_raw(void *buffer, uint16_t len, void *eb);
static void wifi_task_send_raw(void *pvParameters);
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
        if (raw_mode)
        {
            esp_wifi_internal_reg_rxcb(WIFI_IF_STA, NULL);
        }
        break;
    case WIFI_MODE_AP:
        if (raw_mode)
        {
            esp_wifi_internal_reg_rxcb(WIFI_IF_AP, NULL);
        }
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
        wifi_start_station();
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

static void wifi_start_station(void)
{
    // Initialize wifi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // esp_iface_mac_addr_set(wifi_global_addr, ESP_MAC_WIFI_STA);
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    // Initialize wifi AP configuration
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK},
    };
    strcpy((char *)wifi_configuration.ap.ssid, ssid);
    strcpy((char *)wifi_configuration.ap.password, password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_configuration));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_read_mac(wifi_sta_addr, ESP_MAC_WIFI_STA);
    ESP_LOGI(LOG_WIFI_TAG, "ESP32 wifi station addr->%02x:%02x:%02x:%02x:%02x:%02x", wifi_sta_addr[0], wifi_sta_addr[1], wifi_sta_addr[2], wifi_sta_addr[3], wifi_sta_addr[4], wifi_sta_addr[5]);
    if (raw_mode)
    {
        esp_wifi_internal_reg_rxcb(WIFI_IF_STA, wifi_callback_receive_raw);
        // Start a task to send raw data from ethernet buffer
        xTaskCreatePinnedToCore(wifi_task_send_raw, "wifi_send_raw_task", 2048, NULL, 0, &wifi_send_raw_taskHandler, tskNO_AFFINITY);
    }
    esp_wifi_connect();
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
            .beacon_interval = 100},
    };
    strcpy((char *)wifi_configuration.ap.ssid, ssid);
    strcpy((char *)wifi_configuration.ap.password, password);
    wifi_configuration.ap.ssid_len = strlen(ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_configuration));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_read_mac(wifi_ap_addr, ESP_MAC_WIFI_SOFTAP);
    ESP_LOGI(LOG_WIFI_TAG, "ESP32 wifi ap addr->%02x:%02x:%02x:%02x:%02x:%02x", wifi_ap_addr[0], wifi_ap_addr[1], wifi_ap_addr[2], wifi_ap_addr[3], wifi_ap_addr[4], wifi_ap_addr[5]);
    if (raw_mode)
    {
        esp_wifi_internal_reg_rxcb(WIFI_IF_AP, wifi_callback_receive_raw);
        // Start a task to send raw data from ethernet buffer
        xTaskCreatePinnedToCore(wifi_task_send_raw, "wifi_send_raw_task", 2048, NULL, 0, &wifi_send_raw_taskHandler, tskNO_AFFINITY);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    // WIFI Station events
    case WIFI_EVENT_STA_START:
        ESP_LOGI(LOG_WIFI_TAG, "Wifi has been started in station mode");
        break;
    case WIFI_EVENT_STA_STOP:
        ESP_LOGI(LOG_WIFI_TAG, "Wifi station stopped");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        wifi_ap_record_t ap_info;
        esp_wifi_sta_get_ap_info(&ap_info);
        memcpy(wifi_ap_addr, ap_info.bssid, 6);
        wifi_connected = true;
        ESP_LOGI(LOG_WIFI_TAG, "Wifi station has connected to %s mac->" MACSTR, ssid, MAC2STR(wifi_ap_addr));
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        wifi_connected = false;
        ESP_LOGI(LOG_WIFI_TAG, "Wifi station disconnected from %s", ssid);
        esp_wifi_connect();
        break;

    // WIFI Accesspoint events
    case WIFI_EVENT_AP_START:
        ESP_LOGI(LOG_WIFI_TAG, "Wifi has been started in softAP mode. SSID:%s password:%s", ssid, password);
        break;

    case WIFI_EVENT_AP_STOP:
        ESP_LOGI(LOG_WIFI_TAG, "Wifi softAP stopped");
        break;
    case WIFI_EVENT_AP_STACONNECTED:
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        memcpy(wifi_sta_addr, event->mac, 6);
        wifi_connected = true;
        ESP_LOGI(LOG_WIFI_TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED:
    {
        wifi_connected = false;
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(LOG_WIFI_TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    }
}

static esp_err_t wifi_callback_receive_raw(void *buffer, uint16_t len, void *eb)
{
    if (eth_connected)
    {
        memcpy(buffer, buffer + (len - 12), 12);
        esp_eth_transmit(eth_handle, buffer, len - 12);
    }
    esp_wifi_internal_free_rx_buffer(eb);
    return ESP_OK;
}

static void wifi_task_send_raw(void *pvParameters)
{
    if (eth_receive_queue != NULL)
    {
        xQueueReset(eth_receive_queue);
    }
    while (true)
    {
        raw_data_t *received_data;
        if (eth_receive_queue != NULL && xQueueReceive(eth_receive_queue, &received_data, portMAX_DELAY) == pdTRUE)
        {
            if (wifi_connected)
            {
                uint8_t *new_data = malloc((received_data->data_length) + 12);
                memcpy(new_data, received_data->data, received_data->data_length);
                free(received_data->data);
                memcpy(new_data + received_data->data_length, new_data, 12);
                switch (current_wifi_mode)
                {
                case WIFI_MODE_STA:
                    memcpy(new_data, wifi_ap_addr, 6);
                    memcpy(new_data + 6, wifi_sta_addr, 6);
                    esp_wifi_internal_tx(WIFI_IF_STA, new_data, received_data->data_length + 12);
                    free(new_data);
                    free(received_data);
                    break;
                case WIFI_MODE_AP:
                    memcpy(new_data, wifi_sta_addr, 6);
                    memcpy(new_data + 6, wifi_ap_addr, 6);
                    esp_wifi_internal_tx(WIFI_IF_AP, new_data, received_data->data_length + 12);
                    free(new_data);
                    free(received_data);
                    break;
                default:
                    break;
                }
            }
            else
            {
                free(received_data->data);
                free(received_data);
            }
        }
    }
}

/************************ (C) COPYRIGHT WeNet *****END OF FILE****************/
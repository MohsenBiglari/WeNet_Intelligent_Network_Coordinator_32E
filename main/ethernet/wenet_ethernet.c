/* Includes ------------------------------------------------------------------*/
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "esp_event.h"
#include "wenet_ethernet.h"
#include "wenet_wifi.h"
#include "esp_mac.h"
/* Private macro -------------------------------------------------------------*/
#define ETH_LOG "ETHERNET"
#define ETH_RECEIVE_QUEUE_SIZE 50
#define ETH_WAIT_TO_SEND_RAW_TIMOUT 100 // ms
#define ETH_PHY_RST_GPIO 33
#define ETH_MDC_GPIO 23
#define ETH_MDIO_GPIO 18
#define ETH_OCS_POWER_GPIO 32
/* Private typedef -----------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
QueueHandle_t eth_receive_queue = NULL;
bool eth_connected = false;
esp_eth_handle_t eth_handle;
uint8_t eth_esp_addr[6] = {0};
uint8_t eth_device_addr[6] = {0};
/* Private function prototypes -----------------------------------------------*/
static esp_err_t ethernet_input_path(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t len, void *priv);
static void ethernet_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static esp_err_t ethernet_init_LAN8720(esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out);
static void ethernet_uninit_LAN8720(void);
/* Private user code ---------------------------------------------------------*/
/**
 * @brief Initialize the ehternet module
 * @param None
 * @retval
 */
void ethernet_init(void)
{
    // Initialize oscillator power gpio.
    gpio_config_t oscillator_power = {};
    oscillator_power.intr_type = GPIO_INTR_DISABLE;
    oscillator_power.mode = GPIO_MODE_OUTPUT;
    oscillator_power.pin_bit_mask = (1ULL << ETH_OCS_POWER_GPIO);
    oscillator_power.pull_down_en = 0;
    oscillator_power.pull_up_en = 0;
    gpio_config(&oscillator_power);
    ESP_ERROR_CHECK(ethernet_init_LAN8720(NULL, NULL));
    // Log ethernet MAC address
    esp_read_mac(eth_esp_addr, ESP_MAC_ETH);
    ESP_LOGI(ETH_LOG, "ESP32 ethernet addr:%02x:%02x:%02x:%02x:%02x:%02x", eth_esp_addr[0], eth_esp_addr[1], eth_esp_addr[2], eth_esp_addr[3], eth_esp_addr[4], eth_esp_addr[5]);
    // Register ethernet events in default event loop
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &ethernet_event_handler, NULL));
    esp_eth_update_input_path(eth_handle, ethernet_input_path, NULL);
    // Enable prmiscuous mode to pass all traffic
    bool eth_promiscuous = true;
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_PROMISCUOUS, &eth_promiscuous));
    // Enable flow control mechanism
    bool flow_ctrl_enable = true;
    ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle, ETH_CMD_S_FLOW_CTRL, &flow_ctrl_enable));
    eth_receive_queue = xQueueCreate(ETH_RECEIVE_QUEUE_SIZE, sizeof(raw_data_t *));
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

/**
 * @brief Uinitialize the ehternet module
 * @param None
 * @retval
 */
void ethernet_uninit(void)
{
    ethernet_uninit_LAN8720();
}

/**
 * @brief Ethernet input path function. This function will be called when new data is received by ethernet mac
 * @param[in] eth_handle Ethernet handler
 * @param[in] buffer Input data
 * @param[in] len Input data length
 * @param[in] priv
 * @retval
 */
static esp_err_t ethernet_input_path(esp_eth_handle_t eth_handle, uint8_t *buffer, uint32_t len, void *priv)
{
    raw_data_t *new_data = malloc(sizeof(raw_data_t));
    new_data->data = buffer;
    new_data->data_length = len;
    if (xQueueSend(eth_receive_queue, &new_data, pdMS_TO_TICKS(ETH_WAIT_TO_SEND_RAW_TIMOUT)) == pdTRUE)
    {
        return ESP_OK;
    }
    else
    {
        ESP_LOGE(ETH_LOG, "ethernet queue is full!!");
        free(buffer);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/**
 * @brief Uinitialize the ehternet module
 * @param[in] arg extra event arguments
 * @param[in] event_base event base id
 * @param[in] event_id event id
 * @param[in] event_data event extra data
 * @retval
 */
static void ethernet_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(ETH_LOG, "Link Up");

        xQueueReset(eth_receive_queue);
        eth_connected = true;
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        eth_connected = false;
        ESP_LOGI(ETH_LOG, "Link Down");
        xQueueReset(eth_receive_queue);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(ETH_LOG, "Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(ETH_LOG, "Stopped");
        break;
    default:
        break;
    }
}

/**
 * @brief Initialize LAN8720 with ESP32 internal MAC
 *
 * @param[out] mac_out optionally returns Ethernet MAC object
 * @param[out] phy_out optionally returns Ethernet PHY object
 * @return
 *          - true if init succeeded
 *          - false if init failed
 */
static esp_err_t ethernet_init_LAN8720(esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
    gpio_set_level(ETH_OCS_POWER_GPIO, 1);

    // Init common MAC and PHY configs to the default configs
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    // Update PHY config
    phy_config.phy_addr = -1; // Set -1 to enable auto address detection in initialization
    phy_config.reset_gpio_num = ETH_PHY_RST_GPIO;
    // Init  MAC config to default
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    // Update MAC config based on board configuration
    esp32_emac_config.smi_mdc_gpio_num = ETH_MDC_GPIO;
    esp32_emac_config.smi_mdio_gpio_num = ETH_MDIO_GPIO;
    esp32_emac_config.clock_config.rmii.clock_mode = EMAC_CLK_EXT_IN;
    esp32_emac_config.clock_config.rmii.clock_gpio = EMAC_CLK_IN_GPIO;
    // Create new ESP32 Ethernet MAC instance
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
    // Create new LAN8720 PHY instance
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    // Init Ethernet driver to default and install it
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    if (esp_eth_driver_install(&config, &eth_handle) == ESP_OK)
    {
        ESP_LOGI(ETH_LOG, "Ethernet driver has been installed successfully");
        if (mac_out != NULL)
        {
            *mac_out = mac;
        }
        if (phy_out != NULL)
        {
            *phy_out = phy;
        }
        return ESP_OK;
    }
    if (eth_handle != NULL)
    {
        esp_eth_driver_uninstall(eth_handle);
    }
    if (mac != NULL)
    {
        mac->del(mac);
    }
    if (phy != NULL)
    {
        phy->del(phy);
    }
    return ESP_FAIL;
}

/**
 * @brief Uninitialize the LAN8720 ethernet driver
 * @param None
 * @retval
 */
static void ethernet_uninit_LAN8720(void)
{
    // Uninistall ethernet driver
    esp_eth_driver_uninstall(eth_handle);
    eth_handle = NULL;
}

/************************ (C) COPYRIGHT WeNet *****END OF FILE****************/
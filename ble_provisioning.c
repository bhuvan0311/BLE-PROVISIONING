#include <stdio.h>
#include <string.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

static const char *TAG = "ble_prov_example";

// BLE Device Name and Security
#define PROV_BLE_DEVICE_NAME  "ESP32-BLE-Prov"
#define PROOF_OF_POSSESSION   "abcd1234"  // Align with tutorial

// Service UUID (Same as the tutorial)
static uint8_t custom_service_uuid[] = {
  0x21, 0x43, 0x65, 0x87, 0x09, 0xba, 0xdc, 0xfe,
  0xef, 0xcd, 0xab, 0x90, 0x78, 0x56, 0x34, 0x12
};

// Event handler
static void event_handler(void* arg, esp_event_base_t event_base, 
                          int event_id, void* event_data) {
  if (event_base == WIFI_PROV_EVENT) {
    switch (event_id) {
      case WIFI_PROV_START:
        ESP_LOGI(TAG, "Provisioning started");
        break;
      case WIFI_PROV_CRED_RECV: {
        wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
        ESP_LOGI(TAG, "Received SSID: %s, Password: %s",
                 (char *)wifi_sta_cfg->ssid, (char *)wifi_sta_cfg->password);
        break;
      }
      case WIFI_PROV_CRED_SUCCESS:
        ESP_LOGI(TAG, "Provisioning successful. Connecting to Wi-Fi...");
        // Start Wi-Fi connection after provisioning
        esp_wifi_start();
        break;
      case WIFI_PROV_CRED_FAIL: {
        wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
        ESP_LOGE(TAG, "Failed! Reason: %s", 
                (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Auth Error" : "AP Not Found");
        break;
      }
      case WIFI_PROV_END:
        wifi_prov_mgr_deinit(); // Cleanup
        break;
      default:
        break;
    }
  }
}

void app_main() {
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  // Initialize Wi-Fi and Event Loop
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  // Configure Wi-Fi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Configure BLE Provisioning
  wifi_prov_mgr_config_t config = {
    .scheme = wifi_prov_scheme_ble,
    .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
  };
  ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

  // Register Event Handler
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL));

  bool provisioned = false;
  ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

  if (!provisioned) {
    ESP_LOGI(TAG, "Starting BLE provisioning...");
    
    // Set security with POP (same as tutorial)
    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid); // Tutorial's UUID
    
    // Start provisioning with POP
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, PROOF_OF_POSSESSION, 
                                                    PROV_BLE_DEVICE_NAME, NULL));
  } else {
    ESP_LOGI(TAG, "Already provisioned. Connecting to Wi-Fi...");
    wifi_prov_mgr_deinit();
    ESP_ERROR_CHECK(esp_wifi_start());
  }
}
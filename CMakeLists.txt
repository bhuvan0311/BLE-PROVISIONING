idf_component_register(
  SRCS "main.c"
  INCLUDE_DIRS "."
  REQUIRES 
    esp_wifi 
    esp_common 
    nvs_flash 
    wifi_provisioning 
    bt
  PRIV_REQUIRES 
    esp_netif 
    esp_event
)

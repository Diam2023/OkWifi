/**
 * @author Diam
 * @date 2023-07-30
 */


#include <iostream>
#include <thread>

#include <nvs_flash.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include "gpio_cxx.hpp"

#include "wifi_provisioning/manager.h"
#include "ProvServer.h"
#include "OkWifi.h"


#include <wifi_provisioning/wifi_config.h>
#include <wifi_provisioning/manager.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <wifi_provisioning/scheme_ble.h>


// ms s us
using namespace std::chrono_literals;

static const char *TAG = "Main";



/**
 * @brief Application main
 * This Function Will Run With MainTask And When Called app_main() MainTask Ending;
 */
extern "C" _Noreturn void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    esp_event_loop_create_default();

    auto led = idf::GPIO_Output(idf::GPIONum(13));
    auto led_status = false;
    enum class Status {
        Waiting,
        Ending
    };
    auto now_status = Status::Waiting;
    auto res = std::thread([&led, &now_status, &led_status]() -> void {
        while (true) {
            if (led_status) {
                led.set_low();
            } else {
                led.set_high();
            }
            led_status = !led_status;

            if (now_status != Status::Waiting) {
                std::this_thread::sleep_for(800ms);
            }
            std::this_thread::sleep_for(200ms);
        }
    });

    ok_wifi::OkWifi::getInstance().init();

    ok_wifi::OkWifi::getInstance().join();

//    auto def_ = esp_netif_create_default_wifi_sta();
//    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//    esp_wifi_init(&cfg);
//
//    wifi_prov_mgr_config_t config{};
//
//    config.scheme = wifi_prov_scheme_ble;
//    config.scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM;
//    wifi_prov_mgr_reset_provisioning();
//
//    wifi_prov_mgr_init(config);
//
//    const char *service_name = "WaterBox_BLU";
//    wifi_prov_security_t security = WIFI_PROV_SECURITY_0;
//
//    // 设定不自动停止
//    wifi_prov_mgr_disable_auto_stop(100);
//
//    wifi_prov_mgr_start_provisioning(security, nullptr, service_name, nullptr);
//
//    std::this_thread::sleep_for(5s);
//
//    ESP_LOGI(TAG, "Wait Prov Complete!");
//
//    bool provisioned = false;
//    static wifi_prov_sta_fail_reason_t reason;
//    static wifi_prov_sta_state_t state;
//

    now_status = Status::Ending;

    while (true) {

//        wifi_prov_mgr_is_provisioned(&provisioned);
//        wifi_prov_mgr_get_wifi_state(&state);
//
//        if (provisioned && (state == WIFI_PROV_STA_DISCONNECTED) &&
//            (wifi_prov_mgr_get_wifi_disconnect_reason(&reason) == ESP_OK)) {
//            // 连接失败，重新启动配网
////            esp_wifi_disconnect();
////            esp_wifi_stop();
//            nvs_flash_erase();
//
//            ESP_LOGW(TAG, "Stop");
//            ESP_LOGW(TAG, "Stop comp");
//
////            wifi_prov_mgr_reset_provisioning();
////            wifi_prov_mgr_reset_sm_state_on_failure();
//
//            std::this_thread::sleep_for(2s);
//            ESP_LOGW(TAG, "Reprov call");
//            ESP_ERROR_CHECK(wifi_prov_mgr_reset_sm_state_for_reprovision());
//            ESP_LOGW(TAG, "Reprov call comp");
//            std::this_thread::sleep_for(2s);
//
//            wifi_prov_mgr_stop_provisioning();
//            std::this_thread::sleep_for(2s);
//
////            while (!check_idle()) {
////                ESP_LOGI(TAG, "wait idle tick");
////                std::this_thread::sleep_for(2s);
////                wifi_prov_mgr_stop_provisioning();
////            }
//            wifi_prov_mgr_start_provisioning(security, nullptr, service_name, nullptr);
//            ESP_LOGE(TAG, "Error Checked, Restart Prov");
//        }

        std::cout << "Tick Time" << std::endl;
        std::this_thread::sleep_for(5s);
    }
}

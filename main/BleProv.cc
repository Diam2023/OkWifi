//
// Created by 35691 on 9/14/2023.
//

#include "BleProv.h"

#include <wifi_provisioning/scheme_ble.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event_cxx.hpp>

namespace ok_wifi {
    using namespace std::chrono_literals;

    static const char *TAG = "BleProv";

    BleProv::BleProv() : thread(), result(ProvStatus::ProvPrepare, ""), serviceName("WaterBox_Device") {
        thread = std::thread(&BleProv::run, this);
    }

    void BleProv::run() {
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        wifi_prov_mgr_config_t config = {
                .scheme = wifi_prov_scheme_ble,
                .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
        };
        wifi_prov_mgr_reset_provisioning();

        ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

        const char *service_name = "WaterBox_BLU";
        wifi_prov_security_t security = WIFI_PROV_SECURITY_0;
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, nullptr, service_name, nullptr));
        result.setStatus(ProvStatus::ProvWaiting);
        wifi_prov_mgr_wait();

        bool provisioned = false;
        wifi_prov_mgr_is_provisioned(&provisioned);
        if (provisioned) {
            wifi_prov_mgr_deinit();
            esp_event_loop_delete_default();

            wifi_config_t wifi_cfg;
            if (esp_wifi_get_config(WIFI_IF_STA, &wifi_cfg) == ESP_OK) {
                std::string ssid = reinterpret_cast<char *>(wifi_cfg.sta.ssid);
                std::string pwd = reinterpret_cast<char *>(wifi_cfg.sta.password);
                result.setSsid(ssid);
                result.setPwd(pwd);
                result.setResult(ProvResultStatus::ResOk);

                ESP_LOGI(TAG, "SSID: %s, PWD: %s", ssid.c_str(), pwd.c_str());
            } else {
                ESP_LOGI(TAG, "Err Prov disconnected");
                result.setResult(ProvResultStatus::ResError);
            }
        } else {
            ESP_LOGI(TAG, "Prov Wifi Error!");
            result.setResult(ProvResultStatus::ResError);
        }
    }

    ProvResult &BleProv::getProvResult() {
        return result;
    }

//    void BleProv::start() {
//        thread = std::thread(&BleProv::run, this);
//    }

    void BleProv::wait(long timeout) {
        long count = timeout * 2;
        while (result.getResult() == ok_wifi::ProvResultStatus::ResUnknown) {
            std::this_thread::sleep_for(500ms);
            count--;
            if (count == 0) {
                throw idf::event::EventException(ESP_ERR_TIMEOUT);
            }
        }
    }
} // ok_wifi
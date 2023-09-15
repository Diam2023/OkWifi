//
// Created by 35691 on 9/15/2023.
//

#include "ProvServerScanner.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <memory>
#include <cstring>
#include "ProvServer.h"
#include <esp_event_cxx.hpp>
#include <esp_timer_cxx.hpp>
#include <esp_wps.h>

//extern uint8_t esp_wifi_get_user_init_flag_internal(void);

namespace ok_wifi {

    const long DEFAULT_SCAN_LIST_SIZE = 50;

    using namespace std::chrono_literals;

    static const char *TAG = "ProvServerScanner";

    void ProvServerScanner::init() {
        scan_net = esp_netif_create_default_wifi_sta();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);

        esp_wifi_set_mode(WIFI_MODE_STA);
        esp_wifi_start();

        thread = std::thread(&ProvServerScanner::run, this);
    }

    bool ProvServerScanner::scanOnce(std::chrono::seconds sec) {
        memset(ap_info, 0, sizeof(ap_info));
        auto ret = esp_wifi_scan_start(nullptr, true);
        if (ret == ESP_ERR_WIFI_STATE) {
            esp_wifi_scan_stop();
            throw idf::event::EventException(ESP_ERR_WIFI_STATE);
        }
        esp_wifi_scan_get_ap_records(&number, ap_info);
        esp_wifi_scan_get_ap_num(&ap_count);

        ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
        for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
            std::string scanSsid = reinterpret_cast<char *>(ap_info[i].ssid);
            if (scanSsid == this->server_ssid) {
                // connect it;
                ESP_LOGI(TAG, "Found Prov Wifi %s Wait to connect to Prov", scanSsid.c_str());
                return true;
            }
        }
        esp_wifi_scan_stop();
        std::this_thread::sleep_for(sec);
        return false;
    }

    void ProvServerScanner::run() {
        while (true) {
            if (stop_scan_signal) {
                // end for thread;
                break;
            }
            memset(ap_info, 0, sizeof(ap_info));

            esp_wifi_scan_start(nullptr, true);
            std::this_thread::sleep_for(500ms);
            esp_wifi_scan_get_ap_records(&number, ap_info);
            esp_wifi_scan_get_ap_num(&ap_count);

            ESP_LOGI(TAG, "Total APs scanned = %u", ap_count);
            for (int i = 0; (i < DEFAULT_SCAN_LIST_SIZE) && (i < ap_count); i++) {
                std::string scanSsid = reinterpret_cast<char *>(ap_info[i].ssid);
                if (scanSsid == this->server_ssid) {
                    // connect it;
                    ESP_LOGI(TAG, "Found Prov Wifi %s Wait to connect to Prov", scanSsid.c_str());

                    found_flag = true;
                    stop_scan_signal = true;
                }
            }
            esp_wifi_scan_stop();
            esp_wifi_clear_ap_list();
            std::this_thread::sleep_for(500ms);
        }
    }

    void ProvServerScanner::deinit() {
        stop_scan_signal = true;
        if (thread.joinable()) {
            thread.join();
        }
        ESP_LOGW(TAG, "Deinit Scanner!!");
        esp_wifi_clear_ap_list();
        esp_wifi_scan_stop();
        esp_wifi_stop();
        esp_wifi_restore();
        esp_wifi_deinit();
        esp_netif_destroy_default_wifi(scan_net);
    }

    const std::string &ProvServerScanner::getServerSsid() const {
        return server_ssid;
    }

    void ProvServerScanner::setServerSsid(const std::string &serverSsid) {
        server_ssid = serverSsid;
    }

    bool ProvServerScanner::checkFounded() const {
        return found_flag;
    }

    bool ProvServerScanner::wait(int timeout) {
        while (true) {
            timeout--;
            if (timeout <= 0) {
                throw idf::event::EventException(ESP_ERR_TIMEOUT);
                return false;
            }
            std::this_thread::sleep_for(1s);
            if (found_flag) {
                break;
            }
        }
        return true;
    }

} // ok_wifi
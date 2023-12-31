//
// Created by 35691 on 9/15/2023.
//

#include "ProvServer.h"

#include <esp_wifi_default.h>
#include <esp_wifi.h>
#include <cstring>
#include "Tools.h"
#include <esp_log.h>
#include <esp_http_server.h>

namespace ok_wifi {

    static const char *TAG = "ProvServer";

    static esp_err_t prov_get_handler(httpd_req_t *req) {
        const char *resp_str = (const char *) req->user_ctx;
        httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }

    void ProvServer::run() {
        using namespace std::chrono_literals;
        threadStatus = false;

        // 在firstWaitTime内以1s一次检测是否有AP连接
        // 有客户端连接后以outOfDate为间隔检查一次是否还有客户端连接中
        // 当没有检测到有客户端存在并经过outOfDate时间时即结束服务器
        auto firstConnection = false;
        auto firstTimeCounter = firstWaitTime;
        auto outOfDateCounter = outOfDate;
        static wifi_sta_list_t sta;

        while (true) {
            // 检查退出信号
            if (exitSignal)
            {
                break;
            }

            std::this_thread::sleep_for(1s);
            if (firstConnection) {
                outOfDateCounter -= 1s;
            } else {
                firstTimeCounter -= 1s;
            }

            esp_wifi_ap_get_sta_list(&sta);
            if (sta.num > 0) {
                // 有活动连接
                // 先检查是不是第一次检测到
                if (!firstConnection) {
                    // 第一次检测到客户端 则标记
                    firstConnection = true;
                } else {
                    // 并不是第一次检测到客户端
                    // 重置生命周期满
                    outOfDateCounter = outOfDate;
                }
            }

            // 当没有一个客户端连接并且第一次等待时间超时 关闭服务器
            if (!firstConnection && (firstTimeCounter <= 0s)) {
                break;
            }

            // 当所有客户端断开连接则关闭服务器
            if (firstConnection && (outOfDateCounter <= 0s)) {
                break;
            }

        }

        // 停止配网服务器
        httpd_stop(server);
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy_default_wifi(netPtr);
        threadStatus = true;
        ESP_LOGW(TAG, "Httpd And Wifi Going Down!");
    }

    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data) {
        if (event_id == WIFI_EVENT_AP_STACONNECTED) {
            auto *event = (wifi_event_ap_staconnected_t *) event_data;
            ESP_LOGI(TAG, "station %s join, AID=%d",
                     event->mac, event->aid);
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            auto *event = (wifi_event_ap_stadisconnected_t *) event_data;
            ESP_LOGI(TAG, "station %s leave, AID=%d",
                     event->mac, event->aid);
        }
    }

    static esp_event_handler_instance_t instance_any_id;

    void ProvServer::init() {
        ESP_LOGI(TAG, "Init");

        if (netPtr != nullptr) {
            esp_netif_destroy_default_wifi(netPtr);
            netPtr = nullptr;
        }
        netPtr = esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            nullptr,
                                                            &instance_any_id));

        wifi_config_t wifi_config{};
        bzero(&wifi_config, sizeof(wifi_config_t));

        wifi_config.ap.channel = 6;
        wifi_config.ap.max_connection = 20;
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        // wifi_config.ap.pmf_cfg.capable = false;
        // wifi_config.ap.pmf_cfg.required = false;
        wifi_config.ap.ssid_len = board_ssid.length();
        ok_wifi::stringToUint(wifi_config.ap.ssid, board_ssid);
        ok_wifi::stringToUint(wifi_config.ap.password, board_pwd);

        if (board_pwd.empty()) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

        ESP_ERROR_CHECK(esp_wifi_disable_pmf_config(WIFI_IF_AP));

        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "Server Start SSID:%s password:%s channel:%d",
                 board_ssid.c_str(), board_pwd.c_str(), 6);

        // start httpd
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.lru_purge_enable = true;
        responseJson = "{\"ssid\":\"" + prov_ssid + "\"," + "\"pwd\":\"" + prov_pwd + "\"}";

        // handler
        prov.uri = "/prov";
        prov.method = HTTP_GET;
        prov.handler = prov_get_handler;
        prov.user_ctx = (void *) responseJson.c_str();

        // Start the httpd server
        ESP_ERROR_CHECK(httpd_start(&server, &config));
        // Set URI handlers
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &prov));
        thread = std::thread(&ProvServer::run, this);
    }

    void ProvServer::stop() {
        exitSignal = true;
    }

    void ProvServer::waitCompleted() {
        if (thread.joinable()) {
            thread.join();
        }
    }

}
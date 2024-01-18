//
// Created by diam on 23-12-23.
//

#include "WifiProv.h"
#include "Tools.h"

#include <iostream>

#include <esp_log.h>
#include <esp_wifi_types.h>
#include <esp_wifi_default.h>
#include <esp_wifi.h>

#include <cstring>
#include <esp_http_server.h>
#include <ArduinoJson.hpp>

namespace ok_wifi {
    using namespace std::chrono_literals;

    static const char *TAG = "WifiProv";

    char g_ReceiveBuff[1000];

    static esp_err_t prov_set_handler(httpd_req_t *req) {
        if (WifiProv::getInstance().isStopping())
        {
            // Reject after first request
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Reject after first request");
            return ESP_FAIL;
        }
        bzero(g_ReceiveBuff, sizeof(g_ReceiveBuff));

        /* 如果内容长度大于缓冲区则截断 */
        if (req->content_len >= 1000) {
            ESP_LOGE(TAG, "Error receive buffer to large");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error receive buffer to large");
            return ESP_FAIL;
        }

        int ret = httpd_req_recv(req, g_ReceiveBuff, sizeof(g_ReceiveBuff));
        if (ret <= 0) {  /* 返回 0 表示连接已关闭 */
            /* 检查是否超时 */
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* 如果是超时，可以调用 httpd_req_recv() 重试
                 * 简单起见，这里我们直接
                 * 响应 HTTP 408（请求超时）错误给客户端 */
                httpd_resp_send_408(req);
            }
            /* 如果发生了错误，返回 ESP_FAIL 可以确保
             * 底层套接字被关闭 */
            return ESP_FAIL;
        }

        ArduinoJson::DynamicJsonDocument json(500);
        auto error = ArduinoJson::deserializeJson(json, g_ReceiveBuff);
        if (error) {
            ESP_LOGE(TAG, "Error %d %s", error.code(), error.c_str());
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error json string");

            return ESP_FAIL;
        }

        std::cout << g_ReceiveBuff << std::endl;
        std::cout << "SSID: " << json["ssid"] << " PWD: " << json["pwd"] << std::endl;
        // TODO Auth token;
        WifiProv::getInstance().getProvResult().setSsid(json["ssid"]);
        WifiProv::getInstance().getProvResult().setPwd(json["pwd"]);
        WifiProv::getInstance().getProvResult().setResult(ProvResultStatus::ResOk);
        // successful!
        httpd_resp_sendstr(req, "successful!");

        WifiProv::getInstance().stopAsync();

        return ESP_OK;
    }

    bool WifiProv::isStopping() {
        return stopSignal; 
    }

    void WifiProv::init() {

        ESP_LOGI(TAG, "Init");
        if (netObj != nullptr) {
            esp_netif_destroy_default_wifi(netObj);
        }

        netObj = esp_netif_create_default_wifi_ap();

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_wifi_init(&cfg);

        wifi_config_t wifi_config{};
        bzero(&wifi_config, sizeof(wifi_config_t));

        wifi_config.ap.channel = 6;
        wifi_config.ap.max_connection = 1;
        // default open wifi connection or not
        wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        wifi_config.ap.pmf_cfg.capable = false;
        wifi_config.ap.pmf_cfg.required = false;
        wifi_config.ap.ssid_len = serviceName.length();
        ok_wifi::stringToUint(wifi_config.ap.ssid, serviceName);
        ok_wifi::stringToUint(wifi_config.ap.password, serviceName);

//        if (serviceName.empty()) {
//            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
//        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

        ESP_ERROR_CHECK(esp_wifi_disable_pmf_config(WIFI_IF_AP));

        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "Prove Server Start SSID:%s password:%s channel:%d",
                 serviceName.c_str(), serviceName.c_str(), 6);

        // start httpd
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.lru_purge_enable = true;

        // handler

        uri.uri = "/prov_set";
        uri.method = HTTP_PUT;
        uri.handler = prov_set_handler;
        uri.user_ctx = nullptr;

        // Start the httpd server
        ESP_ERROR_CHECK(httpd_start(&handler, &config));
        // Set URI handlers
        ESP_ERROR_CHECK(httpd_register_uri_handler(handler, &uri));

        if (!thread.joinable()) {
            thread = std::thread(&WifiProv::run, this);
        }
    }

    void WifiProv::stop() noexcept {
        stopSignal = true;

        if (thread.joinable()) {
            thread.join();
        }
    }

    void WifiProv::stopAsync() {
        stopSignal = true;
    }

    WifiProv::WifiProv() : serviceName("OkWifiDevice"), provTimeout(9999s), provResult(""), netObj(nullptr) {
        provResult.setResult(ProvResultStatus::ResUnknown);
        stopSignal = false;
    }

    void WifiProv::run() {
        auto tempProvTime = provTimeout;
        // auto net = esp_netif_create_default_wifi_sta();
        static wifi_sta_list_t sta;

        while (true) {

            // stop
            if (stopSignal) {
                break;
            }
            if (tempProvTime <= 0s) {
                provResult.setResult(ProvResultStatus::ResError);
                break;
            }

            esp_wifi_ap_get_sta_list(&sta);
            if (sta.num > 0) {
                // 有活动连接
                if (tempProvTime < provTimeout) {
                    tempProvTime = provTimeout;
                }
            }


            std::this_thread::sleep_for(1s);
            tempProvTime -= 1s;
        }

        std::this_thread::sleep_for(2s);

        ESP_LOGD(TAG, "Stopping WifiProv");

        httpd_stop(handler);
        esp_wifi_stop();
        esp_wifi_deinit();
        if (netObj != nullptr){
            esp_netif_destroy_default_wifi(netObj);
            netObj = nullptr;
        }
    }

    bool WifiProv::wait(long timeout) {

        // 等待timeout时间 如果有结果则返回真
        long count = timeout * 2;
        while (provResult.getResult() == ok_wifi::ProvResultStatus::ResUnknown) {
            std::this_thread::sleep_for(500ms);
            if (timeout == 0) {
                continue;
            }

            count--;
            if (count <= 0) {
                return false;
            }
        }
        return true;
    }

} // ok_wifi

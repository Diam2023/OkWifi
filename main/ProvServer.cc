//
// Created by 35691 on 9/15/2023.
//

#include "ProvServer.h"

#include <utility>
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

        /* After sending the HTTP response the old HTTP request
         * headers are lost. Check if HTTP request headers can be read now. */
        if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
            ESP_LOGI(TAG, "Request headers lost");
        }
        return ESP_OK;
    }

    void ProvServer::run() {
        using namespace std::chrono_literals;

        int outOfDateCounter = outOfDate;

        while (true) {
            outOfDateCounter--;
            std::this_thread::sleep_for(1s);
            if (outOfDateCounter <= 0) {

                wifi_sta_list_t sta;
                esp_wifi_ap_get_sta_list(&sta);
                if (sta.num <= 0) {
                    break;
                } else {
                    ESP_LOGI(TAG, "OutOfDate Check Circle!");
                    outOfDateCounter += outOfDate;
                }
            }
        }
        stop();
    }

    static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data) {
        if (event_id == WIFI_EVENT_AP_STACONNECTED) {
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
            ESP_LOGI(TAG, "station %s join, AID=%d",
                     event->mac, event->aid);
        } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
            wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
            ESP_LOGI(TAG, "station %s leave, AID=%d",
                     event->mac, event->aid);
        }
    }
    static esp_event_handler_instance_t instance_any_id;

    void ProvServer::init() {
        if (netPtr != nullptr)
        {
            esp_netif_destroy_default_wifi(netPtr);
            netPtr = nullptr;
        }
        netPtr = esp_netif_create_default_wifi_ap();
        ESP_LOGW(TAG, "Server Init");

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
        wifi_config.ap.pmf_cfg.required = false;
        wifi_config.ap.ssid_len = board_ssid.length();
        ok_wifi::stringToUint(wifi_config.ap.ssid, board_ssid);
        ok_wifi::stringToUint(wifi_config.ap.password, board_pwd);

        if (board_pwd.empty()) {
            wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        }

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
        ESP_ERROR_CHECK(esp_wifi_start());

        ESP_LOGI(TAG, "Server Start finished. SSID:%s password:%s channel:%d",
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
        ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
        ESP_ERROR_CHECK(httpd_start(&server, &config));
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        ESP_ERROR_CHECK(httpd_register_uri_handler(server, &prov));
        thread = std::thread(&ProvServer::run, this);
    }

    void ProvServer::stop() {
        httpd_stop(server);
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy_default_wifi(netPtr);

        ESP_LOGW(TAG, "Httpd And Wifi Going Down!");
    }

    void ProvServer::waitCompleted() {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
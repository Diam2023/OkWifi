//
// Created by 35691 on 9/15/2023.
//

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include "ProvClient.h"
#include "Tools.h"
#include <esp_timer_cxx.hpp>
#include <lwip/netdb.h>
#include <ArduinoJson.hpp>

static const int WIFI_CONNECTED_BIT = BIT0;
static const int WIFI_FAIL_BIT = BIT1;

namespace ok_wifi {
    EventGroupDef_t *wifi_event_group = nullptr;

    static const char *TAG = "ProvClient";

    static esp_event_handler_instance_t instance_any_id;
    static esp_event_handler_instance_t instance_got_ip;

    static void event_handler(void *arg,
                              esp_event_base_t event_base,
                              int32_t event_id,
                              void *event_data) {
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "Start Connect");
            esp_wifi_connect();
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
            auto *event = (ip_event_got_ip_t *) event_data;
            ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }

    void ProvClient::init() {
        using namespace std::chrono_literals;
        if (wifi_event_group != nullptr) {
            vEventGroupDelete(wifi_event_group);
            wifi_event_group = nullptr;
        }
        wifi_event_group = xEventGroupCreate();

        if (net != nullptr) {
            esp_netif_destroy_default_wifi(net);
            net = nullptr;
        }
        net = esp_netif_create_default_wifi_sta();
        esp_netif_dhcpc_start(net);


        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &event_handler,
                                                            nullptr,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &event_handler,
                                                            nullptr,
                                                            &instance_got_ip));

        bzero(&wifi_config, sizeof(wifi_config_t));
        ok_wifi::stringToUint(wifi_config.sta.ssid, server_ssid);
        ok_wifi::stringToUint(wifi_config.sta.password, server_pwd);
        ESP_LOGI(TAG, "Prepare To Connect ProvServer SSID: %s PWD: %s", wifi_config.sta.ssid, wifi_config.sta.password);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK;
        wifi_config.sta.pmf_cfg.required = false;
        wifi_config.sta.channel = 6;

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

        ESP_ERROR_CHECK(esp_wifi_start());
        esp_wifi_connect();
        ESP_LOGI(TAG, "wifi_init_sta finished.");
    }

    void ProvClient::deinit() {
        ESP_LOGW(TAG, "ProvClient going down!");
        esp_event_handler_unregister(WIFI_EVENT,
                                     ESP_EVENT_ANY_ID,
                                     &event_handler);
        esp_event_handler_unregister(IP_EVENT,
                                     IP_EVENT_STA_GOT_IP,
                                     &event_handler);
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy_default_wifi(net);
        net = nullptr;
        if (wifi_event_group != nullptr) {
            vEventGroupDelete(wifi_event_group);
            wifi_event_group = nullptr;
        }
    }

    ClientProvResult ProvClient::waitWifiConnect() {
        using namespace std::chrono_literals;
        ESP_LOGI(TAG, "wifi wait bit.");
        int outOfDateCounter = provServerOutOfDate;
        while (true) {
            if (outOfDateCounter <= 0) {
                ESP_LOGE(TAG, "请检查热点最大支持个数");
                return ClientProvResult::ProvOutOfDate;
            }

            /* Waiting until either the connection is established (WIFI_CONNECTED_BIT)
             * or connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
             * The bits are set by event_handler() (see above) */
            EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                                   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                                   false,
                                                   false,
                                                   5000 / portTICK_PERIOD_MS);
            if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG, "Connected to Prov Server");
                return ClientProvResult::ProvConnected;
            } else if (bits & WIFI_FAIL_BIT) {
                ESP_LOGI(TAG, "Failed to connect to ProvServer");
                return ClientProvResult::ProvConnectFailed;
            } else {
                ESP_LOGW(TAG, "UNEXPECTED EVENT Start Reconnect");
                outOfDateCounter--;
                esp_wifi_disconnect();
                std::this_thread::sleep_for(1s);
                esp_wifi_connect();
                ESP_LOGW(TAG, "Reconnecting!!! For Wait 15s");
                std::this_thread::sleep_for(15s);

            }
            std::this_thread::sleep_for(1ms);
        }
        ESP_LOGI(TAG, "end wait bit.");
    }

    bool ProvClient::sendRequest() {
        std::string prov_request = "GET " + path + " HTTP/1.0\r\n"
                                                   "Host: " + server + ":" + port + "\r\n"
                                                                                    "User-Agent: esp-idf/1.0 esp32\r\n"
                                                                                    "\r\n";
        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        struct addrinfo *res;
        struct in_addr *addr;
        int s, r;
        char recv_buf[64];

        int retryCounter = provServerOutOfDate;
        while (true) {

            int err = getaddrinfo(server.c_str(), port.c_str(), &hints, &res);

            if (err != 0 || res == nullptr) {
                ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
                vTaskDelay(1000 / portTICK_PERIOD_MS);

                if (retryCounter <= 0) {
                    return false;
                } else {
                    retryCounter--;
                }
                continue;
            }

            /* Code to print the resolved IP.

            Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
            addr = &((struct sockaddr_in *) res->ai_addr)->sin_addr;
            ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

            s = socket(res->ai_family, res->ai_socktype, 0);
            if (s < 0) {
                ESP_LOGE(TAG, "... Failed to allocate socket.");
                freeaddrinfo(res);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                continue;
            }
            ESP_LOGI(TAG, "... allocated socket");

            if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
                ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
                close(s);
                freeaddrinfo(res);
                vTaskDelay(4000 / portTICK_PERIOD_MS);
                continue;
            }

            ESP_LOGI(TAG, "... connected");
            freeaddrinfo(res);

            if (write(s, prov_request.c_str(), prov_request.length()) < 0) {
                ESP_LOGE(TAG, "... socket send failed");
                close(s);
                vTaskDelay(4000 / portTICK_PERIOD_MS);
                retryCounter--;
                if (retryCounter <= 0) {
                    return false;
                }
                continue;
            }
            ESP_LOGI(TAG, "... socket send success");

            timeval receiving_timeout{};
            receiving_timeout.tv_sec = 5;
            receiving_timeout.tv_usec = 0;
            if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                           sizeof(receiving_timeout)) < 0) {
                ESP_LOGE(TAG, "... failed to set socket receiving timeout");
                close(s);
                vTaskDelay(4000 / portTICK_PERIOD_MS);
                retryCounter--;
                if (retryCounter <= 0) {
                    return false;
                }
                continue;
            }
            ESP_LOGI(TAG, "... set socket receiving timeout success");

            std::string jsonData;
            int line = 0;
            do {
                bzero(recv_buf, sizeof(recv_buf));
                r = read(s, recv_buf, sizeof(recv_buf) - 1);

                if (line == 1) {
                    jsonData = recv_buf;
                }
                line++;
//                ESP_LOGI(TAG, "line %d DATA: %s", line, recv_buf);
            } while (r > 0);

            ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
            close(s);
            ESP_LOGI(TAG, "End Send Request");

            ArduinoJson::DynamicJsonDocument json(128);
            auto error = ArduinoJson::deserializeJson(json, jsonData);
            if (error) {
                ESP_LOGE(TAG, "Error %d %s", error.code(), error.c_str());
                return false;
            }
            setProvSsid(json["ssid"]);
            setProvPwd(json["pwd"]);

            ESP_LOGI(TAG, "JSON Data: %s", jsonData.c_str());

            // TODO Get Prov SSID And PWD

//            this->prov_ssid =
            break;
        }

        return true;
    }
}

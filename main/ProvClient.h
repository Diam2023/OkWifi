//
// Created by 35691 on 9/15/2023.
//

#ifndef OKWIFI_PROVCLIENT_H
#define OKWIFI_PROVCLIENT_H

#include <string>
#include <thread>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_tls_crypto.h"
#include <esp_http_client.h>

namespace ok_wifi {


    class ProvClient {
    private:
        std::string server_ssid;
        std::string server_pwd;


        std::string prov_ssid;
        std::string prov_pwd;

        std::thread thread;

//    httpd_handle_t client;

    public:
        [[nodiscard]] const std::string &getServerSsid() const {
            return server_ssid;
        }

        void setServerSsid(const std::string &serverSsid) {
            server_ssid = serverSsid;
        }

        [[nodiscard]] const std::string &getServerPwd() const {
            return server_pwd;
        }

        void setServerPwd(const std::string &serverPwd) {
            server_pwd = serverPwd;
        }

        [[nodiscard]] const std::string &getProvSsid() const {
            return prov_ssid;
        }

        void setProvSsid(const std::string &provSsid) {
            prov_ssid = provSsid;
        }

        [[nodiscard]] const std::string &getProvPwd() const {
            return prov_pwd;
        }

        void setProvPwd(const std::string &provPwd) {
            prov_pwd = provPwd;
        }

    public:
        void init();

        void run();


        ProvClient();


        ProvClient &getInstance() {
            static ProvClient client;
            return client;
        }


    };

}

#endif //OKWIFI_PROVCLIENT_H

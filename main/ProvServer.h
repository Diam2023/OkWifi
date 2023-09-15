//
// Created by 35691 on 9/15/2023.
//

#ifndef OKWIFI_PROVSERVER_H
#define OKWIFI_PROVSERVER_H

#include <thread>
#include <string>


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
#include <esp_http_server.h>


namespace ok_wifi {

    static const char *DEFAULT_PROV_SSID = "Default_OkWifi";
    static const char *DEFAULT_PROV_PWD = "Default_OkWifi";


    class ProvServer {
    private:
        std::string board_ssid;
        std::string board_pwd;

        std::string prov_ssid;
        std::string prov_pwd;

        std::thread thread;

        httpd_handle_t server;

        httpd_uri_t prov;

        std::string responseJson;
        esp_netif_obj *netPtr;

        // start thread
        void run();

        // close after 200s
        int outOfDate = 200;
    public:
        [[nodiscard]] int getOutOfDate() const {
            return outOfDate;
        }

        void setOutOfDate(int nums) {
            ProvServer::outOfDate = nums;
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


        [[nodiscard]] const std::string &getBoardSsid() const {
            return board_ssid;
        }

        void setBoardSsid(const std::string &boardSsid) {
            board_ssid = boardSsid;
        }

        [[nodiscard]] const std::string &getBoardPwd() const {
            return board_pwd;
        }

        void setBoardPwd(const std::string &boardPwd) {
            board_pwd = boardPwd;
        }

        ProvServer() : board_ssid(DEFAULT_PROV_SSID), board_pwd(DEFAULT_PROV_PWD) {};

        void stop();

        void init();

        /**
         * Wait thread Completed
         */
        void waitCompleted();

        inline static ProvServer &getInstance() {
            static ProvServer server;
            return server;

        }
    };

}

#endif //OKWIFI_PROVSERVER_H

//
// Created by 35691 on 9/15/2023.
//

#ifndef OKWIFI_PROVSERVERSCANNER_H
#define OKWIFI_PROVSERVERSCANNER_H

#include <string>
#include <thread>
#include <esp_netif_types.h>
#include "ProvServer.h"

namespace ok_wifi {

    class ProvServerScanner {
    private:
        std::string server_ssid;

        bool stop_scan_signal = false;

        esp_netif_t *scan_net = nullptr;

        std::thread thread;

        bool found_flag = false;

        static const long DEFAULT_SCAN_LIST_SIZE = 50;
        uint16_t ap_count = 0;
        wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
        uint16_t number = DEFAULT_SCAN_LIST_SIZE;

    public:
        [[nodiscard]] const std::string &getServerSsid() const;

        void setServerSsid(const std::string &serverSsid);

        [[nodiscard]] bool checkFounded() const;

        void join() {
            thread.join();
        }

        /**
         * Wait Time out
         * @param timeout seconds of time
         */
        bool wait(int timeout);

        void run();

        ProvServerScanner(std::string ssid = DEFAULT_PROV_SSID) : server_ssid(ssid) {};

        void init();

        void deinit();

        static ProvServerScanner &getInstance() {
            static ProvServerScanner scanner;
            return scanner;
        }
    };

} // ok_wifi

#endif //OKWIFI_PROVSERVERSCANNER_H

//
// Created by diam on 23-12-23.
//

#ifndef OKWIFI_WIFIPROV_H
#define OKWIFI_WIFIPROV_H

#include <thread>

#include <esp_wifi.h>
#include <esp_http_server.h>

namespace ok_wifi {

    enum class ProvResultStatus {
        ResUnknown,
        ResOk,
        ResError // Timeout
    };

    /**
     * Result of process
     */
    class ProvResult {
    private:
        /**
         * Result Message
         */
        std::string resultMessage;

        /**
         * wifi ssid
         */
        std::string ssid;

        /**
         * wifi password
         */
        std::string pwd;

        ProvResultStatus result;
    public:
        [[nodiscard]] ProvResultStatus getResult() const {
            return result;
        }

        void setResult(ProvResultStatus result_) {
            ProvResult::result = result_;
        }

        [[nodiscard]] const std::string &getResultMessage() const {
            return resultMessage;
        }

        void setResultMessage(const std::string &message) {
            ProvResult::resultMessage = message;
        }

        [[nodiscard]] const std::string &getSsid() const {
            return ssid;
        }

        void setSsid(const std::string &ssid_) {
            ProvResult::ssid = ssid_;
        }

        [[nodiscard]] const std::string &getPwd() const {
            return pwd;
        }

        void setPwd(const std::string &pwd_) {
            ProvResult::pwd = pwd_;
        }

        /**
         * Construct a new object
         * @param message_ message for result
         * @param ssid_ ssid of wifi from prov
         * @param pwd_ pwd of wifi from prov
         */
        explicit ProvResult(std::string message_, std::string ssid_ = "",
                   std::string pwd_ = "") : resultMessage(std::move(message_)), ssid(std::move(ssid_)),
                                            pwd(std::move(pwd_)), result(ProvResultStatus::ResUnknown) {};
    };


    class WifiProv {
    protected:

        std::string serviceName;


        std::chrono::seconds provTimeout;

        /**
         * main thread
         */
        std::thread thread;

        /**
         * Signal for stop
         */
        bool stopSignal;

        /**
         * result of Prov
         */
        ProvResult provResult;

        esp_netif_obj* netObj;

        httpd_uri uri;
        httpd_handle_t handler;

    public:
        WifiProv();

        virtual ~WifiProv() = default;

        void init();

        void stop() noexcept;

        bool isStopping();
        /**
         * 重启配网
         */
//        void restart();

        /**
         * Timeout for waiting
         * @param timeout forever if 0
         */
        bool wait(long timeout = 20);

        /**
         * Main Thread
         */
        void run();

        ProvResult &getProvResult() {
            return provResult;
        }

        static WifiProv &getInstance() {
            static WifiProv prov;
            return prov;
        }
    };

} // ok_wifi

#endif //OKWIFI_WIFIPROV_H

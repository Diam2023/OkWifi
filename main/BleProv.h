//
// Created by 35691 on 9/14/2023.
//

#ifndef OKWIFI_BLEPROV_H
#define OKWIFI_BLEPROV_H

#include <thread>
#include <mutex>
#include <string>
#include <utility>
#include <esp_netif_types.h>

namespace ok_wifi {

    // TODO To Write Process!
    enum class ProvStatus {
        ProvPrepare, // Preparing to prov
        ProvWaiting, // Waiting for Prov
        ProvReceived, // Received Signal
        ProvClosed, // Closed From Prover
        ProvConnectFailed, // SSID Not found Or WifiError
        ProvUnknownError, // unknown error
    };

    enum class ProvResultStatus
    {
        ResUnknown,
        ResOk,
        ResError
    };

    /**
     * Result of process
     */
    class ProvResult {
    private:
        /**
         * Prov Status
         */
        ProvStatus status;
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

        [[nodiscard]] ProvStatus getStatus() const {
            return status;
        }

        void setStatus(ProvStatus provStatus) {
            ProvResult::status = provStatus;
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
         * @param status_ status for result
         * @param message_ message for result
         * @param ssid_ ssid of wifi from prov
         * @param pwd_ pwd of wifi from prov
         */
        ProvResult(const ProvStatus status_, std::string message_, std::string ssid_ = "",
                   std::string pwd_ = "") : status(status_), resultMessage(std::move(message_)), ssid(std::move(ssid_)),
                                            pwd(std::move(pwd_)), result(ProvResultStatus::ResUnknown) {};
    };

    class BleProv {
    private:
        std::thread thread;

        ProvResult result;

        std::string serviceName;

        esp_netif_obj *net;

    public:
        BleProv();

        void init();

        void stop();

        /**
         * Timeout for waiting
         * @param timeout forever if 0
         */
        bool wait(long timeout = 20);

        /**
         * Main Thread
         */
        void run();

        ProvResult &getProvResult();

        static BleProv& getInstance()
        {
            static BleProv prov;
            return prov;
        }
    };

} // ok_wifi

#endif //OKWIFI_BLEPROV_H

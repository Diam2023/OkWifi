/**
 * @author Diam
 * @date 2023-07-30
 */


#include <iostream>
#include <memory>
#include <thread>

#include "esp_timer_cxx.hpp"
#include "esp_event_cxx.hpp"
#include "gpio_cxx.hpp"

// ms s us
using namespace std::chrono_literals;

//{
//
//using namespace idf::event;
//
//// Create A Timer For Application
//auto led_fir = idf::GPIO_Output(idf::GPIONum(12));
//auto led_sec = idf::GPIO_Output(idf::GPIONum(13));
//static bool led_fir_status = false;
//
//ESPEventLoop eventLoop;
//auto ledEvent = ESPEvent("led_event", ESPEventID(0xF1));
//
//auto reg = eventLoop.register_event_timed(
//        ledEvent,
//        [&led_sec](const ESPEvent &event, void *data) {
//
//            std::cout << "received event: " << event.base << "/" << event.id << std::endl;
//            auto status = *reinterpret_cast<bool *>(data);
//            if (status) {
//                led_sec.set_low();
//            } else {
//                led_sec.set_high();
//            }
//            std::cout << "led event started" << std::endl;
//        }, 500ms, [](const idf::event::ESPEvent &) {
//            std::cout << "led event timeout..." << std::endl;
//        }
//);
//
//auto timer = std::make_unique<idf::esp_timer::ESPTimer>([&]() {
//    if (led_fir_status) {
//        led_fir.set_high();
//    } else {
//        led_fir.set_low();
//    }
//
//    led_fir_status = !led_fir_status;
//
//    std::cout << "Test Timer Output" << std::endl;
//
//}, "TestTimer");
//
//// Start Timer  ms
//timer->start_periodic(500ms);
//
//while (true) {
//eventLoop.post_event_data(ledEvent, led_fir_status, 10ms);
//std::cout << "Test out put" << std::endl;
//std::this_thread::sleep_for(125ms);
//}
//}


/**
 * @brief Application main
 * This Function Will Run With MainTask And When Called app_main() MainTask Ending;
 */
extern "C" _Noreturn void app_main() {
    while (true) {
        std::cout << "Test out put" << std::endl;
        std::this_thread::sleep_for(125ms);
    }
}

# A batch distribution network project

## Version
* ESP-IDF: v5.2 


## Notice
当使用配网的SSID与PWD进行配网，
失败时需要调用`wifi_prov_mgr_reset_sm_state_for_reprovision()`以及
`wifi_prov_mgr_stop_provisioning()`两个函数来重置，
并且使用`wifi_prov_mgr_start_provisioning`来重新开启配网，
但是如果配网状态不为IDLE，调用开始api会报错未停止，
所以需要检测是否为IDLE状态以及阻塞到配网完全停止才可以重新配网。

结局方案是提供检测的函数或者阻塞的函数，所以有如下操作：

编译时在源码`manager.h`中加入
```c
void wifi_prov_mgr_stop_provisioning_block();

bool wifi_prov_mgr_is_stopped();
```
在`manager.c`中加入
```c
void wifi_prov_mgr_stop_provisioning_block()
{
    if (!prov_ctx_lock) {
        ESP_LOGE(TAG, "Provisioning manager not initialized");
        return;
    }

    ACQUIRE_LOCK(prov_ctx_lock);

    wifi_prov_mgr_stop_service(1);

    RELEASE_LOCK(prov_ctx_lock);
}

bool wifi_prov_mgr_is_stopped()
{
    return (prov_ctx->prov_state == WIFI_PROV_STATE_IDLE);
}
```

## Question && Solution

## Feature
* [x] batch mode
* [ ] Add Select Hide SSID Option
* [ ] Add AP Select command for Server When Wifi Connection going full
* [ ] Add Security module

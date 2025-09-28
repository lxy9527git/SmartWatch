/**
 * @file wifi.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_wifi.h>
#include "ssid_manager.h"
#include "wifi_station.h"
#include "wifi_configuration_ap.h"

namespace hdl{

    class wifi
    {
        private:
            /*私有构造函数，禁止外部直接实例化*/
            wifi(){}
            /*禁止拷贝构造和赋值操作*/
            wifi(const wifi&) = delete;
            wifi& operator = (const wifi&) = delete;
        public:

            /*获取单例实例的静态方法*/
            inline static wifi& getInstance() {
                static wifi instance;
                return instance;
            }

            inline static void Init()
            {
                /*Initialize the default event loop*/
                ESP_ERROR_CHECK(esp_event_loop_create_default());
                /*Initialize NVS flash for Wi-Fi configuration*/
                esp_err_t ret = nvs_flash_init();
                if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                    ESP_ERROR_CHECK(nvs_flash_erase());
                    ret = nvs_flash_init();
                }
                ESP_ERROR_CHECK(ret);
            }

            inline static void StopWifi()
            {
                wifi_mode_t mode;
                /*获取当前模式*/
                esp_wifi_get_mode(&mode);

                if(mode == WIFI_MODE_STA){
                    StopWifiStation();
                }else if(mode == WIFI_MODE_APSTA){
                    StopWifiConfigurationAp();
                }
            }

            inline static bool IsSsidListEmpty()
            {
                auto& ssid_list = SsidManager::GetInstance().GetSsidList();
                return ssid_list.empty();
            }

            inline static void ClearSsidList()
            {
                SsidManager::GetInstance().Clear();
            }

            inline static void StartWifiConfigurationAp()
            {
                auto& ap = WifiConfigurationAp::GetInstance();
                ap.SetLanguage("zh-CN");
                ap.SetSsidPrefix("SmartWatch");
                ap.Start();
            }

            inline static void StopWifiConfigurationAp()
            {
                WifiConfigurationAp::GetInstance().Stop();
            }

            inline static void StartWifiStation(std::function<void(const std::string& ssid)> on_connected)
            {
                if(on_connected != NULL)WifiStation::GetInstance().OnConnected(on_connected);
                WifiStation::GetInstance().Start();
            }

            inline static void StopWifiStation()
            {
                WifiStation::GetInstance().OnConnected(NULL);
                WifiStation::GetInstance().Stop();
            }
            
            inline static bool WifiStationIsConnected()
            {
                return WifiStation::GetInstance().IsConnected();
            }
    };
}




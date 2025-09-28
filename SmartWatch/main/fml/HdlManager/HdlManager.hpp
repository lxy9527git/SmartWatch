/**
 * @file HdlManager.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <ctime>
#include <time.h>
#include <sys/time.h>
#include <esp_sleep.h>
#include "hdl.hpp"
#include "esp_sntp.h"

namespace fml{

    class HdlManager
    {
        #define HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_NOTHING_BIT                                  (1<<0)
        #define HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_LVGL_BIT                                     (1<<1)

        struct RtcData_t{
            int64_t UpdateInterval = 10000;         //unit:us
            int64_t UpdateCount = 0;
            struct tm RtcTime;
        };

        struct PowerInfos_t{
            uint8_t BatteryLevel;
            bool BatteryIsCharging;
        };

        enum PowerMode_t{
            POWERMODE_NORMAL = 0,
            POWERMODE_GOINGSLEEP,
            POWERMODE_SLEEPING
        };

        struct PowerManager_t{
            int64_t UpdateInterval = 10000;         //unit:us
            int64_t UpdateCount = 0;

            PowerMode_t PowerMode = POWERMODE_NORMAL;
            uint32_t AutoSleepTime = 60000;        //unit:ms
        };

        struct KeyData_t{
            int64_t UpdateInterval = 10000;         //unit:us
            int64_t UpdateCount = 0;

            uint8_t KeyPwr;
            uint8_t KeyUp;
        };
        private:
            const char* TAG = "HdlManager";
            RtcData_t _RtcData;
            PowerInfos_t _PowerInfos;
            PowerManager_t _PowerManager;
            KeyData_t _KeyData;
            EventGroupHandle_t xEventGroup;
            
            /*私有构造函数，禁止外部直接实例化*/
            HdlManager() : xEventGroup(NULL){}
            ~HdlManager(){if(xEventGroup != NULL)vEventGroupDelete(xEventGroup);}
            /*禁止拷贝构造和赋值操作*/
            HdlManager(const HdlManager&) = delete;
            HdlManager& operator = (const HdlManager&) = delete;

            static void _UpdateSntpTime(const std::string& ssid);
            void _UpdateRtcTime();
            void _UpdatePowerInfos();
            void _UpdateGoSleep();
            void _UpdatePowerMode();
            void _UpdateKeyData();
        public:
            /*获取单例实例的静态方法*/
            inline static HdlManager& getInstance() {
                static HdlManager instance;
                return instance;
            }

            void init();
            void update();
            
            /*睡眠相关*/
            inline void set_no_sleep_for_nothing(){xEventGroupSetBits(xEventGroup, HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_NOTHING_BIT);}
            inline void clear_no_sleep_for_nothing(){xEventGroupClearBits(xEventGroup, HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_NOTHING_BIT);}
            inline void set_no_sleep_for_lvgl(){xEventGroupSetBits(xEventGroup, HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_LVGL_BIT);}
            inline void clear_no_sleep_for_lvgl(){xEventGroupClearBits(xEventGroup, HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_LVGL_BIT);}
            /*电量相关*/
            inline uint8_t get_battery_level(){return _PowerInfos.BatteryLevel;}
            inline bool get_battery_is_charging(){return _PowerInfos.BatteryIsCharging;}
            /*时间相关*/
            inline struct tm get_rtc_time(){return _RtcData.RtcTime;}
            /*屏幕相关*/
            inline static void set_brightness(uint8_t brightness){
                hdl::hdl::getInstance().setBrightness(brightness);
                hdl::hdl::getInstance().FlashSetInt("disp",true,"brightness",brightness);
            }
            inline static int get_brightness(){
                return hdl::hdl::getInstance().FlashGetInt("disp",true,"brightness",0);
            }
            /*wifi相关*/
            inline static void start_wifi_configuration_ap(){
                hdl::hdl::getInstance().StartWifiConfigurationAp();
                hdl::hdl::getInstance().FlashSetInt("wifi",true,"configure",1);
            }
            inline static void stop_wifi_configuration_ap(){
                hdl::hdl::getInstance().StopWifiConfigurationAp();
                hdl::hdl::getInstance().FlashSetInt("wifi",true,"configure",0);
            }
            inline static int get_wifi_configureation_ap(){
                return hdl::hdl::getInstance().FlashGetInt("wifi",true,"configure",0);
            }
            inline static void clear_wifi_ssid_list(){hdl::hdl::getInstance().ClearSsidList();}
            inline static bool wifi_station_is_connected(){return hdl::hdl::getInstance().WifiStationIsConnected();}
            inline static void start_wifi_station(){
                hdl::hdl::getInstance().StartWifiStation(_UpdateSntpTime);
                hdl::hdl::getInstance().FlashSetInt("wifi",true,"onoff",1);
            }
            inline static void stop_wifi_station(){
                hdl::hdl::getInstance().StopWifiStation();
                hdl::hdl::getInstance().FlashSetInt("wifi",true,"onoff",0);
            }
            inline static int get_wifi_station(){
                return hdl::hdl::getInstance().FlashGetInt("wifi",true,"onoff",0);
            }
            inline static void stop_wifi(){
                hdl::hdl::getInstance().StopWifi();
                hdl::hdl::getInstance().FlashSetInt("wifi",true,"onoff",0);
                hdl::hdl::getInstance().FlashSetInt("wifi",true,"configure",0);
            }
            /*蓝牙相关*/
            inline static esp_err_t bt_init(){return hdl::hdl::getInstance().BTInit();}
            inline static esp_err_t bt_deinit(){return hdl::hdl::getInstance().BTDeinit();}
            inline static void bt_set_button(uint8_t button_index, bool state){hdl::hdl::getInstance().BTSetButton(button_index,state);}
            inline static void bt_set_hat_switch(uint8_t value){hdl::hdl::getInstance().BTSetHatSwitch(value);}
            inline static void bt_set_joystick(uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry){hdl::hdl::getInstance().BTSetJoyStick(lx,ly,rx,ry);}
            inline static void bt_set_z_axis(uint16_t z){hdl::hdl::getInstance().BTSetZaxis(z);}
            inline static bool bt_is_connected(){return hdl::hdl::getInstance().BTIsConnected();}
            inline static void bt_send_report(){hdl::hdl::getInstance().BTSendReport();}
            inline static void bt_test(){hdl::hdl::getInstance().BTTest();}
            /*音频相关*/
            inline static int get_audio_data(std::vector<int32_t>& data){return hdl::hdl::getInstance().AudioInputData(data);}
            inline static int set_audio_data(std::vector<int32_t>& data){return hdl::hdl::getInstance().AudioOutputData(data);}
            inline static void set_audio_volume(int volume){
                hdl::hdl::getInstance().AudioSetOutputVolume(volume);
                hdl::hdl::getInstance().FlashSetInt("audio",true,"output_volume",volume);
            }
            inline static int get_audio_volume(){
                return hdl::hdl::getInstance().FlashGetInt("audio",true,"output_volume",0);
            }
            /*存储相关*/
            inline static void set_flash_int(const std::string& ns = "", bool read_write = false, const std::string& key = "", int32_t value = 0){hdl::hdl::getInstance().FlashSetInt(ns,read_write,key,value);}
            inline static int32_t get_flash_int(const std::string& ns = "", bool read_write = false, const std::string& key = "", int32_t default_value = 0){return hdl::hdl::getInstance().FlashGetInt(ns,read_write,key,default_value);}
            inline static void set_flash_string(const std::string& ns = "", bool read_write = false, const std::string& key = "", const std::string& value = ""){hdl::hdl::getInstance().FlashSetString(ns,read_write,key,value);}
            inline static std::string get_flash_string(const std::string& ns = "", bool read_write = false, const std::string& key = "", const std::string& default_value = ""){return hdl::hdl::getInstance().FlashGetString(ns,read_write,key,default_value);}
            /*IMU相关*/
            inline static void imu_start(){hdl::hdl::getInstance().IMUStart();}
            inline static void imu_stop(){hdl::hdl::getInstance().IMUStop();}
            inline static uint16_t imu_get_joystick_lx(){return hdl::hdl::getInstance().IMUGetJoyStickLx();}
            inline static uint16_t imu_get_joystick_ly(){return hdl::hdl::getInstance().IMUGetJoyStickLy();}
    };
}




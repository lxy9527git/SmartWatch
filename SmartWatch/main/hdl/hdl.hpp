/**
 * @file hdl.hpp
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
#include "hdl_config.hpp"
#include "disp.hpp"
#include "lvgl.hpp"
#include "button.hpp"
#include "power.hpp"
#include "wifi.hpp"
#include "bt.hpp"
#include "esp_littlefs.h"
#include "audio.hpp"
#include "flash.hpp"
#include "imu.hpp"

namespace hdl{

    class hdl{
        private:
            bool _isSleeping;
            esp_vfs_littlefs_conf_t fs = {
                .base_path = "/littlefs",
                .partition_label = "littlefs",
                .partition = NULL,
                .format_if_mount_failed = true,
                .read_only = false,
                .dont_mount = false,
                .grow_on_mount = false
            };

            /*私有构造函数，禁止外部直接实例化*/
            hdl() : _isSleeping(false) {}
            /*禁止拷贝构造和赋值操作*/
            hdl(const hdl&) = delete;
            hdl& operator = (const hdl&) = delete;
            inline static void lv_log_print_g_cb(lv_log_level_t level, const char * buf){ESP_LOGI(getInstance().TAG, "%d, %s", level, buf);}
        public:
            const char* TAG = "hdl";

            /*获取单例实例的静态方法*/
            inline static hdl& getInstance() {
                /*C++11 保证线程安全*/
                static hdl instance;
                return instance;
            }

            inline void init()
            {
                /* Filesystem */
                esp_err_t ret = esp_vfs_littlefs_register(&fs);
                if(ret != ESP_OK){
                    if (ret == ESP_FAIL)
                    {
                        ESP_LOGE(TAG, "Failed to mount or format filesystem");
                    }
                    else if (ret == ESP_ERR_NOT_FOUND)
                    {
                        ESP_LOGE(TAG, "Failed to find LittleFS partition");
                    }
                    else
                    {
                        ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
                    }
                    abort();
                }
                size_t total = 0, used = 0;
                ret = esp_littlefs_info(fs.partition_label, &total, &used);
                if (ret != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
                }
                else
                {
                    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
                }
                //lfs_t* p_lfs = NULL;
                //esp_littlefs_get_lfs(fs.partition_label, &p_lfs);

                /* Lvgl + disp + IIC */
                lvgl::getInstance().init(2);
                lv_display_set_color_format(lv_display_get_default(), (lv_color_format_t)LGVL_COLORDEPTH);
                //lv_log_register_print_cb(lv_log_print_g_cb);
                //lv_littlefs_set_handler(p_lfs);
                lvgl::getInstance().update();

                /* PMU AXP2101 */
                power::getInstance().init(BOARD_SDA_PIN, BOARD_SCL_PIN, GPIO_NUM_NC); 
                
                /* Buttons */
                button::getInstance().init(BOOT_BUTTON_GPIO);
                button::getInstance().begin();

                /* wifi */
                wifi::getInstance().Init(); 

                /*用于异常重启后，硬件复位清除异常状态*/
                {
                    int exception = FlashGetInt("hdl", true, "exception", 0);
                    if(exception != 238){
                        ESP_LOGE(TAG, "exception:%d", exception);
                        FlashSetInt("hdl", true, "exception", 238);
                        vTaskDelay(pdMS_TO_TICKS(100));
                        HardReset();
                    }
                    FlashSetInt("hdl", true, "exception", 0);
                }

                /* imu */
                imu::getInstance().init();
                imu::getInstance().self_test();
                
                /* audio */
                audio::getInstance().init(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
                                        AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, 
                                        AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
            }

            inline static void update(){lvgl::getInstance().update();}
            inline void isSleeping(bool sleep) { _isSleeping = sleep; }
            inline bool isSleeping(void) { return _isSleeping; } 
            /*内存相关*/
            inline static void PrintRAMInfo(char* title){
                /*详细内存统计: MALLOC_CAP_INTERNAL ...*/
                ESP_LOGI(getInstance().TAG,"-------------------%s--------------------------->", title);
                multi_heap_info_t info;
                heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
                ESP_LOGI(getInstance().TAG,"MALLOC_CAP_INTERNAL: free=%dKB, alloc=%dKB, largest=%d, mini=%d, allocb=%d, freeb=%d, tb=%d",
                info.total_free_bytes/1024, 
                info.total_allocated_bytes/1024,
                info.largest_free_block,
                info.minimum_free_bytes,
                info.allocated_blocks,
                info.free_blocks,
                info.total_blocks);

                heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
                ESP_LOGI(getInstance().TAG,"MALLOC_CAP_SPIRAM: free=%dKB, alloc=%dKB, largest=%d, mini=%d, allocb=%d, freeb=%d, tb=%d",
                info.total_free_bytes/1024, 
                info.total_allocated_bytes/1024,
                info.largest_free_block,
                info.minimum_free_bytes,
                info.allocated_blocks,
                info.free_blocks,
                info.total_blocks);
                ESP_LOGI(getInstance().TAG,"-------------------%s---------------------------<", title);
            }
            /*wifi相关*/
            inline static void StopWifi(){wifi::getInstance().StopWifi();}
            inline static void ClearSsidList(){wifi::getInstance().ClearSsidList();}
            inline static void StartWifiConfigurationAp(){wifi::getInstance().StartWifiConfigurationAp();}
            inline static void StopWifiConfigurationAp(){wifi::getInstance().StopWifiConfigurationAp();}
            inline static bool WifiStationIsConnected(){return wifi::getInstance().WifiStationIsConnected();}
            inline static void StartWifiStation(std::function<void(const std::string& ssid)> on_connected){
                if(!wifi::getInstance().IsSsidListEmpty())wifi::getInstance().StartWifiStation(on_connected);
            }
            inline static void StopWifiStation(){wifi::getInstance().StopWifiStation();}
            /*bt相关*/
            inline static esp_err_t BTInit(){return bt::getInstance().init();}
            inline static esp_err_t BTDeinit(){return bt::getInstance().deinit();}
            inline static void BTSetButton(uint8_t button_index, bool state){bt::getInstance().set_button(button_index,state);}
            inline static void BTSetHatSwitch(uint8_t value){bt::getInstance().set_hat_switch(value);}
            inline static void BTSetJoyStick(uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry){bt::getInstance().set_joystick(lx,ly,rx,ry);}
            inline static void BTSetZaxis(uint16_t z){bt::getInstance().set_z_axis(z);}
            inline static bool BTIsConnected(){return bt::getInstance().is_connected();}
            inline static void BTSendReport(){bt::getInstance().send_report();}
            inline static void BTTest(){bt::getInstance().test_gamepad();}
            /*电量相关*/
            inline static uint8_t batteryLevel(){return power::getInstance().batteryLevel();}
            inline static bool isCharging(){return power::getInstance().isCharging();}
            inline static void HardReset(){power::getInstance().powerReset();}
            /*屏幕相关*/
            inline static void resetDisp(){disp::getInstance().init();}
            inline static void setBrightness(uint8_t brightness){disp::getInstance().setBrightness(brightness);}
            inline static void DispSleep(){disp::getInstance().sleep();}
            inline static void DispWakeUp(){disp::getInstance().wakeup();}
            /*按键相关*/
            inline static bool isPowerKeyPressed(){return power::getInstance().isKeyPressed();}
            inline static bool isButtonKeyPressed(){return button::getInstance().pressed();}
            inline static bool isButtonKeyReleased(){return button::getInstance().released();}
            /*音频相关*/
            inline static void AudioSetOutputVolume(int volume){audio::getInstance().SetOutputVolume(volume);}
            inline static void AudioStart(){audio::getInstance().Start();}
            inline static void AudioStop(){audio::getInstance().Stop();}
            inline static int AudioRead(int32_t* dest, int samples){return audio::getInstance().read(dest, samples);}
            inline static int AudioWrite(const int32_t* data, int samples){return audio::getInstance().write(data, samples);}
            inline static int AudioOutputData(std::vector<int32_t>& data){return audio::getInstance().OutputData(data);}
            inline static int AudioInputData(std::vector<int32_t>& data){return audio::getInstance().InputData(data);}
            /*存储相关*/
            inline static std::string FlashGetString(const std::string& ns = "", bool read_write = false, const std::string& key = "", const std::string& default_value = ""){flash save(ns, read_write);return save.GetString(key, default_value);}
            inline static void FlashSetString(const std::string& ns = "", bool read_write = false, const std::string& key = "", const std::string& value = ""){flash save(ns, read_write);save.SetString(key, value);}
            inline static int32_t FlashGetInt(const std::string& ns = "", bool read_write = false, const std::string& key = "", int32_t default_value = 0){flash save(ns, read_write);return save.GetInt(key, default_value);}
            inline static void FlashSetInt(const std::string& ns = "", bool read_write = false, const std::string& key = "", int32_t value = 0){flash save(ns, read_write);save.SetInt(key, value);}
            inline static void FlashEraseKey(const std::string& ns = "", bool read_write = false, const std::string& key = ""){flash save(ns, read_write);save.EraseKey(key);}
            inline static void FlashEraseAll(const std::string& ns = "", bool read_write = false){flash save(ns, read_write);save.EraseAll();}
            /*IMU相关*/
            inline static void IMUStart(){imu::getInstance().start();}
            inline static void IMUStop(){imu::getInstance().stop();}
            inline static uint16_t IMUGetJoyStickLx(){return imu::getInstance().get_joystick_lx();}
            inline static uint16_t IMUGetJoyStickLy(){return imu::getInstance().get_joystick_ly();}
            inline static esp_err_t IMUClearAllInterrupts(){return imu::getInstance().clear_all_interrupts();}
            inline static esp_err_t IMUPowerOff(){return imu::getInstance().power_off();}
            
    };
}



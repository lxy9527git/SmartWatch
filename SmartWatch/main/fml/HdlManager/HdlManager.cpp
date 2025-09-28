/**
 * @file HdlManager.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "HdlManager.hpp"

namespace fml{

    void HdlManager::_UpdateSntpTime(const std::string& ssid)
    {
        if(esp_sntp_enabled())esp_sntp_stop();
        
        esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
        esp_sntp_setservername(0, "pool.ntp.org");
        //sntp_set_time_sync_notification_cb(time_sync_notification_cb); /*可选，同步时间成功后回调通知*/
        /*Set timezone to China Standard Time*/
        setenv("TZ", "CST-8", 1);
        tzset();
        esp_sntp_init();
    }

    void HdlManager::_UpdateRtcTime()
    {
        /* Set RTC time once*/
        //wait to do......rtc.setTime(_RtcData.RtcTime);

        /* Read RTC */
        //wait to do......rtc.getTime(_RtcData.RtcTime);

        //Temporary substitution------------------>
        time_t now;
        struct tm timeinfo;
        time(&now);                         // 获取当前时间戳
        localtime_r(&now, &timeinfo);       // 转换为 tm 结构体（本地时间）
        _RtcData.RtcTime.tm_hour = timeinfo.tm_hour;
        _RtcData.RtcTime.tm_min = timeinfo.tm_min;
        _RtcData.RtcTime.tm_sec = timeinfo.tm_sec;
        _RtcData.RtcTime.tm_year = timeinfo.tm_year;
        _RtcData.RtcTime.tm_mon = timeinfo.tm_mon;
        _RtcData.RtcTime.tm_mday = timeinfo.tm_mday;
        _RtcData.RtcTime.tm_wday = timeinfo.tm_wday;
        //<------------------Temporary substitution
    }
    void HdlManager::_UpdatePowerInfos()
    {
        _PowerInfos.BatteryLevel = hdl::hdl::getInstance().batteryLevel();
        _PowerInfos.BatteryIsCharging = hdl::hdl::getInstance().isCharging();
    }
    void HdlManager::_UpdateGoSleep()
    {
        EventBits_t bits = xEventGroupWaitBits(xEventGroup,HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_NOTHING_BIT | HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_LVGL_BIT,pdFALSE,pdFALSE,0);

        if ((bits & (HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_NOTHING_BIT | 
            HDLMANAGER_EVENTGROUP_NO_SLEEP_FOR_LVGL_BIT)) != 0) {
            /*任一事件位置位 → 不允许睡眠*/
            /*重置默认显示器的不活动计时器,模拟用户活动*/
            lv_disp_trig_activity(NULL);
        } else {
            /*两个位均为0 → 允许睡眠*/
            /* Check lvgl inactive time */
            if (lv_disp_get_inactive_time(NULL) > _PowerManager.AutoSleepTime){
                _PowerManager.PowerMode = POWERMODE_SLEEPING;                                                           /*一定时间没操作屏幕，进入睡眠*/
            }else if(_KeyData.KeyUp == hdl::button::PRESSED){                                                           /*当按下KeyUp，准备进入睡眠*/
                _PowerManager.PowerMode = POWERMODE_GOINGSLEEP;
            }else if((_PowerManager.PowerMode == POWERMODE_GOINGSLEEP) && (_KeyData.KeyUp == hdl::button::RELEASED)){   /*当松开KeyUp，进入睡眠*/
                _PowerManager.PowerMode = POWERMODE_SLEEPING;
            }   
        }
    }
    void HdlManager::_UpdatePowerMode()
    {
        if(_PowerManager.PowerMode == POWERMODE_SLEEPING){
            ESP_LOGI(TAG, "going sleep...");
            /* Setup wakeup pins */
            /* Touch pad */
            hdl::button::setwakeup(DISPLAY_TOUCH_INT, 0);

            /* imu */
            hdl::hdl::getInstance().IMUClearAllInterrupts();
            hdl::button::setwakeup(LSM_IQ, 0);

            /*启用GPIO唤醒功能*/
            esp_sleep_enable_gpio_wakeup();

            /* Close display */
            hdl::hdl::getInstance().DispSleep();

            /* Go to sleep :) */
            esp_light_sleep_start();

            /* ---------------------------------------------------------------- */

            ESP_LOGI(TAG, "wakeup!!!!");

            /* Wake up o.O */
            _PowerManager.PowerMode = POWERMODE_NORMAL;

            /* Update data at once */
            /* Clear key pwr */
            hdl::hdl::getInstance().isPowerKeyPressed();

            /* Restart display */
            hdl::hdl::getInstance().DispWakeUp();            

            /* Reset lvgl inactive time */
            lv_disp_trig_activity(NULL);

            /* Refresh full screen */
            lv_obj_invalidate(lv_scr_act());

            /* Display on */
            int brightness_volume = get_brightness();
            if(brightness_volume != 0){
                set_brightness(brightness_volume);
            }else{
                set_brightness(DISP_DEFAULT_BRIGHTNESS_VOLUME);
            }

        }
    }
    void HdlManager::_UpdateKeyData()
    {
        /* Key Home or Power */
        if (hdl::hdl::getInstance().isPowerKeyPressed()) {
            _KeyData.KeyPwr = hdl::button::PRESSED;
        }else{
            _KeyData.KeyPwr = hdl::button::RELEASED;
        }

        /* Key Up */
        if (hdl::hdl::getInstance().isButtonKeyPressed()) {
            _KeyData.KeyUp = hdl::button::PRESSED;
        }else if(hdl::hdl::getInstance().isButtonKeyReleased()){
            _KeyData.KeyUp = hdl::button::RELEASED;
        }
    }

    void HdlManager::init()
    {
        xEventGroup = xEventGroupCreate();
        hdl::hdl::getInstance().init();
        stop_wifi_configuration_ap();
        start_wifi_station();

        /*设置音量*/
        int output_volume =get_audio_volume();
        if(output_volume != 0){
            set_audio_volume(output_volume);
        }else{
            set_audio_volume(AUDIO_DEFAULT_OUTPUT_VOLUME);
        }

        /*设置亮度*/
        int brightness_volume = get_brightness();
        if(brightness_volume != 0){
            set_brightness(brightness_volume);
        }else{
            set_brightness(DISP_DEFAULT_BRIGHTNESS_VOLUME);
        }
    }

    void HdlManager::update()
    {
        /* Update Power */
        if ((esp_timer_get_time() - _PowerManager.UpdateCount) > _PowerManager.UpdateInterval) {
            _UpdatePowerInfos();
            _PowerManager.UpdateCount = esp_timer_get_time();
        }

        /* Update RTC */
        if ((esp_timer_get_time() - _RtcData.UpdateCount) > _RtcData.UpdateInterval) {
            _UpdateRtcTime();
            _RtcData.UpdateCount = esp_timer_get_time();
        }

        /* Update keys */
        if ((esp_timer_get_time() - _KeyData.UpdateCount) > _KeyData.UpdateInterval) {
            _UpdateKeyData();
            _KeyData.UpdateCount = esp_timer_get_time();
        }

        /* Update power control */
        _UpdateGoSleep();
        _UpdatePowerMode();

        /* hdl update */
        hdl::hdl::getInstance().update();
    }
}


/**
 * @file WatchDial.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <mooncake.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/time.h>
#include "bll.hpp"

using namespace fml;

namespace apl{

    class WatchDial : public mooncake::UIAbility
    {
        #define WATCHDIAL_TAG_NAME                               "WatchDial"

        #define WATCHDIAL_LV_STATUS_H                            (20)

        #define WATCHDAIL_LV_TIME_HM_W                           (DISPLAY_WIDTH)
        #define WATCHDIAL_LV_TIME_HM_H                           (75)
        #define WATCHDIAL_LV_TIME_HM_X                           (0)
        #define WATCHDIAL_LV_TIME_HM_Y                           (DISPLAY_HEIGHT - WATCHDIAL_LV_TIME_HM_H - 4)
        #define WATCHDAIL_LV_TIME_MW_W                           (DISPLAY_WIDTH)
        #define WATCHDIAL_LV_TIME_MW_H                           (30)
        #define WATCHDIAL_LV_TIME_MW_X                           (0)
        #define WATCHDIAL_LV_TIME_MW_Y                           (DISPLAY_HEIGHT - WATCHDIAL_LV_TIME_HM_H - WATCHDIAL_LV_TIME_MW_H - 4)

        struct watchdial_lv_screen_t{
            lv_obj_t* screen;
            lv_obj_t* tileview;
            lv_obj_t* tile;
        };

        struct watchdial_lv_bg_t{
            int start_jpeg_index;
            int end_jpeg_index;
            int cur_jpeg_index;
            lv_obj_t* img;
            lv_image_dsc_t img_dsc;
            struct fml::JpegDecoder::jpeg_output_info_t jpeg_src;
        };

        struct watchdial_lv_status_t{
            lv_obj_t* container;
            lv_obj_t* wifi_lable;
            lv_obj_t* bluetooth_lable;
            lv_obj_t* bat_icon_lable;
            lv_obj_t* bat_font_lable;
            lv_obj_t* time_hm_lable;
        };

        struct watchdial_lv_time_t{
            lv_obj_t* hm_lable;
            lv_obj_t* mw_lable;
        };

        private:
            const char* TAG = WATCHDIAL_TAG_NAME;
            struct watchdial_lv_screen_t lv_screen;
            struct watchdial_lv_bg_t lv_bg;
            struct watchdial_lv_status_t lv_status;
            struct watchdial_lv_time_t lv_time;

            static void tileview_event_cb(lv_event_t * e);

            static void bg_img_event_cb(lv_event_t* e);
            static void get_lv_bg(struct watchdial_lv_bg_t* lv_bg, struct watchdial_lv_screen_t* lv_screen, void* arg);
            static void set_lv_bg(struct watchdial_lv_bg_t* lv_bg, uint8_t* data);

            static void get_lv_status(struct watchdial_lv_status_t* lv_status, struct watchdial_lv_screen_t* lv_screen);
            static void set_lv_status_time(struct watchdial_lv_status_t* lv_status, struct tm* tm);
            static void set_lv_status_bat(struct watchdial_lv_status_t* lv_status, int percent, bool is_charge);
            static void set_lv_status_bluetooth(struct watchdial_lv_status_t* lv_status, bool is_bluetooth);
            static void set_lv_status_wifi(struct watchdial_lv_status_t* lv_status, bool is_wifi);
            
            static void get_lv_time(struct watchdial_lv_time_t* lv_time, struct watchdial_lv_screen_t* lv_screen);
            static void set_lv_time(struct watchdial_lv_time_t* lv_time, struct tm* tm);
        public:
            WatchDial(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* tile);
            ~WatchDial();

            /*生命周期回调*/
            void onCreate() override;
            void onShow() override;
            void onForeground() override;
            void onBackground() override;
            void onHide() override;
            void onDestroy() override;
    };


}






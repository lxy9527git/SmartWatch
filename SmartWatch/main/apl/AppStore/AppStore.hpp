/**
 * @file AppStore.hpp
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
#include "bll.hpp"

namespace apl{

    class AppStore : public mooncake::AppAbility
    {
        #define APPSTORE_TAG_NAME                               "AppStore"

        #define APPSTORE_LV_MARQUEE_ICON_W                      (40)
        #define APPSTORE_LV_MARQUEE_ICON_H                      (40)
        #define APPSTORE_LV_MARQUEE_W                           (DISPLAY_WIDTH)
        #define APPSTORE_LV_MARQUEE_H                           (50)
        #define APPSTORE_LV_MARQUEE_MOVE_SPEED                  (2)

        typedef void (*lv_marquee_icon_event_cb_t)(lv_event_t* e);

        struct appstore_lv_screen_t{
            lv_obj_t* screen;
            lv_obj_t* tileview;
            lv_obj_t* tile;
            void* icon_cb_data;
            lv_marquee_icon_event_cb_t icon_cb;
        };

        struct appstore_lv_bg_t{
            int start_jpeg_index;
            int end_jpeg_index;
            int cur_jpeg_index;
            lv_obj_t* img;
            lv_image_dsc_t img_dsc;
            struct fml::JpegDecoder::jpeg_output_info_t jpeg_src;
        };

        struct appstore_lv_marquee_t{
            lv_obj_t* container_top;
            lv_obj_t* container_bottom;
            lv_timer_t* timer;
        };

        private:
            const char* TAG = APPSTORE_TAG_NAME;
            struct appstore_lv_screen_t lv_screen;
            struct appstore_lv_bg_t lv_bg;
            struct appstore_lv_marquee_t lv_marquee;

            static void bg_img_event_cb(lv_event_t* e);
            static void get_lv_bg(struct appstore_lv_bg_t* lv_bg, struct appstore_lv_screen_t* lv_screen, void* arg);
            static void set_lv_bg(struct appstore_lv_bg_t* lv_bg, uint8_t* data);

            static void lv_marquee_timer_cb(lv_timer_t *timer);
            static void get_lv_marquee(struct appstore_lv_marquee_t* lv_marquee, struct appstore_lv_screen_t* lv_screen, void* user_data);
        public:
            AppStore(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* tile, lv_marquee_icon_event_cb_t _icon_cb, void* _icon_cb_data);
            ~AppStore();

            /*生命周期回调*/
            void onCreate() override;
            void onOpen() override;
            void onRunning() override;
            void onSleeping() override;
            void onClose() override;
            void onDestroy() override;
    };
}






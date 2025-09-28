/**
 * @file Setting.hpp
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

    class Setting : public mooncake::AppAbility
    {
        #define SETTING_LV_CONTAINER_W                          (DISPLAY_WIDTH)
        #define SETTING_LV_CONTAINER_H                          (DISPLAY_HEIGHT)

        #define SETTING_LV_ITEM_HEIGHT                          40
        #define SETTING_LV_ITEM_SPACING                         20
        #define SETTING_LV_ITEM_WIDTH                           (SETTING_LV_CONTAINER_W - 40)

        #define SETTING_LV_PROMPT_W                             (SETTING_LV_CONTAINER_W)
        

        struct setting_lv_screen_t{
            lv_obj_t* screen;
            lv_obj_t* tileview;
            lv_obj_t* tile;
        };

        struct setting_lv_prompt_t{
            lv_obj_t* prompt;
        };

        private:
            const char* TAG = "Setting";
            struct setting_lv_screen_t lv_screen;
            struct setting_lv_prompt_t lv_prompt;
            lv_obj_t* container;
            lv_obj_t* slider_volume;
            lv_obj_t* slider_brightness;
            lv_obj_t* switch_wifi_config;
            lv_obj_t* switch_wifi_onoff;
            lv_obj_t* btn_clear_wifi;
            lv_obj_t* label_volume;
            lv_obj_t* label_brightness;
            lv_obj_t* label_wifi_config;
            lv_obj_t* label_wifi_onoff;

            static void lv_prompt_event_cb(lv_event_t * e);
            static void get_lv_prompt(struct setting_lv_prompt_t* lv_prompt, struct setting_lv_screen_t* lv_screen);
            static void set_lv_prompt(struct setting_lv_prompt_t* lv_prompt, struct setting_lv_screen_t* lv_screen, const char* text);

            static void slider_volume_event_cb(lv_event_t * e);
            static void slider_brightness_event_cb(lv_event_t * e);
            static void switch_wifi_config_event_cb(lv_event_t * e);
            static void switch_wifi_onoff_event_cb(lv_event_t * e);
            static void btn_clear_wifi_event_cb(lv_event_t * e);

            void create_ui_elements();
        public:
            Setting(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* tile);
            ~Setting();

            /*生命周期回调*/
            void onCreate() override;
            void onOpen() override;
            void onRunning() override;
            void onSleeping() override;
            void onClose() override;
            void onDestroy() override;
    };

}



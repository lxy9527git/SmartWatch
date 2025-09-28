/**
 * @file apl.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <stdio.h>
#include <mooncake.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "WatchDial.hpp"
#include "AppStore.hpp"
#include "Assistant.hpp"
#include "Painter.hpp"
#include "GamePad.hpp"
#include "Setting.hpp"

namespace apl{
    
    class apl
    {
        #define APL_SCREEN_W                                            (DISPLAY_WIDTH)
        #define APL_SCREEN_H                                            (DISPLAY_HEIGHT)

        #define APL_LV_BOOT_W                                           (APL_SCREEN_W)
        #define APL_LV_BOOT_H                                           (30)
        #define APL_LV_BOOT_X                                           (0)
        #define APL_LV_BOOT_Y                                           (APL_SCREEN_H/2)

        #define APL_TILEVIEW_TILE_COL_ID_WATCHDIAL                      (0)
        #define APL_TILEVIEW_TILE_ROW_ID_WATCHDIAL                      (1)

        #define APL_TILEVIEW_TILE_COL_ID_APPSTORE                       (1)
        #define APL_TILEVIEW_TILE_ROW_ID_APPSTORE                       (1)

        #define APL_TILEVIEW_TILE_COL_ID_SETTING                        (1)
        #define APL_TILEVIEW_TILE_ROW_ID_SETTING                        (0)

        #define APL_TILEVIEW_TILE_COL_ID_OVERLAP                        (2)
        #define APL_TILEVIEW_TILE_ROW_ID_OVERLAP                        (1)

        struct apl_lv_boot_t{      
            lv_obj_t* background;
            lv_obj_t* label;
        };

        typedef enum {
            APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_ASSISTANT = 0,
            APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_PAINTER,
            APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_GAMEPAD,
        }APL_TILEVIEW_TILE_OVERLAP_CONTAINER_CONTAINER_DISPLAY_E;

        private:
            const char* TAG = "apl";
            mooncake::Mooncake mc;
            lv_obj_t* screen;
            lv_obj_t* tileview;
            lv_obj_t* WatchDial_tile;
            lv_obj_t* AppStore_tile;
            lv_obj_t* Setting_tile;
            lv_obj_t* OverLap_tile;
            lv_obj_t* OverLap_tile_container_Assistant;
            lv_obj_t* OverLap_tile_container_Painter;
            lv_obj_t* OverLap_tile_container_Gamepad;
            APL_TILEVIEW_TILE_OVERLAP_CONTAINER_CONTAINER_DISPLAY_E cur_overlap_container_disp;
            
            int WatchDial_id;
            int AppStore_id;
            int Setting_id;
            int Assistant_id;
            int Painter_id;
            int Gamepad_id;
            struct apl_lv_boot_t lv_boot;
            struct fml::SpeechRecongnition::sr_result_t sr_result;
            char *cmd_phoneme[sizeof(sr_cmd)/sizeof(sr_cmd[0])] = {
                sr_cmd[0].cmd,
                sr_cmd[1].cmd,
                sr_cmd[2].cmd,
                sr_cmd[3].cmd,
                sr_cmd[4].cmd,
            };
            /*私有构造函数，禁止外部直接实例化*/
            apl();
            ~apl();
            /*禁止拷贝构造和赋值操作*/
            apl(const apl&) = delete;
            apl& operator = (const apl&) = delete;
            static void tileview_event_cb(lv_event_t * e);
            static void JpegDecoderInputInfoCallBack(void* input_info_user_data, int Number, int index);
            static int get_m_audio(std::vector<int16_t>& data);
            static int set_m_audio(std::vector<int16_t>& data);
            static void lv_marquee_icon_event_cb(lv_event_t * e);

            void init_lv_boot();
        public:
            /*获取单例实例的静态方法*/
            inline static apl& getInstance() {
                static apl instance;
                return instance;
            }

            void init();
            void update();
            void tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_CONTAINER_DISPLAY_E cur_disp);
    };

}





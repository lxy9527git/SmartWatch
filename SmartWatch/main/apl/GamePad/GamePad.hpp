/**
 * @file GamePad.hpp
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
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <queue>
#include "bll.hpp"


namespace apl{

    class GamePad : public mooncake::AppAbility
    {
        #define GAMEPAD_LV_CONTAINER_W                                  (DISPLAY_WIDTH)
        #define GAMEPAD_LV_CONTAINER_H                                  (DISPLAY_HEIGHT)

        #define GAMEPAD_LV_ITEM_HEIGHT                                  (40)
        #define GAMEPAD_LV_ITEM_SPACING                                 (20)
        #define GAMEPAD_LV_ITEM_WIDTH                                   (GAMEPAD_LV_CONTAINER_W - 40)

        #define GAMEPAD_WIFI_STATE_NONE                                 (0)
        #define GAMEPAD_WIFI_STATE_AP                                   (1)
        #define GAMEPAD_WIFI_STAET_STA                                  (2)

        #define GAMEPAD_BUTTON_HOLD_TIME_MS                             (150)

        /*摇杆相关定义*/
        #define JOYSTICK_RADIUS                                         (60)                    /*摇杆半径*/
        #define JOYSTICK_BG_RADIUS                                      (80)                    /*摇杆背景半径*/

        #define JOYSTICK_CENTER_X                                       (DISPLAY_WIDTH / 2)     /*摇杆居中*/
        #define JOYSTICK_CENTER_Y                                       (DISPLAY_HEIGHT - JOYSTICK_BG_RADIUS - 20)
        
        struct gamepad_lv_screen_t{
            lv_obj_t* screen;
            lv_obj_t* tileview;
            lv_obj_t* container;
        };

        /*摇杆结构体*/
        struct Joystick {
            lv_obj_t* bg;           /*背景圆*/
            lv_obj_t* stick;        /*摇杆圆*/
            lv_point_t center;      /*中心点坐标*/
            bool is_active;         /*是否激活*/
            uint16_t lx;            /*左摇杆X值*/
            uint16_t ly;            /*左摇杆Y值*/
        };


        private:
            const char* TAG = "GamePad";
            struct gamepad_lv_screen_t lv_screen;
            lv_obj_t *main_cont;                                        /*创建主容器*/
            lv_obj_t *title_bar;                                        /*顶部标题栏*/
            lv_obj_t *title_label;                                      /*标题*/    
            uint8_t wifi_state;                                         /*保存当前wifi状态*/
            std::string records;                                        /*存储在flash里的所有记录，格式：pinyin-数字;  举例: ling-0;er-2;sha-32310; 以;分割各个记录*/
            std::map<std::string, std::string> pinyin_map;              /*存储拼音到数字的映射*/
            std::queue<std::pair<uint8_t, uint32_t>> buttonQueue;       /*存储按钮操作和时间的队列*/
            std::vector<std::string> registered_cmds;                   /*存储当前已注册的命令词*/
            uint32_t lastButtonTime;                                    /*记录上次按钮操作的时间*/
            uint32_t button_hold_time_ms;                               /*按钮保持时间（毫秒）*/

            /*配置界面相关变量*/
            lv_obj_t *config_cont;                                      /*配置界面容器*/
            lv_obj_t *list;                                             /*拼音映射列表*/
            lv_obj_t *kb;                                               /*键盘控件*/
            lv_obj_t *pinyin_ta;                                        /*拼音文本输入区域*/
            lv_obj_t *number_ta;                                        /*数字文本输入区域*/
            lv_obj_t *edit_cont;                                        /*编辑容器*/
            std::string current_edit_key;                               /*当前编辑的拼音*/
            bool is_editing;                                            /*是否处于编辑模式*/
            bool is_adding;                                             /*是否处于添加模式*/

            /*摇杆相关变量*/
            Joystick joystick;                                          /*左摇杆*/

            /*无限重复相关变量*/
            bool is_infinite_repeat;
            std::string repeat_sequence;
            uint32_t lastRepeatTime;
            uint32_t repeat_interval_ms;                     
            
            /*开关控制相关变量*/
            lv_obj_t *button_switch;                                    /*按钮发送开关*/
            lv_obj_t *joystick_switch;                                  /*摇杆发送开关*/
            bool button_enabled;                                        /*按钮发送是否启用*/
            bool joystick_enabled;                                      /*摇杆发送是否启用*/

            static std::string trim(const std::string& str);
            static void get_sr_pinyin(void* user_data, char* pinyin);
            static void list_event_cb(lv_event_t * e);                  /*列表事件回调*/
            static void kb_event_cb(lv_event_t * e);                    /*键盘事件回调*/
            static void add_btn_event_cb(lv_event_t * e);               /*添加按钮事件回调*/
            static void save_btn_event_cb(lv_event_t * e);              /*保存按钮事件回调*/
            static void cancel_btn_event_cb(lv_event_t * e);            /*取消按钮事件回调*/
            static void delete_btn_event_cb(lv_event_t * e);            /*删除按钮事件回调*/
            static void time_slider_event_cb(lv_event_t * e);           /*时间滑块事件回调*/
            static void joystick_event_cb(lv_event_t * e);              /*摇杆事件回调*/
            static void button_switch_event_cb(lv_event_t * e);         /*按钮开关事件回调*/
            static void joystick_switch_event_cb(lv_event_t * e);       /*摇杆开关事件回调*/
            void parse_records();                                       /*解析records字符串到pinyin_map*/
            void update_config_list();                                  /*更新配置列表显示*/
            void save_records();                                        /*保存records到flash*/
            void show_edit_dialog(const std::string& key = "", const std::string& value = ""); /*显示编辑对话框*/
            void hide_edit_dialog();                                    /*隐藏编辑对话框*/
            void show_time_config_dialog();                             /*显示时间配置对话框*/
            void show_button_switch_dialog();                           /*显示按钮发送开关配置对话框**/
            void show_joystick_switch_dialog();                         /*显示摇杆发送开关配置对话框**/
            void hide_time_config_dialog();                             /*隐藏时间配置对话框*/
            void create_joystick();                                     /*创建摇杆*/
            void update_joystick_position(Joystick& joystick, lv_point_t point); /*更新摇杆位置*/
            void reset_joystick(Joystick& joystick);                    /*重置摇杆位置*/
            void send_joystick_data();                                  /*发送摇杆数据*/
            void save_switch_states();                                  /*保存开关状态*/
            
        public:
            GamePad(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* container);
            ~GamePad();
            void create_ui();
            /*生命周期回调*/
            void onCreate() override;
            void onOpen() override;
            void onRunning() override;
            void onSleeping() override;
            void onClose() override;
            void onDestroy() override;
    };

}

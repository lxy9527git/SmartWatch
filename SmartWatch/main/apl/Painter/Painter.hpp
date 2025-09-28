/**
 * @file Painter.hpp
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
#include <string>
#include <algorithm>
#include <esp_http_client.h>
#include <cstring>
#include <esp_tls.h>
#include <setjmp.h> 
#include <mutex>
#include <memory>
#include "bll.hpp"

namespace apl{

    class Painter : public mooncake::AppAbility
    {
        #define PAINTER_MAX_KB_HEIGHT                                 (80)

        #define PAINTER_TITLE_BAR_HEIGHT                              (40)

        #define PAINTER_INPUT_CONT_HEIGHT                             (60)
        #define PAINTER_INPUT_TA_HEIGHT                               (40)

        #define PAINTER_SEND_BTN_WIDTH                                (60)
        #define PAINTER_SEND_BTN_HEIGHT                               (40)
        #define PAINTER_SEND_BTN_UP_COLOR                             (0x4CAF50)


        #define PAINTER_VOICE_BTN_WIDTH                               (60)
        #define PAINTER_VOICE_BTN_HEIGHT                              (40)
        #define PAINTER_VOICE_BTN_UP_COLOR                            (0x4CAF50)


        #define PAINTER_BTN_DISABLED_COLOR                            (0x9E9E9E)    /*灰色*/


        #define PAINTER_MSG_AREA_HEIGHT                               (DISPLAY_HEIGHT - PAINTER_TITLE_BAR_HEIGHT - PAINTER_INPUT_CONT_HEIGHT)

        #define PAINTER_MSG_BUBBLE_QUESTION_X                         (73)
        #define PAINTER_MSG_BUBBLE_QUESTION_Y                         (10)
        #define PAINTER_MSG_BUBBLE_QUESTION_W                         (142)
        #define PAINTER_MSG_BUBBLE_QUESTION_H                         (33)

        #define PAINTER_MSG_BUBBLE_ANSWER_X                           (21)
        #define PAINTER_MSG_BUBBLE_ANSWER_Y                           (78)
        #define PAINTER_MSG_BUBBLE_ANSWER_W                           (120)         /*需要8的倍数，不然解码会失败*/
        #define PAINTER_MSG_BUBBLE_ANSWER_H                           (96)          /*需要8的倍数，不然解码会失败*/    

        #define PAINTER_MAX_IMAGE_SIZE                                (300*1024) 

        #define PAINTER_JPEG_DECODE_TASK_PRIOR                        (2)
        #define PAINTER_JPEG_DECODE_TASK_CORE                         (1)

        #define PAINTER_JPEG_DOWNLOAD_TASK_PRIOR                      (2)
        #define PAINTER_JPEG_DOWNLOAD_TASK_CORE                       (1)

        

        struct painter_lv_screen_t{
            lv_obj_t* screen;
            lv_obj_t* tileview;
            lv_obj_t* container;
        };

        /*消息结构体*/
        typedef struct {
            const char *text;
            int is_me;          /*1: 自己发送的消息, 0: 接收的消息*/
        } ChatMessage;

        struct AsyncData {
            Painter* app;
            char* text;
        };

        struct DecodeTaskData {
            Painter* app;
            std::vector<uint8_t> buffer;
        };

        private:
            const char* TAG = "Painter";
            struct painter_lv_screen_t lv_screen;
            lv_obj_t *main_cont;                            /*创建主容器*/
            lv_obj_t *title_bar;                            /*顶部标题栏*/
            lv_obj_t *title_label;                          /*标题*/    
            lv_obj_t *msg_cont;                             /*消息容器*/
            lv_obj_t *input_cont;                           /*底部输入区域*/
            lv_obj_t *input_ta;                             /*输入框*/
            lv_obj_t *send_btn;                             /*发送按钮*/
            lv_obj_t *send_label;                           /*发送图标*/
            lv_obj_t *voice_btn;                            /*语音按钮*/
            lv_obj_t *voice_label;                          /*语音按钮图标*/
            lv_obj_t *pinyin_ime;                           /*拼音输入法对象*/
            lv_obj_t *kb;                                   /*键盘对象*/
            lv_obj_t *cand_panel;                           /*汉字候选框*/

            /*JPEG 解码相关成员*/
            lv_img_dsc_t img_dsc;
            std::vector<uint8_t> download_buffer;
            lv_obj_t* current_image;
            std::string image_url;
            bool is_jpeg_valid;                             /*JPEG有效性标志*/
            size_t downloaded_size;                         /*已下载字节数*/
            volatile bool abort_tasks;                      /*任务终止标志*/
            TaskHandle_t decode_task_handle;                /*解码任务句柄*/
            TaskHandle_t download_task_handle;              /*下载任务句柄*/
            std::mutex btn_mutex;                           /*按钮状态互斥锁*/
            bool is_send_btn_busy;                          /*发送按钮忙状态*/
            bool is_voice_btn_busy;                         /*语音按钮忙状态*/

            static lv_obj_t* create_msg_bubble(lv_obj_t *parent, const char *text, int is_me);
            static void get_ai_answer(void* user_data, char* answer);
            static void send_btn_event_cb(lv_event_t *e);
            static void get_sr_pinyin(void* user_data, char* pinyin);
            static void voice_btn_event_cb(lv_event_t *e);
            static void ta_event_cb(lv_event_t *e);
            static void jpeg_decode_task(void* arg);  
            void reset();
            void start_image_download(const char* url);
            bool decode_jpeg();
            void scale_and_display_image();
            void create_image_bubble(const char* url);
            static esp_err_t http_event_handler(esp_http_client_event_t *evt);
            void download_image();
            bool verify_jpeg_signature();
            void set_send_btn_busy(bool enabled);
            void set_voice_btn_busy(bool enabled);
        public:
            Painter(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* container);
            ~Painter();
            void add_message(const char *text, int is_me);
            void init_pinyin_input();
            void create_chat_ui();
            /*生命周期回调*/
            void onCreate() override;
            void onOpen() override;
            void onRunning() override;
            void onSleeping() override;
            void onClose() override;
            void onDestroy() override;
    };
}


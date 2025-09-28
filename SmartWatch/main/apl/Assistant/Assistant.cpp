/**
 * @file Assistant.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "Assistant.hpp"

namespace apl{
    /*创建消息气泡*/
    lv_obj_t* Assistant::create_msg_bubble(lv_obj_t *parent, const char *text, int is_me)
    {
        /*创建主容器*/
        lv_obj_t *cont = lv_obj_create(parent);

        /*设置边框宽度为0 - 消除边框*/
        lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

        /*根据发送者设置固定位置和大小*/
        if (is_me) {
            /*自己发送的消息 - 固定位置和大小*/
            lv_obj_set_size(cont, ASSISTANT_MSG_BUBBLE_QUESTION_W, ASSISTANT_MSG_BUBBLE_QUESTION_H); 
            lv_obj_set_pos(cont, ASSISTANT_MSG_BUBBLE_QUESTION_X, ASSISTANT_MSG_BUBBLE_QUESTION_Y);   
            
            /*样式设置*/
            lv_obj_set_style_text_color(cont, lv_color_black(), 0);
        } else {
            /*对方发送的消息 - 固定位置和大小*/
            lv_obj_set_size(cont, ASSISTANT_MSG_BUBBLE_ANSWER_W, ASSISTANT_MSG_BUBBLE_ANSWER_H); 
            lv_obj_set_pos(cont, ASSISTANT_MSG_BUBBLE_ANSWER_X, ASSISTANT_MSG_BUBBLE_ANSWER_Y);  
            
            /*样式设置*/
            lv_obj_set_style_text_color(cont, lv_color_black(), 0);
        }

        /*创建消息文本*/
        lv_obj_t *msg_label = lv_label_create(cont);
        lv_label_set_text(msg_label, text);
        lv_obj_set_width(msg_label, lv_pct(100));
        lv_obj_set_style_text_font(msg_label, &MyFonts16, 0);
        lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP); /*自动换行*/
        lv_obj_set_scrollbar_mode(msg_label, LV_SCROLLBAR_MODE_OFF);
        lv_obj_align(msg_label, LV_ALIGN_TOP_MID, 0, -10);

        return cont;
    }

    void Assistant::get_ai_answer(void* user_data, char* answer)
    {
        Assistant* app = static_cast<Assistant*>(user_data);
        /*恢复发送按钮状态*/
        app->set_send_btn_busy(false);
        lv_async_call([](void* data){
             AsyncData* d = static_cast<AsyncData*>(data);
            d->app->add_message(d->text, 0);
            free(d->text);  /*释放内存*/
            delete d;       /*删除数据对象*/
        }, new AsyncData{app, strdup(answer)});
    }
    /*发送按钮事件处理*/
    void Assistant::send_btn_event_cb(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        Assistant* app = (Assistant*)lv_event_get_user_data(e);
        if (code == LV_EVENT_CLICKED) {
            /*更新UI状态*/
            app->set_send_btn_busy(true);
            /*清空消息容器*/
            if (app->msg_cont) {
                lv_obj_clean(app->msg_cont);             /*删除所有子对象（消息气泡）*/
            }
            const char *text = lv_textarea_get_text(app->input_ta);
            if (text && text[0] != '\0') {
                /*模拟发送消息*/
                app->add_message(text, 1);
                bll::ArtificialIntelligence::getInstance().ask_question(text, get_ai_answer, app);
                lv_textarea_set_text(app->input_ta, "");
                lv_obj_add_flag(app->kb, LV_OBJ_FLAG_HIDDEN); /*发送后隐藏键盘*/
            }else {
                /*如果没有文本，恢复按钮状态*/
                app->set_send_btn_busy(false);
            }
        }
    }
    
    /*获取SR的拼音字符串*/
    void Assistant::get_sr_pinyin(void* user_data, char* pinyin)
    {
        Assistant* app = static_cast<Assistant*>(user_data);

        /*先恢复按钮状态*/
        app->set_voice_btn_busy(false);

        std::string question(pinyin);
        question.append(",将这段拼音转为文字，不要调用工具，只要拼音的文字，其余不要。");
        
        /*设置发送按钮为忙状态（因为接下来会调用AI）*/
        app->set_send_btn_busy(true);
        
        
        bll::ArtificialIntelligence::getInstance().ask_question(question.c_str(), [](void* user_data, char* answer){
            Assistant* app = static_cast<Assistant*>(user_data);
             /*恢复发送按钮状态*/
            app->set_send_btn_busy(false);

            lv_async_call([](void* data){
                AsyncData* d = static_cast<AsyncData*>(data);
                lv_textarea_set_text(d->app->input_ta, d->text);
                free(d->text);  /*释放内存*/
                delete d;       /*删除数据对象*/
            }, new AsyncData{app, strdup(answer)});
        }, user_data);
    }
    /*语音按钮事件处理*/
    void Assistant::voice_btn_event_cb(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        Assistant* app = (Assistant*)lv_event_get_user_data(e);
        if (code == LV_EVENT_CLICKED) {
            /*更新UI状态*/
            app->set_voice_btn_busy(true);
            fml::SpeechRecongnition::getInstance().sr_multinet_clean(get_sr_pinyin, app);
        }
    }
    /*输入框事件回调*/
    void Assistant::ta_event_cb(lv_event_t *e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        Assistant* app = (Assistant*)lv_event_get_user_data(e);

        if (code == LV_EVENT_FOCUSED) {
            if (!app->pinyin_ime || !app->kb) return;
            /*隐藏消息容器*/
            lv_obj_add_flag(app->msg_cont, LV_OBJ_FLAG_HIDDEN);

            /*将键盘与输入框关联*/
            lv_keyboard_set_textarea(app->kb, app->input_ta);
            
            /*显示键盘*/
            lv_obj_clear_flag(app->kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(app->cand_panel, LV_OBJ_FLAG_HIDDEN);

            /*调整键盘位置*/
            lv_obj_set_y(app->kb, lv_obj_get_y(app->input_ta) - lv_obj_get_height(app->kb) - 10);
            lv_obj_move_foreground(app->kb);

            lv_obj_align_to(app->cand_panel, app->kb, LV_ALIGN_OUT_TOP_MID, 0, 0);
        } 
        else if (code == LV_EVENT_DEFOCUSED) {
            /*显示消息容器*/
            lv_obj_clear_flag(app->msg_cont, LV_OBJ_FLAG_HIDDEN);
            /*隐藏键盘*/
            lv_obj_add_flag(app->kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(app->cand_panel, LV_OBJ_FLAG_HIDDEN);
            /* 恢复输入区域位置 */
            lv_obj_set_y(lv_obj_get_parent(app->input_ta), lv_obj_get_height(lv_scr_act()) - ASSISTANT_MAX_KB_HEIGHT - 10);       /*底部位置*/
        }
    }

    void Assistant::reset()
    {
        /*1. 清空消息容器*/
        if (msg_cont) {
            lv_obj_clean(msg_cont);/*删除所有子对象（消息气泡）*/
        }
        /*2. 隐藏键盘和候选面板*/
        if (kb) {
            lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
        if (cand_panel) {
            lv_obj_add_flag(cand_panel, LV_OBJ_FLAG_HIDDEN);
        }
        /*3. 清空输入框*/
        if (input_ta) {
            lv_textarea_set_text(input_ta, "");
        }
        /*4. 重置按钮状态*/
        set_send_btn_busy(false);
        set_voice_btn_busy(false);
        /*5. 确保消息容器可见*/
        if (msg_cont) {
            lv_obj_clear_flag(msg_cont, LV_OBJ_FLAG_HIDDEN);
        }
    }

    /*设置发送按钮状态*/
    void Assistant::set_send_btn_busy(bool enabled) {
        /*设置按钮状态*/
        {
            std::lock_guard<std::mutex> lock(btn_mutex);
            is_send_btn_busy = enabled;
        }
        lv_async_call([](void* arg) {
            Assistant* app = static_cast<Assistant*>(arg);
            if (!app->is_send_btn_busy) {
                lv_obj_set_style_bg_color(app->send_btn, lv_color_hex(ASSISTANT_SEND_BTN_UP_COLOR), 0);
                lv_obj_clear_state(app->send_btn, LV_STATE_DISABLED);
            } else {
                lv_obj_set_style_bg_color(app->send_btn, lv_color_hex(ASSISTANT_BTN_DISABLED_COLOR), 0);
                lv_obj_add_state(app->send_btn, LV_STATE_DISABLED);
            }
        }, this);
    }

    /*设置语音按钮状态*/
    void Assistant::set_voice_btn_busy(bool enabled) {
        /*设置按钮状态*/
        {
            std::lock_guard<std::mutex> lock(btn_mutex);
            is_voice_btn_busy = enabled;
        }
        lv_async_call([](void* arg) {
            Assistant* app = static_cast<Assistant*>(arg);
            if (!app->is_voice_btn_busy) {
                lv_obj_set_style_bg_color(app->voice_btn, lv_color_hex(ASSISTANT_VOICE_BTN_UP_COLOR), 0);
                lv_obj_clear_state(app->voice_btn, LV_STATE_DISABLED);
            } else {
                lv_obj_set_style_bg_color(app->voice_btn, lv_color_hex(ASSISTANT_BTN_DISABLED_COLOR), 0);
                lv_obj_add_state(app->voice_btn, LV_STATE_DISABLED);
            }
        }, this);
    }

    Assistant::Assistant(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* container)
    {
        memset(&lv_screen, 0, sizeof(lv_screen));
        lv_screen.screen = screen;
        lv_screen.tileview = tileview;
        lv_screen.container = container;

        main_cont = NULL;
        title_bar = NULL;
        title_label = NULL;
        msg_cont = NULL;
        input_cont = NULL;
        input_ta = NULL;
        send_btn = NULL;
        send_label = NULL;
        voice_btn = NULL;
        voice_label = NULL;
        pinyin_ime = NULL;
        kb = NULL;
        is_send_btn_busy = false;
        is_voice_btn_busy = false;
        ESP_LOGI(TAG, "Assistant on construct");
    }

    Assistant::~Assistant()
    {
        reset();
        if(main_cont != NULL)lv_obj_del(main_cont);
        ESP_LOGI(TAG, "Assistant on deconstruct");
    }
    /*添加消息到容器*/
    void Assistant::add_message(const char *text, int is_me)
    {
        /*创建消息气泡 - 位置已在 create_msg_bubble 中固定*/
        create_msg_bubble(msg_cont, text, is_me);
    }
    /*初始化拼音输入法*/
    void Assistant::init_pinyin_input()
    {
        /*创建拼音输入法*/
        pinyin_ime = lv_ime_pinyin_create(lv_screen.container);
        lv_obj_set_style_text_font(pinyin_ime, &MyFonts16, 0);
        lv_ime_pinyin_set_dict(pinyin_ime, MyFonts16Dict);
        /*设置候选框中文字体*/
        cand_panel = lv_ime_pinyin_get_cand_panel(pinyin_ime);
        lv_obj_set_style_text_font(cand_panel, &MyFonts16, 0);
        lv_obj_set_size(cand_panel, LV_PCT(100), LV_PCT(10));
        /*创建键盘*/
        kb = lv_keyboard_create(lv_screen.container);
        lv_obj_set_size(kb, LV_PCT(100), ASSISTANT_MAX_KB_HEIGHT + 20);      
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);    /*初始隐藏*/
        lv_obj_set_style_bg_color(kb, lv_color_hex(0xF0F0F0), 0);
        /* 设置键盘内容高度 */
        lv_obj_set_content_height(kb, ASSISTANT_MAX_KB_HEIGHT);
        lv_keyboard_set_textarea(kb, input_ta);
        
        /* 绑定输入法 */
        if (pinyin_ime && kb) {
            lv_ime_pinyin_set_keyboard(pinyin_ime, kb);
        }
    }
    /*创建聊天界面*/
    void Assistant::create_chat_ui()
    {
        /*创建主容器*/
        main_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
        lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(main_cont, 0, 0);
        lv_obj_set_style_border_width(main_cont, 0, 0);
        lv_obj_set_style_bg_color(main_cont, lv_color_hex(0xF5F5F5), 0);
        lv_obj_set_style_bg_image_src(main_cont, &_assistant_bg_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(main_cont, LV_OPA_COVER, 0);              /*设置不透明度*/
        lv_obj_set_style_bg_image_tiled(main_cont, false, 0);                   /*禁止平铺*/
        lv_obj_set_style_bg_image_recolor_opa(main_cont, LV_OPA_TRANSP, 0);     /*禁用颜色覆盖*/
        lv_obj_clear_flag(main_cont, LV_OBJ_FLAG_SCROLLABLE);                   /*禁用滚动*/
        /*顶部标题栏*/
        title_bar = lv_obj_create(main_cont);
        lv_obj_set_size(title_bar, lv_pct(100), ASSISTANT_TITLE_BAR_HEIGHT);
        lv_obj_set_style_bg_opa(title_bar, LV_OPA_TRANSP, 0); 
        lv_obj_set_style_border_width(title_bar, 0, 0);
        lv_obj_set_style_pad_all(title_bar, 0, 0);
        /*标题*/
        title_label = lv_label_create(title_bar);
        lv_label_set_text(title_label, "小助手");
        lv_obj_set_style_text_font(title_label, &MyFonts16, 0);
        lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 10);
        /*创建消息容器*/
        msg_cont = lv_obj_create(main_cont);
        lv_obj_set_size(msg_cont, lv_pct(100), ASSISTANT_MSG_AREA_HEIGHT); 
        lv_obj_set_style_border_width(msg_cont, 0, 0);
        lv_obj_set_style_bg_opa(msg_cont, LV_OPA_TRANSP, 0); 
        lv_obj_set_style_pad_all(msg_cont, 0, 0);
        lv_obj_clear_flag(msg_cont, LV_OBJ_FLAG_SCROLLABLE); /*禁用滚动*/
        /*底部输入区域*/
        input_cont = lv_obj_create(main_cont);
        lv_obj_set_size(input_cont, lv_pct(100), ASSISTANT_INPUT_CONT_HEIGHT);
        lv_obj_set_style_border_width(input_cont, 1, 0);
        lv_obj_set_style_border_color(input_cont, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_bg_opa(input_cont, LV_OPA_TRANSP, 0); 
        lv_obj_set_flex_flow(input_cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(input_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(input_cont, 5, 0); /*内边距减小*/
        /*语音按钮*/
        voice_btn = lv_btn_create(input_cont);
        lv_obj_set_size(voice_btn, ASSISTANT_SEND_BTN_WIDTH, ASSISTANT_SEND_BTN_HEIGHT);
        lv_obj_set_style_bg_color(voice_btn, lv_color_hex(ASSISTANT_SEND_BTN_UP_COLOR), 0); 
        lv_obj_set_style_radius(voice_btn, 8, 0);
        voice_label = lv_label_create(voice_btn);
        lv_label_set_text(voice_label, LV_SYMBOL_VOLUME_MAX); 
        lv_obj_set_style_text_font(voice_label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(voice_label, lv_color_white(), 0);
        lv_obj_center(voice_label);
        lv_obj_add_event_cb(voice_btn, voice_btn_event_cb, LV_EVENT_CLICKED, this);
        /*输入框*/
        input_ta = lv_textarea_create(input_cont);
        lv_obj_set_flex_grow(input_ta, 1);
        lv_obj_set_height(input_ta, ASSISTANT_INPUT_TA_HEIGHT);
        lv_textarea_set_placeholder_text(input_ta, "写入问题");
        lv_textarea_set_one_line(input_ta, true);
        lv_obj_set_style_text_font(input_ta, &MyFonts16, 0);
        lv_obj_add_event_cb(input_ta, ta_event_cb, LV_EVENT_ALL, this);
        /*发送按钮*/
        send_btn = lv_btn_create(input_cont);
        lv_obj_set_size(send_btn, ASSISTANT_SEND_BTN_WIDTH, ASSISTANT_SEND_BTN_HEIGHT);
        lv_obj_set_style_bg_color(send_btn, lv_color_hex(ASSISTANT_VOICE_BTN_UP_COLOR), 0);
        lv_obj_set_style_radius(send_btn, 8, 0);
        send_label = lv_label_create(send_btn);
        lv_label_set_text(send_label, LV_SYMBOL_GPS);
        lv_obj_set_style_text_font(send_label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(send_label, lv_color_white(), 0);
        lv_obj_center(send_label);
        lv_obj_add_event_cb(send_btn, send_btn_event_cb, LV_EVENT_CLICKED, this);
    }

    void Assistant::onCreate()
    {
        create_chat_ui();
        init_pinyin_input();
        ESP_LOGI(TAG, "Assistant on create");
    }

    void Assistant::onOpen()
    {
        fml::HdlManager::getInstance().set_no_sleep_for_lvgl();
        bll::ArtificialIntelligence::getInstance().reset();
        reset();
        ESP_LOGI(TAG, "Assistant on Open");
    }

    void Assistant::onRunning()
    {
        //ESP_LOGI(TAG, "Assistant on Running");
    }

    void Assistant::onSleeping()
    {
        //ESP_LOGI(TAG, "Assistant on Sleeping");
    }

    void Assistant::onClose()
    {
        fml::HdlManager::getInstance().clear_no_sleep_for_lvgl();
        reset();
        ESP_LOGI(TAG, "Assistant on Close");
    }

    void Assistant::onDestroy()
    {
        ESP_LOGI(TAG, "Assistant on destroy");
    }

}

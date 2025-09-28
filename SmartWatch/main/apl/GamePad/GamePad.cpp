/**
 * @file GamePad.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "GamePad.hpp"

namespace apl{

    /*辅助函数：去除字符串前后的空格*/
    std::string GamePad::trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) {
            return "";
        }
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    void GamePad::get_sr_pinyin(void* user_data, char* pinyin)
    {
        GamePad* app = static_cast<GamePad*>(user_data);
        /*查找records里有无对应的pinyin，有的话就获取pinyin对应的操作*/
        std::string received_pinyin = trim(pinyin); /*去除前后空格*/
        // ESP_LOGI(app->TAG, "Received pinyin: %s", received_pinyin.c_str());
        // ESP_LOG_BUFFER_HEXDUMP(app->TAG, received_pinyin.c_str(), received_pinyin.length(), ESP_LOG_INFO);
        /*在映射表中查找对应的数字*/
        auto it = app->pinyin_map.find(received_pinyin);
        if (it != app->pinyin_map.end()) {
            std::string num_str = it->second;
            ESP_LOGI(app->TAG, "Found corresponding number: %s", num_str.c_str());
            
            /*检查是否包含#字符*/
            if (num_str.find('#') != std::string::npos) {
                /*包含#字符，设置无限重复标志和序列*/
                app->is_infinite_repeat = true;
                app->repeat_sequence.clear();
                
                /*提取纯净的数字序列（去除#）*/
                for (char c : num_str) {
                    if (c >= '0' && c <= '9') {
                        app->repeat_sequence += c;
                    }
                }
                
                if (!app->repeat_sequence.empty()) {
                    ESP_LOGI(app->TAG, "Start infinite repeating: %s", app->repeat_sequence.c_str());
                }
            } else {
                /*不包含#字符，正常执行一次*/
                app->is_infinite_repeat = false; /*停止任何现有的重复*/
                for (char c : num_str) {
                    if (c >= '0' && c <= '9') {
                        uint8_t button_index = c - '0';
                        /*将按钮按下和释放操作加入队列*/
                        app->buttonQueue.push(std::make_pair(button_index, 1)); /*按下*/
                        app->buttonQueue.push(std::make_pair(button_index, 0)); /*释放，延迟button_hold_time_ms处理*/
                    }
                }
            }
        } else {
            ESP_LOGW(app->TAG, "Pinyin not found in records: %s", received_pinyin.c_str());
        }

        fml::SpeechRecongnition::getInstance().sr_multinet_clean(NULL,NULL);
    }

    void GamePad::list_event_cb(lv_event_t * e)
    {
        lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        const char * key = static_cast<const char*>(lv_obj_get_user_data(obj));
        
        if (key) {
            if (strcmp(key, "time_config") == 0) {
                app->show_time_config_dialog();
            } else if (strcmp(key, "button_switch_config") == 0) {
                app->show_button_switch_dialog();
            } else if (strcmp(key, "joystick_switch_config") == 0) {
                app->show_joystick_switch_dialog();
            } else {
                auto it = app->pinyin_map.find(key);
                if (it != app->pinyin_map.end()) {
                    app->show_edit_dialog(it->first, it->second);
                }
            }
        }
    }

    void GamePad::kb_event_cb(lv_event_t * e)
    {
        /*键盘事件处理*/
    }

    void GamePad::add_btn_event_cb(lv_event_t * e)
    {
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        app->show_edit_dialog();
    }

    void GamePad::save_btn_event_cb(lv_event_t * e)
    {
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        
        /*获取拼音和数字*/
        std::string pinyin(lv_textarea_get_text(app->pinyin_ta));
        std::string number(lv_textarea_get_text(app->number_ta));
        
        std::string pinyin_str = trim(pinyin);
        std::string number_str = trim(number);
        
        if (!pinyin_str.empty() && !number_str.empty()) {
            /*如果是编辑模式且拼音有变化，先删除旧的*/
            if (!app->is_adding && pinyin_str != app->current_edit_key) {
                app->pinyin_map.erase(app->current_edit_key);
            }
            
            /*添加或更新映射*/
            app->pinyin_map[pinyin_str] = number_str;
            
            /*保存到flash*/
            app->save_records();
            
            /*更新列表显示*/
            app->update_config_list();
        }
        
        app->hide_edit_dialog();
    }

    void GamePad::cancel_btn_event_cb(lv_event_t * e)
    {
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        app->hide_edit_dialog();
    }

    void GamePad::delete_btn_event_cb(lv_event_t * e)
    {
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        
        /*删除映射*/
        app->pinyin_map.erase(app->current_edit_key);
        
        /*保存到flash*/
        app->save_records();
        
        /*更新列表显示*/
        app->update_config_list();
        
        app->hide_edit_dialog();
    }

    void GamePad::time_slider_event_cb(lv_event_t * e)
    {
        lv_obj_t * slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        
        if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
            app->button_hold_time_ms = lv_slider_get_value(slider);
            
            /*更新标签显示*/
            lv_obj_t * label = static_cast<lv_obj_t*>(lv_obj_get_user_data(slider));
            if (label) {
                std::string text = "按钮保持时间: " + std::to_string(app->button_hold_time_ms) + "ms";
                lv_label_set_text(label, text.c_str());
            }
            
            /*保存到flash*/
            fml::HdlManager::getInstance().set_flash_int("gamepad", true, "button_time", app->button_hold_time_ms);
        }
    }

    void GamePad::joystick_event_cb(lv_event_t * e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        lv_obj_t * obj = static_cast<lv_obj_t*>(lv_event_get_target(e));
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        
        if (obj == app->joystick.bg || obj == app->joystick.stick) {
            if (code == LV_EVENT_PRESSING) {
                /*当摇杆按下时，禁用Tileview的所有滑动方向*/
                lv_obj_set_scroll_dir(app->lv_screen.tileview, LV_DIR_NONE); /*使用 LV_DIR_NONE 禁用所有方向*/
                lv_point_t point;
                lv_indev_get_point(lv_indev_get_act(), &point);
                app->update_joystick_position(app->joystick, point);
            } else if (code == LV_EVENT_RELEASED) {
                /*当摇杆释放时，恢复Tileview原有的滑动方向 */
                lv_obj_set_scroll_dir(app->lv_screen.tileview, LV_DIR_LEFT); /*根据你实际的滑动方向设置*/
                /*重置摇杆位置*/
                app->reset_joystick(app->joystick);
            }
        }
    }

    void GamePad::button_switch_event_cb(lv_event_t * e)
    {
        lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        
        if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
            app->button_enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
            app->save_switch_states();
            
            /*如果禁用按钮发送，清空按钮队列并停止无限重复*/
            if (!app->button_enabled) {
                while (!app->buttonQueue.empty()) {
                    app->buttonQueue.pop();
                }
                app->is_infinite_repeat = false;
            }
            
            /*隐藏对话框*/
            app->hide_edit_dialog();
        }
    }

    void GamePad::joystick_switch_event_cb(lv_event_t * e)
    {
        lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
        GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
        
        if(lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
            app->joystick_enabled = lv_obj_has_state(obj, LV_STATE_CHECKED);
            app->save_switch_states();
            
            /*如果禁用摇杆发送，重置摇杆位置*/
            if (!app->joystick_enabled) {
                app->reset_joystick(app->joystick);
            }
            
            /*隐藏对话框*/
            app->hide_edit_dialog();
        }
    }

    void GamePad::parse_records()
    {
        pinyin_map.clear();
        std::stringstream ss(records);
        std::string item;
        
        /*按分号分割字符串*/
        while (std::getline(ss, item, ';')) {
            /*按连字符分割拼音和数字*/
            size_t dash_pos = item.find('-');
            if (dash_pos != std::string::npos) {
                std::string pinyin = trim(item.substr(0, dash_pos)); /*去除前后空格*/
                std::string number = trim(item.substr(dash_pos + 1)); /*去除前后空格*/
                
                /*存储到映射表*/
                pinyin_map[pinyin] = number;
                //ESP_LOGI(TAG, "Parsed: %s -> %s", pinyin.c_str(), number.c_str());
            }
        }
    }

    void GamePad::update_config_list()
    {
        lv_obj_clean(list);

        /*添加时间配置按钮*/
        lv_obj_t * time_btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, ("按钮时间: " + std::to_string(button_hold_time_ms) + "ms").c_str());
        lv_obj_set_user_data(time_btn, const_cast<char*>("time_config"));
        lv_obj_set_style_text_font(time_btn, &MyFonts16, 0);
        lv_obj_add_event_cb(time_btn, list_event_cb, LV_EVENT_CLICKED, this);
        
        /*添加按钮发送开关配置按钮*/
        lv_obj_t * button_switch_btn = lv_list_add_btn(list, LV_SYMBOL_OK, ("按钮发送: " + std::string(button_enabled ? "启用" : "禁用")).c_str());
        lv_obj_set_user_data(button_switch_btn, const_cast<char*>("button_switch_config"));
        lv_obj_set_style_text_font(button_switch_btn, &MyFonts16, 0);
        lv_obj_add_event_cb(button_switch_btn, list_event_cb, LV_EVENT_CLICKED, this);
        
        /*添加摇杆发送开关配置按钮*/
        lv_obj_t * joystick_switch_btn = lv_list_add_btn(list, LV_SYMBOL_OK, ("摇杆发送: " + std::string(joystick_enabled ? "启用" : "禁用")).c_str());
        lv_obj_set_user_data(joystick_switch_btn, const_cast<char*>("joystick_switch_config"));
        lv_obj_set_style_text_font(joystick_switch_btn, &MyFonts16, 0);
        lv_obj_add_event_cb(joystick_switch_btn, list_event_cb, LV_EVENT_CLICKED, this);
        
        for (const auto& pair : pinyin_map) {
            lv_obj_t * btn = lv_list_add_btn(list, LV_SYMBOL_EDIT, (pair.first + " -> " + pair.second).c_str());
            lv_obj_set_user_data(btn, const_cast<char*>(pair.first.c_str()));
            lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, this);
        }
        
        /*添加添加按钮*/
        lv_obj_t * add_btn = lv_list_add_btn(list, LV_SYMBOL_PLUS, "添加新映射");
        lv_obj_set_style_text_font(add_btn, &MyFonts16, 0);
        lv_obj_add_event_cb(add_btn, add_btn_event_cb, LV_EVENT_CLICKED, this);
    }

    void GamePad::save_records()
    {
        records.clear();
        for (const auto& pair : pinyin_map) {
            records += pair.first + "-" + pair.second + ";";
        }
        fml::HdlManager::getInstance().set_flash_string("gamepad", true, "records", records);

        /*移除所有已注册的命令词*/
        if (!registered_cmds.empty()) {
            std::vector<char*> old_cmd_ptrs;
            for (auto& cmd : registered_cmds) {
                old_cmd_ptrs.push_back(const_cast<char*>(cmd.c_str()));
            }
            /*停止语音检测任务*/
            fml::SpeechRecongnition::getInstance().sr_multinet_clean(NULL,NULL);
            fml::SpeechRecongnition::getInstance().sr_remove_command_list(old_cmd_ptrs.data(), old_cmd_ptrs.size());
        }

        /*重新注册新的命令词列表*/
        registered_cmds.clear(); /*清空已注册列表*/
        std::vector<char*> new_cmd_ptrs;
        for (const auto& pair : pinyin_map) {
            registered_cmds.push_back(pair.first); /*更新已注册列表*/
            new_cmd_ptrs.push_back(const_cast<char*>(pair.first.c_str()));
        }

        if (!new_cmd_ptrs.empty()) {
            /*停止语音检测任务*/
            fml::SpeechRecongnition::getInstance().sr_multinet_clean(NULL,NULL);
            fml::SpeechRecongnition::getInstance().sr_add_command_list(new_cmd_ptrs.data(), new_cmd_ptrs.size());
            //ESP_LOGI(TAG, "重新注册了 %d 个语音命令词", new_cmd_ptrs.size());
        }
    }

    void GamePad::show_edit_dialog(const std::string& key, const std::string& value)
    {
        is_editing = !key.empty();
        is_adding = key.empty();
        current_edit_key = key;
        
        /*创建编辑容器*/
        edit_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(edit_cont, lv_pct(100), lv_pct(100));
        lv_obj_align(edit_cont, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(edit_cont, LV_OPA_90, 0);
        lv_obj_set_flex_flow(edit_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(edit_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(edit_cont, 0, 0);
        lv_obj_set_style_bg_image_src(edit_cont, &_GamePad_BG_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(edit_cont, LV_OPA_COVER, 0);              /*设置不透明度*/
        lv_obj_set_style_bg_image_tiled(edit_cont, false, 0);                   /*禁止平铺*/
        lv_obj_set_style_bg_image_recolor_opa(edit_cont, LV_OPA_TRANSP, 0);     /*禁用颜色覆盖*/
        lv_obj_set_scrollbar_mode(edit_cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_width(edit_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        
        /*创建标题*/
        lv_obj_t * title = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(title, &MyFonts16, 0);
        lv_label_set_text(title, is_adding ? "添加映射" : "编辑映射");
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        
        /*创建拼音输入框*/
        lv_obj_t * pinyin_label = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(pinyin_label, &MyFonts16, 0);
        lv_label_set_text(pinyin_label, "拼音:");
        lv_obj_set_style_text_color(pinyin_label, lv_color_white(), 0);
        lv_obj_align(pinyin_label, LV_ALIGN_TOP_LEFT, 10, 50);
        
        pinyin_ta = lv_textarea_create(edit_cont);
        lv_textarea_set_one_line(pinyin_ta, true);
        lv_textarea_set_max_length(pinyin_ta, 20);
        lv_textarea_set_text(pinyin_ta, key.c_str());
        lv_obj_set_size(pinyin_ta, GAMEPAD_LV_CONTAINER_W - 80, 40);
        lv_obj_align(pinyin_ta, LV_ALIGN_TOP_LEFT, 60, 40);
        
        /*创建数字输入框*/
        lv_obj_t * number_label = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(number_label, &MyFonts16, 0);
        lv_label_set_text(number_label, "数字:");
        lv_obj_set_style_text_color(number_label, lv_color_white(), 0);
        lv_obj_align(number_label, LV_ALIGN_TOP_LEFT, 10, 100);
        
        number_ta = lv_textarea_create(edit_cont);
        lv_textarea_set_one_line(number_ta, true);
        lv_textarea_set_max_length(number_ta, 10);
        lv_textarea_set_text(number_ta, value.c_str());
        lv_obj_set_size(number_ta, GAMEPAD_LV_CONTAINER_W - 80, 40);
        lv_obj_align(number_ta, LV_ALIGN_TOP_LEFT, 60, 90);
        
        /*创建键盘（初始隐藏）*/
        kb = lv_keyboard_create(edit_cont);
        lv_obj_set_size(kb, GAMEPAD_LV_CONTAINER_W - 60, 100);
        lv_obj_align_to(kb, pinyin_ta, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);     /*初始放在拼音框下方*/

        /*为拼音文本框添加点击事件*/
        lv_obj_add_event_cb(pinyin_ta, [](lv_event_t * e) {
            lv_obj_t * ta = (lv_obj_t *)lv_event_get_target(e);
            lv_event_code_t code = lv_event_get_code(e);
            if(code == LV_EVENT_CLICKED) {
                GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
                lv_keyboard_set_textarea(app->kb, ta);
            }
        }, LV_EVENT_CLICKED, this);

        /*为数字文本框添加点击事件*/
        lv_obj_add_event_cb(number_ta, [](lv_event_t * e) {
            lv_obj_t * ta = (lv_obj_t *)lv_event_get_target(e);
            lv_event_code_t code = lv_event_get_code(e);
            if(code == LV_EVENT_CLICKED) {
                GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
                lv_keyboard_set_textarea(app->kb, ta);
            }
        }, LV_EVENT_CLICKED, this);
        
        /*创建按钮*/
        lv_obj_t * save_btn = lv_btn_create(edit_cont);
        lv_obj_t * save_label = lv_label_create(save_btn);
        lv_obj_set_style_text_font(save_label, &MyFonts16, 0);
        lv_label_set_text(save_label, "保存");
        lv_obj_set_size(save_btn, 80, 40);
        lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
        lv_obj_add_event_cb(save_btn, save_btn_event_cb, LV_EVENT_CLICKED, this);
        
        lv_obj_t * cancel_btn = lv_btn_create(edit_cont);
        lv_obj_t * cancel_label = lv_label_create(cancel_btn);
        lv_obj_set_style_text_font(cancel_label, &MyFonts16, 0);
        lv_label_set_text(cancel_label, "取消");
        lv_obj_set_size(cancel_btn, 80, 40);
        lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
        lv_obj_add_event_cb(cancel_btn, cancel_btn_event_cb, LV_EVENT_CLICKED, this);
        
        if (!is_adding) {
            lv_obj_t * delete_btn = lv_btn_create(edit_cont);
            lv_obj_t * delete_label = lv_label_create(delete_btn);
            lv_obj_set_style_text_font(delete_label, &MyFonts16, 0);
            lv_label_set_text(delete_label, "删除");
            lv_obj_set_size(delete_btn, 80, 40);
            lv_obj_align(delete_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
            lv_obj_add_event_cb(delete_btn, delete_btn_event_cb, LV_EVENT_CLICKED, this);
        }
    }

    void GamePad::hide_edit_dialog()
    {
        if (edit_cont) {
            lv_obj_del(edit_cont);
            edit_cont = nullptr;
            kb = nullptr;
            pinyin_ta = nullptr;
            number_ta = nullptr;
        }

        /*更新列表显示*/
        update_config_list();
    }

    void GamePad::show_time_config_dialog()
    {
        /*创建编辑容器*/
        edit_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(edit_cont, lv_pct(100), lv_pct(100));
        lv_obj_align(edit_cont, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(edit_cont, LV_OPA_90, 0);
        lv_obj_set_style_bg_image_src(edit_cont, &_GamePad_BG_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(edit_cont, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_image_tiled(edit_cont, false, 0);
        lv_obj_set_style_bg_image_recolor_opa(edit_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_scrollbar_mode(edit_cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_width(edit_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        
        /*创建标题*/
        lv_obj_t * title = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(title, &MyFonts16, 0);
        lv_label_set_text(title, "按钮时间配置");
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        
        /*创建时间标签*/
        lv_obj_t * time_label = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(time_label, &MyFonts16, 0);
        std::string text = "按钮保持时间: " + std::to_string(button_hold_time_ms) + "ms";
        lv_label_set_text(time_label, text.c_str());
        lv_obj_set_style_text_color(time_label, lv_color_white(), 0);
        lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 50);
        
        /*创建滑块*/
        lv_obj_t * slider = lv_slider_create(edit_cont);
        lv_slider_set_range(slider, 10, 1000);  /*设置范围10ms到1000ms*/
        lv_slider_set_value(slider, button_hold_time_ms, LV_ANIM_OFF);
        lv_obj_set_size(slider, GAMEPAD_LV_CONTAINER_W - 60, 20);
        lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 90);
        lv_obj_set_user_data(slider, time_label);  /*将标签保存为用户数据*/
        lv_obj_add_event_cb(slider, time_slider_event_cb, LV_EVENT_ALL, this);
        
        /*创建按钮*/
        lv_obj_t * ok_btn = lv_btn_create(edit_cont);
        lv_obj_t * ok_label = lv_label_create(ok_btn);
        lv_obj_set_style_text_font(ok_label, &MyFonts16, 0);
        lv_label_set_text(ok_label, "确定");
        lv_obj_set_size(ok_btn, 80, 40);
        lv_obj_align(ok_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
        lv_obj_add_event_cb(ok_btn, [](lv_event_t * e) {
            GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
            app->hide_edit_dialog();
        }, LV_EVENT_CLICKED, this);
    }

    void GamePad::show_button_switch_dialog()
    {
        /*创建编辑容器*/
        edit_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(edit_cont, lv_pct(100), lv_pct(100));
        lv_obj_align(edit_cont, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(edit_cont, LV_OPA_90, 0);
        lv_obj_set_style_bg_image_src(edit_cont, &_GamePad_BG_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(edit_cont, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_image_tiled(edit_cont, false, 0);
        lv_obj_set_style_bg_image_recolor_opa(edit_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_scrollbar_mode(edit_cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_width(edit_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        
        /*创建标题*/
        lv_obj_t * title = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(title, &MyFonts16, 0);
        lv_label_set_text(title, "按钮发送配置");
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        
        /*创建开关标签*/
        lv_obj_t * switch_label = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(switch_label, &MyFonts16, 0);
        lv_label_set_text(switch_label, "按钮发送:");
        lv_obj_set_style_text_color(switch_label, lv_color_white(), 0);
        lv_obj_align(switch_label, LV_ALIGN_TOP_MID, 0, 50);
        
        /*创建开关*/
        lv_obj_t * switch_obj = lv_switch_create(edit_cont);
        lv_obj_set_size(switch_obj, 60, 30);
        lv_obj_align(switch_obj, LV_ALIGN_TOP_MID, 0, 90);
        if (button_enabled) {
            lv_obj_add_state(switch_obj, LV_STATE_CHECKED);
        }
        lv_obj_add_event_cb(switch_obj, button_switch_event_cb, LV_EVENT_VALUE_CHANGED, this);
        
        /*创建状态标签*/
        lv_obj_t * state_label = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(state_label, &MyFonts16, 0);
        lv_label_set_text(state_label, button_enabled ? "启用" : "禁用");
        lv_obj_set_style_text_color(state_label, lv_color_white(), 0);
        lv_obj_align(state_label, LV_ALIGN_TOP_MID, 0, 130);
        
        /*为开关添加事件回调，更新状态标签*/
        lv_obj_add_event_cb(switch_obj, [](lv_event_t * e) {
            lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
            lv_obj_t * label = static_cast<lv_obj_t*>(lv_obj_get_user_data(obj));
            if (label) {
                if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
                    lv_label_set_text(label, "启用");
                } else {
                    lv_label_set_text(label, "禁用");
                }
            }
        }, LV_EVENT_VALUE_CHANGED, state_label);
        lv_obj_set_user_data(switch_obj, state_label);
    }

    void GamePad::show_joystick_switch_dialog()
    {
        /*创建编辑容器*/
        edit_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(edit_cont, lv_pct(100), lv_pct(100));
        lv_obj_align(edit_cont, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(edit_cont, LV_OPA_90, 0);
        lv_obj_set_style_bg_image_src(edit_cont, &_GamePad_BG_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(edit_cont, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_image_tiled(edit_cont, false, 0);
        lv_obj_set_style_bg_image_recolor_opa(edit_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_scrollbar_mode(edit_cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_width(edit_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        
        /*创建标题*/
        lv_obj_t * title = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(title, &MyFonts16, 0);
        lv_label_set_text(title, "摇杆发送配置");
        lv_obj_set_style_text_color(title, lv_color_white(), 0);
        lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
        
        /*创建开关标签*/
        lv_obj_t * switch_label = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(switch_label, &MyFonts16, 0);
        lv_label_set_text(switch_label, "摇杆发送:");
        lv_obj_set_style_text_color(switch_label, lv_color_white(), 0);
        lv_obj_align(switch_label, LV_ALIGN_TOP_MID, 0, 50);
        
        /*创建开关*/
        lv_obj_t * switch_obj = lv_switch_create(edit_cont);
        lv_obj_set_size(switch_obj, 60, 30);
        lv_obj_align(switch_obj, LV_ALIGN_TOP_MID, 0, 90);
        if (joystick_enabled) {
            lv_obj_add_state(switch_obj, LV_STATE_CHECKED);
        }
        lv_obj_add_event_cb(switch_obj, joystick_switch_event_cb, LV_EVENT_VALUE_CHANGED, this);
        
        /*创建状态标签*/
        lv_obj_t * state_label = lv_label_create(edit_cont);
        lv_obj_set_style_text_font(state_label, &MyFonts16, 0);
        lv_label_set_text(state_label, joystick_enabled ? "启用" : "禁用");
        lv_obj_set_style_text_color(state_label, lv_color_white(), 0);
        lv_obj_align(state_label, LV_ALIGN_TOP_MID, 0, 130);
        
        /*为开关添加事件回调，更新状态标签*/
        lv_obj_add_event_cb(switch_obj, [](lv_event_t * e) {
            lv_obj_t * obj = (lv_obj_t *)lv_event_get_target(e);
            lv_obj_t * label = static_cast<lv_obj_t*>(lv_obj_get_user_data(obj));
            if (label) {
                if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
                    lv_label_set_text(label, "启用");
                } else {
                    lv_label_set_text(label, "禁用");
                }
            }
        }, LV_EVENT_VALUE_CHANGED, state_label);
        lv_obj_set_user_data(switch_obj, state_label);
    }

    void GamePad::hide_time_config_dialog()
    {
        hide_edit_dialog();  /*重用相同的隐藏逻辑*/
    }

    void GamePad::create_joystick()
    {
        /*创建摇杆容器*/
        lv_obj_t* joystick_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(joystick_cont, JOYSTICK_BG_RADIUS * 2, JOYSTICK_BG_RADIUS * 2);
        lv_obj_align(joystick_cont, LV_ALIGN_BOTTOM_MID, 0, -20);  /*底部居中*/
        lv_obj_set_style_bg_opa(joystick_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_opa(joystick_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_pad_all(joystick_cont, 0, 0);
        lv_obj_clear_flag(joystick_cont, LV_OBJ_FLAG_SCROLLABLE);
        
        /*设置摇杆中心点*/
        joystick.center.x = JOYSTICK_CENTER_X;
        joystick.center.y = JOYSTICK_CENTER_Y;
        
        /*摇杆背景 - 透明，只保留边框*/
        joystick.bg = lv_obj_create(joystick_cont);
        lv_obj_set_size(joystick.bg, JOYSTICK_BG_RADIUS * 2, JOYSTICK_BG_RADIUS * 2);
        lv_obj_align(joystick.bg, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_radius(joystick.bg, JOYSTICK_BG_RADIUS, 0);
        lv_obj_set_style_bg_opa(joystick.bg, LV_OPA_TRANSP, 0);                         /*背景透明*/
        lv_obj_set_style_border_color(joystick.bg, lv_color_white(), 0);                /*白色边框*/
        lv_obj_set_style_border_width(joystick.bg, 1, 0);                               /*边框宽度1像素*/
        lv_obj_set_style_border_opa(joystick.bg, LV_OPA_100, 0);                        /*边框不透明度100%*/
        lv_obj_clear_flag(joystick.bg, LV_OBJ_FLAG_SCROLLABLE);
        
        /*摇杆 - 透明，只保留边框*/
        joystick.stick = lv_obj_create(joystick_cont);
        lv_obj_set_size(joystick.stick, JOYSTICK_RADIUS * 2, JOYSTICK_RADIUS * 2);
        lv_obj_align_to(joystick.stick, joystick.bg, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_radius(joystick.stick, JOYSTICK_RADIUS, 0);
        lv_obj_set_style_bg_opa(joystick.stick, LV_OPA_TRANSP, 0);                      /*背景透明*/
        lv_obj_set_style_border_color(joystick.stick, lv_color_white(), 0);             /*白色边框*/
        lv_obj_set_style_border_width(joystick.stick, 1, 0);                            /*边框宽度1像素*/
        lv_obj_set_style_border_opa(joystick.stick, LV_OPA_100, 0);                     /*边框不透明度100%*/
        lv_obj_clear_flag(joystick.stick, LV_OBJ_FLAG_SCROLLABLE);
        
        /*添加事件处理*/
        lv_obj_add_event_cb(joystick.bg, joystick_event_cb, LV_EVENT_ALL, this);
        lv_obj_add_event_cb(joystick.stick, joystick_event_cb, LV_EVENT_ALL, this);
        
        /*初始化摇杆值*/
        reset_joystick(joystick);
    }

    void GamePad::update_joystick_position(Joystick& joystick, lv_point_t point)
    {
        /*计算相对于中心点的偏移*/
        int16_t dx = point.x - joystick.center.x;
        int16_t dy = point.y - joystick.center.y;
        
        /*计算距离*/
        float distance = sqrt(dx * dx + dy * dy);
        
        /*限制在摇杆背景范围内*/
        if (distance > JOYSTICK_BG_RADIUS - JOYSTICK_RADIUS) {
            dx = dx * (JOYSTICK_BG_RADIUS - JOYSTICK_RADIUS) / distance;
            dy = dy * (JOYSTICK_BG_RADIUS - JOYSTICK_RADIUS) / distance;
        }
        
        /*更新摇杆位置*/
        lv_obj_align(joystick.stick, LV_ALIGN_CENTER, dx, dy);
        
        /*计算摇杆值（0-65535）*/
        joystick.lx = 32768 + (dx * 32768 / (JOYSTICK_BG_RADIUS - JOYSTICK_RADIUS));
        joystick.ly = 32768 + (dy * 32768 / (JOYSTICK_BG_RADIUS - JOYSTICK_RADIUS));
        
        /*发送摇杆数据*/
        send_joystick_data();
    }

    void GamePad::reset_joystick(Joystick& joystick)
    {
        /*重置摇杆位置*/
        lv_obj_align(joystick.stick, LV_ALIGN_CENTER, 0, 0);
        
        /*重置摇杆值到中心*/
        joystick.lx = 32768;
        joystick.ly = 32768;
        
        /*发送摇杆数据*/
        send_joystick_data();
    }

    void GamePad::send_joystick_data()
    {
        if (joystick_enabled) {
            fml::HdlManager::getInstance().bt_set_joystick(
                joystick.lx, 
                joystick.ly, 
                32768,  /*右摇杆X固定为中间值*/
                32768   /*右摇杆Y固定为中间值*/
            );
            fml::HdlManager::getInstance().bt_send_report();
        }
    }

    void GamePad::save_switch_states()
    {
        fml::HdlManager::getInstance().set_flash_int("gamepad", true, "btn_enabled", button_enabled ? 1 : 0);
        fml::HdlManager::getInstance().set_flash_int("gamepad", true, "joy_enabled", joystick_enabled ? 1 : 0);
    }

    GamePad::GamePad(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* container)
    {
        memset(&lv_screen, 0, sizeof(lv_screen));
        lv_screen.screen = screen;
        lv_screen.tileview = tileview;
        lv_screen.container = container;

        main_cont = NULL;
        title_bar = NULL;
        title_label = NULL;
        config_cont = NULL;
        list = NULL;
        kb = NULL;
        pinyin_ta = NULL;
        number_ta = NULL;
        edit_cont = NULL;

        wifi_state = GAMEPAD_WIFI_STATE_NONE;
        lastButtonTime = 0;
        button_hold_time_ms = GAMEPAD_BUTTON_HOLD_TIME_MS;  /*默认GAMEPAD_BUTTON_HOLD_TIME_MS*/
        is_editing = false;
        is_adding = false;

        /*初始化无限重复相关变量*/
        is_infinite_repeat = false;
        repeat_sequence = "";
        lastRepeatTime = 0;
        repeat_interval_ms = 100;  /*重复间隔100ms*/

        /*初始化摇杆*/
        memset(&joystick, 0, sizeof(joystick));

        /*初始化开关控制变量*/
        button_switch = NULL;
        joystick_switch = NULL;
        button_enabled = true;  /*默认启用*/
        joystick_enabled = true; /*默认启用*/
        
        ESP_LOGI(TAG, "GamePad on construct");
    }

    GamePad::~GamePad()
    {
        if(main_cont != NULL)lv_obj_del(main_cont);
        if(config_cont != NULL) lv_obj_del(config_cont);
        if(edit_cont != NULL) lv_obj_del(edit_cont);
        ESP_LOGI(TAG, "GamePad on deconstruct");
    }

    /*创建游戏手柄界面*/
    void GamePad::create_ui()
    {
        /*创建主容器*/
        main_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
        lv_obj_set_flex_flow(main_cont, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(main_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(main_cont, 0, 0);
        lv_obj_set_style_pad_row(main_cont, GAMEPAD_LV_ITEM_SPACING, 0);
        lv_obj_set_style_bg_image_src(main_cont, &_GamePad_BG_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(main_cont, LV_OPA_COVER, 0);              /*设置不透明度*/
        lv_obj_set_style_bg_image_tiled(main_cont, false, 0);                   /*禁止平铺*/
        lv_obj_set_style_bg_image_recolor_opa(main_cont, LV_OPA_TRANSP, 0);     /*禁用颜色覆盖*/
        lv_obj_set_scrollbar_mode(main_cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_width(main_cont, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        /*顶部标题栏*/
        title_bar = lv_obj_create(main_cont);
        lv_obj_set_size(title_bar, lv_pct(100), GAMEPAD_LV_ITEM_HEIGHT);
        lv_obj_set_style_bg_opa(title_bar, LV_OPA_TRANSP, 0); 
        lv_obj_set_style_border_width(title_bar, 0, 0);
        lv_obj_set_style_pad_all(title_bar, 0, 0);
        /*标题*/
        title_label = lv_label_create(title_bar);
        lv_label_set_text(title_label, "游戏手柄");
        lv_obj_set_style_text_font(title_label, &MyFonts16, 0);
        lv_obj_set_style_text_color(title_label, lv_color_white(), 0);
        lv_obj_align(title_label, LV_ALIGN_CENTER, 0, 10);
        /*创建配置按钮*/
        lv_obj_t * config_btn = lv_btn_create(main_cont);
        lv_obj_set_size(config_btn, GAMEPAD_LV_ITEM_WIDTH, GAMEPAD_LV_ITEM_HEIGHT);
        lv_obj_set_style_bg_opa(config_btn, LV_OPA_TRANSP, 0);                  /*背景透明*/
        lv_obj_set_style_border_color(config_btn, lv_color_white(), 0);         /*白色边框*/
        lv_obj_set_style_border_width(config_btn, 1, 0);                        /*边框宽度1像素*/
        lv_obj_set_style_border_opa(config_btn, LV_OPA_100, 0);                 /*边框不透明度100%*/
        lv_obj_add_event_cb(config_btn, [](lv_event_t * e) {
            GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
            lv_obj_clear_flag(app->config_cont, LV_OBJ_FLAG_HIDDEN);
        }, LV_EVENT_CLICKED, this);
        /*创建摇杆*/
        create_joystick();
        /*创建配置界面容器*/
        config_cont = lv_obj_create(lv_screen.container);
        lv_obj_set_size(config_cont, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_opa(config_cont, LV_OPA_COVER, 0);
        lv_obj_add_flag(config_cont, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_bg_image_src(config_cont, &_GamePad_BG_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(config_cont, LV_OPA_COVER, 0);              /*设置不透明度*/
        lv_obj_set_style_bg_image_tiled(config_cont, false, 0);                   /*禁止平铺*/
        lv_obj_set_style_bg_image_recolor_opa(config_cont, LV_OPA_TRANSP, 0);     /*禁用颜色覆盖*/
        /*配置界面标题*/
        lv_obj_t * config_title = lv_label_create(config_cont);
        lv_label_set_text(config_title, "拼音映射配置");
        lv_obj_set_style_text_color(config_title, lv_color_white(), 0);
        lv_obj_set_style_text_font(config_title, &MyFonts16, 0);
        lv_obj_align(config_title, LV_ALIGN_TOP_MID, 0, 10);
        /*创建返回按钮*/
        lv_obj_t * back_btn = lv_btn_create(config_cont);
        lv_obj_t * back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, LV_SYMBOL_LEFT);
        lv_obj_set_size(back_btn, 40, 40);
        lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
        lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
            GamePad * app = static_cast<GamePad*>(lv_event_get_user_data(e));
            lv_obj_add_flag(app->config_cont, LV_OBJ_FLAG_HIDDEN);
        }, LV_EVENT_CLICKED, this);
        /*创建列表*/
        list = lv_list_create(config_cont);
        lv_obj_set_size(list, GAMEPAD_LV_ITEM_WIDTH, GAMEPAD_LV_CONTAINER_H - 80);
        lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 50);
    }

    void GamePad::onCreate()
    {
        /*创建界面*/
        create_ui();
        /*获取flash里的记录*/
        records = fml::HdlManager::getInstance().get_flash_string("gamepad",true,"records");
        if(records == ""){
            /*内置记录*/
            records.append("ling-0;");
            records.append("yi-1;");
            records.append("er-2;");
            records.append("san-3;");
            records.append("si-4;");
            records.append("wu-5;");
            records.append("liu-6;");
            records.append("qi-7;");
            records.append("ba-8;");
            records.append("jiu-9;");
            records.append("chong fu yi-#1;");
            /*保存到flash*/
            fml::HdlManager::getInstance().set_flash_string("gamepad",true,"records",records);
        }

        /*获取按钮时间配置*/
        button_hold_time_ms = fml::HdlManager::getInstance().get_flash_int("gamepad", true, "button_time", GAMEPAD_BUTTON_HOLD_TIME_MS);

        /*获取开关状态配置*/
        button_enabled = fml::HdlManager::getInstance().get_flash_int("gamepad", true, "btn_enabled", 1) != 0;
        joystick_enabled = fml::HdlManager::getInstance().get_flash_int("gamepad", true, "joy_enabled", 1) != 0;

        /*解析records字符串*/
        parse_records();

        /*更新列表显示*/
        update_config_list();
        
        ESP_LOGI(TAG, "GamePad on create");
    }

    void GamePad::onOpen()
    {
        /*IMU开启并进入校准状态*/
        fml::HdlManager::getInstance().imu_start();
        /*禁止进入睡眠模式*/
        fml::HdlManager::getInstance().set_no_sleep_for_lvgl();
        /*注册语音命令词唤醒*/
        if (!pinyin_map.empty()) {
            std::vector<char*> cmd_ptrs;
            for (const auto& pair : pinyin_map) {
                cmd_ptrs.push_back(const_cast<char*>(pair.first.c_str()));
            }
            /*停止语音检测任务*/
            fml::SpeechRecongnition::getInstance().sr_multinet_clean(NULL,NULL);
            fml::SpeechRecongnition::getInstance().sr_add_command_list(cmd_ptrs.data(), cmd_ptrs.size());
        }
        /*开启回调响应*/
        fml::SpeechRecongnition::getInstance().sr_multinet_result(get_sr_pinyin, this);
        /*保存当前wifi状态*/
        if(fml::HdlManager::getInstance().get_wifi_station()){          
            /*获取wifi开关*/            
            wifi_state = GAMEPAD_WIFI_STAET_STA;
            /*关闭wifi*/
            fml::HdlManager::getInstance().stop_wifi();
        }else if(fml::HdlManager::getInstance().get_wifi_configureation_ap()){      
            /*获取wifi配置模式*/
            wifi_state = GAMEPAD_WIFI_STATE_AP;
            /*关闭wifi*/
            fml::HdlManager::getInstance().stop_wifi();
        }else{
            wifi_state = GAMEPAD_WIFI_STATE_NONE;
        }

        /*清空按钮队列*/
        while (!buttonQueue.empty()) {
            buttonQueue.pop();
        }
        lastButtonTime = 0;

        /*清空编辑窗口*/
        hide_edit_dialog();

        /*开启bt*/
        fml::HdlManager::getInstance().bt_init();
        /*复位手柄*/
        fml::HdlManager::getInstance().bt_set_joystick(32768,32768,32768,32768);
        fml::HdlManager::getInstance().bt_set_z_axis(32768);
        fml::HdlManager::getInstance().bt_set_hat_switch(0);
        for (int i = 0; i < 10; i++) fml::HdlManager::getInstance().bt_set_button(i, false);

        /*重置无限重复状态*/
        is_infinite_repeat = false;
        repeat_sequence = "";
        lastRepeatTime = 0;

        ESP_LOGI(TAG, "GamePad on Open");
    }

    void GamePad::onRunning()
    {
        uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        /*处理按钮队列（仅在按钮发送启用时）*/
        if (button_enabled && !buttonQueue.empty()) {
            auto& operation = buttonQueue.front();
            uint8_t button_index = operation.first;
            uint8_t state = operation.second;
            
            if (state == 1) {   /*按下按钮*/
                fml::HdlManager::getInstance().bt_set_button(button_index, true);
                fml::HdlManager::getInstance().bt_send_report();
                lastButtonTime = currentTime;
                /*移出按下操作，保留释放操作在队列中*/
                buttonQueue.pop();
            } else {            /*释放按钮*/
                if (currentTime - lastButtonTime >= button_hold_time_ms) {
                    fml::HdlManager::getInstance().bt_set_button(button_index, false);
                    fml::HdlManager::getInstance().bt_send_report();
                    buttonQueue.pop();
                }
            }
        }

        /*处理无限重复（仅在按钮发送启用时）*/
        if (button_enabled && is_infinite_repeat && !repeat_sequence.empty()) {
            if (buttonQueue.empty() && (currentTime - lastRepeatTime >= repeat_interval_ms)) {
                /*将重复序列加入队列*/
                for (char c : repeat_sequence) {
                    uint8_t button_index = c - '0';
                    buttonQueue.push(std::make_pair(button_index, 1)); /*按下*/
                    buttonQueue.push(std::make_pair(button_index, 0)); /*释放*/
                }
                lastRepeatTime = currentTime;
            }
        }

        /*处理摇杆数据（仅在摇杆发送启用时）*/
        if (joystick_enabled) {
            static uint16_t old_lx = 0;
            static uint16_t old_ly = 0;
            uint16_t cur_lx = fml::HdlManager::getInstance().imu_get_joystick_lx();
            uint16_t cur_ly = fml::HdlManager::getInstance().imu_get_joystick_ly();
            if((old_lx != cur_lx) || (old_ly != cur_ly))
            {
                old_lx = cur_lx;
                old_ly = cur_ly;
                fml::HdlManager::getInstance().bt_set_joystick(cur_lx, cur_ly,32768,32768);
                fml::HdlManager::getInstance().bt_send_report();
            }
        }

        //ESP_LOGI(TAG, "GamePad on Running");
    }

    void GamePad::onSleeping()
    {
        //ESP_LOGI(TAG, "GamePad on Sleeping");
    }

    void GamePad::onClose()
    {
        /*IMU退出运行*/
        fml::HdlManager::getInstance().imu_stop();
        /*清空编辑窗口*/
        hide_edit_dialog();
        /*允许进入睡眠模式*/
        fml::HdlManager::getInstance().clear_no_sleep_for_lvgl();
        /*移除所有命令词并关闭回调响应*/
        if (!pinyin_map.empty()) {
            std::vector<char*> cmd_ptrs;
            for (const auto& pair : pinyin_map) {
                cmd_ptrs.push_back(const_cast<char*>(pair.first.c_str()));
            }
            fml::SpeechRecongnition::getInstance().sr_multinet_clean(NULL,NULL);
            fml::SpeechRecongnition::getInstance().sr_remove_command_list(cmd_ptrs.data(), cmd_ptrs.size());
        }
        /*关闭回调响应*/
        fml::SpeechRecongnition::getInstance().sr_multinet_result(NULL, NULL);
        /*关闭bt*/
        fml::HdlManager::getInstance().bt_deinit();

        /*停止无限重复*/
        is_infinite_repeat = false;
        repeat_sequence = "";

        /*恢复原先wifi状态*/
        switch(wifi_state){
            case GAMEPAD_WIFI_STAET_STA:
                fml::HdlManager::getInstance().start_wifi_station();
            break;

            case GAMEPAD_WIFI_STATE_AP:
                fml::HdlManager::getInstance().start_wifi_configuration_ap();
            break;
        }

        ESP_LOGI(TAG, "GamePad on Close");
    }

    void GamePad::onDestroy()
    {
        ESP_LOGI(TAG, "GamePad on destroy");
    }

}
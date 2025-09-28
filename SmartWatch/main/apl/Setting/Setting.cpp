/**
 * @file Setting.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "Setting.hpp"

namespace apl{

    void Setting::lv_prompt_event_cb(lv_event_t * e)
    {
        lv_event_code_t code = lv_event_get_code(e);

        if(code == LV_EVENT_DELETE){
            fml::HdlManager::getInstance().clear_no_sleep_for_lvgl();
            /*用户点击关闭会自动释放控件，所以这里要赋值NULL*/
            ((struct setting_lv_prompt_t*)lv_event_get_user_data(e))->prompt = NULL;
        }
    }

    void Setting::get_lv_prompt(struct setting_lv_prompt_t* lv_prompt, struct setting_lv_screen_t* lv_screen)
    {
        if(lv_prompt != NULL && lv_screen != NULL){
            lv_prompt->prompt = lv_msgbox_create(lv_screen->tile);
            /*居中显示消息框*/
            lv_obj_center(lv_prompt->prompt);
            /*设置消息框的宽度*/
            lv_obj_set_width(lv_prompt->prompt, SETTING_LV_PROMPT_W);
            /*添加关闭按钮*/
            lv_msgbox_add_close_button(lv_prompt->prompt);
            /*设置事件回调*/
            lv_obj_add_event_cb(lv_prompt->prompt, lv_prompt_event_cb, LV_EVENT_ALL, lv_prompt);
            /*设置字体*/
            lv_obj_set_style_text_font(lv_prompt->prompt, &MyFonts16, 0);
            /*提示框存在时，不允许睡眠*/
            fml::HdlManager::getInstance().set_no_sleep_for_lvgl();
        }
    }

    void Setting::set_lv_prompt(struct setting_lv_prompt_t* lv_prompt, struct setting_lv_screen_t* lv_screen, const char* text)
    {
        if(lv_prompt != NULL && lv_screen != NULL){
            get_lv_prompt(lv_prompt, lv_screen);
            lv_msgbox_add_text(lv_prompt->prompt, text);
        }
    }

    void Setting::slider_volume_event_cb(lv_event_t * e) {
        Setting* app = (Setting*)lv_event_get_user_data(e);
        int32_t value = lv_slider_get_value(app->slider_volume);
        /*设置音量*/
        fml::HdlManager::getInstance().set_audio_volume(value);
        ESP_LOGI(app->TAG, "Volume set to: %d", value);
    }

    void Setting::slider_brightness_event_cb(lv_event_t * e) {
        Setting* app = (Setting*)lv_event_get_user_data(e);
        int32_t value = lv_slider_get_value(app->slider_brightness);
        /*设置亮度*/
        fml::HdlManager::getInstance().set_brightness(value);
        ESP_LOGI(app->TAG, "Brightness set to: %d", value);
    }

    void Setting::switch_wifi_config_event_cb(lv_event_t * e) {
        Setting* app = (Setting*)lv_event_get_user_data(e);
        bool state = lv_obj_has_state(app->switch_wifi_config, LV_STATE_CHECKED);
        /*设置WiFi配置模式*/
        if(state){
            set_lv_prompt(&app->lv_prompt, &app->lv_screen,"不要关闭弹窗, 请连接wifi:SmartWatch-XXXX, 然后连接IP地址:192.168.4.1");
            fml::HdlManager::getInstance().stop_wifi();
            fml::HdlManager::getInstance().start_wifi_configuration_ap();
        }else{
            fml::HdlManager::getInstance().stop_wifi();
        }
        ESP_LOGI(app->TAG, "WiFi config mode: %s", state ? "ON" : "OFF");
    }

    void Setting::switch_wifi_onoff_event_cb(lv_event_t * e) {
        Setting* app = (Setting*)lv_event_get_user_data(e);
        bool state = lv_obj_has_state(app->switch_wifi_onoff, LV_STATE_CHECKED);
        /*设置WiFi开关*/
        if(state){
            fml::HdlManager::getInstance().stop_wifi();
            fml::HdlManager::getInstance().start_wifi_station();
        }else{
            fml::HdlManager::getInstance().stop_wifi();
        }
        ESP_LOGI(app->TAG, "WiFi: %s", state ? "ON" : "OFF");
    }

    void Setting::btn_clear_wifi_event_cb(lv_event_t * e) {
        lv_event_code_t code = lv_event_get_code(e);
        Setting* app = (Setting*)lv_event_get_user_data(e);
        if (code == LV_EVENT_CLICKED) {
            /*清除WiFi配置*/
            fml::HdlManager::getInstance().clear_wifi_ssid_list();
            app->set_lv_prompt(&app->lv_prompt, &app->lv_screen, "WiFi配置已清除,可重启验证.");
            ESP_LOGI(app->TAG, "Cleared all WiFi configurations");
        }
    }

    /*创建UI元素*/
    void Setting::create_ui_elements() {
        /*创建容器用于布局*/
        container = lv_obj_create(lv_screen.tile);
        lv_obj_set_size(container, SETTING_LV_CONTAINER_W, SETTING_LV_CONTAINER_H);
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(container, 10, 0);
        lv_obj_set_style_pad_row(container, SETTING_LV_ITEM_SPACING, 0);
        lv_obj_set_style_bg_image_src(container, &_Setting_BG_RGB565_240x280, 0);
        lv_obj_set_style_bg_image_opa(container, LV_OPA_COVER, 0);              /*设置不透明度*/
        lv_obj_set_style_bg_image_tiled(container, false, 0);                   /*禁止平铺*/
        lv_obj_set_style_bg_image_recolor_opa(container, LV_OPA_TRANSP, 0);     /*禁用颜色覆盖*/
        lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_border_width(container, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

        /*创建音量控制滑块*/
        label_volume = lv_label_create(container);
        lv_label_set_text(label_volume, "音量控制");
        lv_obj_set_style_text_font(label_volume, &MyFonts16, 0);
        lv_obj_set_width(label_volume, SETTING_LV_ITEM_WIDTH);
        lv_obj_set_style_text_color(label_volume, lv_color_hex(0xffffff), 0);
        
        slider_volume = lv_slider_create(container);
        lv_obj_set_width(slider_volume, SETTING_LV_ITEM_WIDTH);
        lv_slider_set_range(slider_volume, 0, 100);
        lv_obj_add_event_cb(slider_volume, slider_volume_event_cb, LV_EVENT_VALUE_CHANGED, this);

        /*创建亮度控制滑块*/
        label_brightness = lv_label_create(container);
        lv_label_set_text(label_brightness, "亮度控制");
        lv_obj_set_style_text_font(label_brightness, &MyFonts16, 0);
        lv_obj_set_width(label_brightness, SETTING_LV_ITEM_WIDTH);
        lv_obj_set_style_text_color(label_brightness, lv_color_hex(0xffffff), 0);
        
        slider_brightness = lv_slider_create(container);
        lv_obj_set_width(slider_brightness, SETTING_LV_ITEM_WIDTH);
        lv_slider_set_range(slider_brightness, 0, 200);
        lv_obj_add_event_cb(slider_brightness, slider_brightness_event_cb, LV_EVENT_VALUE_CHANGED, this);

        /*创建清除WiFi配置按钮*/
        btn_clear_wifi = lv_btn_create(container);
        lv_obj_set_size(btn_clear_wifi, 150, SETTING_LV_ITEM_HEIGHT);
        lv_obj_t* btn_label = lv_label_create(btn_clear_wifi);
        lv_label_set_text(btn_label, "清除WiFi配置");
        lv_obj_set_style_text_color(btn_label, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(btn_label, &MyFonts16, 0);
        lv_obj_center(btn_label);
        lv_obj_add_event_cb(btn_clear_wifi, btn_clear_wifi_event_cb, LV_EVENT_CLICKED, this);

        /*创建WiFi配置开关*/
        label_wifi_config = lv_label_create(container);
        lv_label_set_text(label_wifi_config, "WiFi配置模式");
        lv_obj_set_style_text_color(label_wifi_config, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(label_wifi_config, &MyFonts16, 0);
        lv_obj_set_width(label_wifi_config, SETTING_LV_ITEM_WIDTH);
        
        switch_wifi_config = lv_switch_create(container);
        lv_obj_add_event_cb(switch_wifi_config, switch_wifi_config_event_cb, LV_EVENT_VALUE_CHANGED, this);

        /*创建WiFi开关*/
        label_wifi_onoff = lv_label_create(container);
        lv_label_set_text(label_wifi_onoff, "WiFi开关");
        lv_obj_set_style_text_color(label_wifi_onoff, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_font(label_wifi_onoff, &MyFonts16, 0);
        lv_obj_set_width(label_wifi_onoff, SETTING_LV_ITEM_WIDTH);
        
        switch_wifi_onoff = lv_switch_create(container);
        lv_obj_add_event_cb(switch_wifi_onoff, switch_wifi_onoff_event_cb, LV_EVENT_VALUE_CHANGED, this);
    }

    Setting::Setting(lv_obj_t* screen, lv_obj_t* tileview, lv_obj_t* tile)
    {
        memset(&lv_screen, 0, sizeof(lv_screen));
        memset(&lv_prompt, 0, sizeof(lv_prompt));

        lv_screen.screen = screen;
        lv_screen.tileview = tileview;
        lv_screen.tile = tile;

        slider_volume = NULL;
        slider_brightness = NULL;
        switch_wifi_config = NULL;
        switch_wifi_onoff = NULL;
        btn_clear_wifi = NULL;
        label_volume = NULL;
        label_brightness = NULL;
        label_wifi_config = NULL;
        label_wifi_onoff = NULL;
        ESP_LOGI(TAG, "Setting on construct");
    }

    Setting::~Setting()
    {
        if(lv_prompt.prompt != NULL)lv_obj_del(lv_prompt.prompt);
        if(container != NULL) lv_obj_del(container);
        ESP_LOGI(TAG, "Setting on deconstruct");
    }

    void Setting::onCreate()
    {
        /*在创建时构建UI*/
        create_ui_elements();
        ESP_LOGI(TAG, "Setting on create");
    }

    void Setting::onOpen()
    {
        /*获取音量*/
        lv_slider_set_value(slider_volume, fml::HdlManager::getInstance().get_audio_volume(), LV_ANIM_OFF);

        /*获取亮度*/
        lv_slider_set_value(slider_brightness, fml::HdlManager::getInstance().get_brightness(), LV_ANIM_OFF);

        /*获取wifi开关*/
        if(fml::HdlManager::getInstance().get_wifi_station()){
            lv_obj_add_state(switch_wifi_onoff, LV_STATE_CHECKED); 
        }else{
            lv_obj_add_state(switch_wifi_onoff, LV_STATE_DEFAULT); 
        }

        /*获取wifi配置模式*/
        if(fml::HdlManager::getInstance().get_wifi_configureation_ap()){
            lv_obj_add_state(switch_wifi_config, LV_STATE_CHECKED); 
        }else{
            lv_obj_add_state(switch_wifi_config, LV_STATE_DEFAULT); 
        }
        
        ESP_LOGI(TAG, "Setting on Open");
    }

    void Setting::onRunning()
    {
        //ESP_LOGI(TAG, "Setting on Running");
    }

    void Setting::onSleeping()
    {
        //ESP_LOGI(TAG, "Setting on Sleeping");
    }

    void Setting::onClose()
    {
        ESP_LOGI(TAG, "Setting on Close");
    }

    void Setting::onDestroy()
    {
        ESP_LOGI(TAG, "Setting on destroy");
    }

}

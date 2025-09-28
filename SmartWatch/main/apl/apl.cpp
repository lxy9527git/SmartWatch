/**
 * @file apl.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "apl.hpp"

namespace apl{

    apl::apl()
    {
        screen = NULL;
        tileview = NULL;
        WatchDial_tile = NULL;
        AppStore_tile = NULL;
        Setting_tile = NULL;
        OverLap_tile = NULL;
        OverLap_tile_container_Assistant = NULL;
        OverLap_tile_container_Painter = NULL;
        OverLap_tile_container_Gamepad = NULL;
        cur_overlap_container_disp = APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_ASSISTANT;
        
        WatchDial_id = -1;
        AppStore_id = -1;
        Assistant_id = -1;
        Painter_id = -1;
        Gamepad_id = -1;
        Setting_id = -1;

        memset(&lv_boot, 0, sizeof(lv_boot));
        memset(&sr_result, 0, sizeof(sr_result));
    }

    apl::~apl()
    {
        /*自动递归释放子控件*/
        if(screen != NULL)lv_obj_del(screen);
        /*销毁app*/
        if(WatchDial_id != -1)mc.extensionManager()->destroyAbility(WatchDial_id);
        if(AppStore_id != -1)mc.extensionManager()->destroyAbility(AppStore_id);
        if(Assistant_id != -1)mc.extensionManager()->destroyAbility(Assistant_id);
        if(Painter_id != -1)mc.extensionManager()->destroyAbility(Painter_id);
        if(Gamepad_id != -1)mc.extensionManager()->destroyAbility(Gamepad_id);
        if(Setting_id != -1)mc.extensionManager()->destroyAbility(Setting_id);
    }

    void apl::tileview_event_cb(lv_event_t * e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        apl* app = (apl*)lv_event_get_user_data(e);

        /*切换事件处理*/
        if (code == LV_EVENT_VALUE_CHANGED) {
            mooncake::AbilityManager* am = app->mc.extensionManager();
            /*获取当前激活的tile对象*/
            lv_obj_t *active_tile = lv_tileview_get_tile_act(app->tileview);
            /*如果是watchdial tile*/
            if(active_tile == app->WatchDial_tile){
                if(am->getAppAbilityCurrentState(app->Gamepad_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Gamepad_id);
                if(am->getAppAbilityCurrentState(app->Setting_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Setting_id);
                if(am->getAppAbilityCurrentState(app->AppStore_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->AppStore_id);
                if(am->getAppAbilityCurrentState(app->Assistant_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Assistant_id);
                if(am->getAppAbilityCurrentState(app->Painter_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Painter_id);
                if(am->getUIAbilityCurrentState(app->WatchDial_id) !=  mooncake::UIAbility::State_t::StateForeground)am->showUIAbility(app->WatchDial_id);
            }else if(active_tile == app->AppStore_tile){
                if(am->getAppAbilityCurrentState(app->Gamepad_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Gamepad_id);
                if(am->getAppAbilityCurrentState(app->Setting_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Setting_id);
                if(am->getUIAbilityCurrentState(app->WatchDial_id) !=  mooncake::UIAbility::State_t::StateBackground)am->hideUIAbility(app->WatchDial_id);
                if(am->getAppAbilityCurrentState(app->Assistant_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Assistant_id);
                if(am->getAppAbilityCurrentState(app->Painter_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Painter_id);
                if(am->getAppAbilityCurrentState(app->AppStore_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(app->AppStore_id);
            }else if(active_tile == app->Setting_tile){
                if(am->getAppAbilityCurrentState(app->Gamepad_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Gamepad_id);
                if(am->getUIAbilityCurrentState(app->WatchDial_id) !=  mooncake::UIAbility::State_t::StateBackground)am->hideUIAbility(app->WatchDial_id);
                if(am->getAppAbilityCurrentState(app->AppStore_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->AppStore_id);
                if(am->getAppAbilityCurrentState(app->Assistant_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Assistant_id);
                if(am->getAppAbilityCurrentState(app->Painter_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Painter_id);
                if(am->getAppAbilityCurrentState(app->Setting_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(app->Setting_id);
            }else if(active_tile == app->OverLap_tile){
                if(am->getUIAbilityCurrentState(app->WatchDial_id) !=  mooncake::UIAbility::State_t::StateBackground)am->hideUIAbility(app->WatchDial_id);
                if(am->getAppAbilityCurrentState(app->AppStore_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->AppStore_id);
                if(am->getAppAbilityCurrentState(app->Setting_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Setting_id);
                switch(app->cur_overlap_container_disp){
                    case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_ASSISTANT:
                        if(am->getAppAbilityCurrentState(app->Gamepad_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Gamepad_id);
                        if(am->getAppAbilityCurrentState(app->Painter_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Painter_id);
                        if(am->getAppAbilityCurrentState(app->Assistant_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(app->Assistant_id);
                    break;

                    case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_PAINTER:
                        if(am->getAppAbilityCurrentState(app->Gamepad_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Gamepad_id);
                        if(am->getAppAbilityCurrentState(app->Assistant_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Assistant_id);
                        if(am->getAppAbilityCurrentState(app->Painter_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(app->Painter_id);
                    break;

                    case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_GAMEPAD:
                        if(am->getAppAbilityCurrentState(app->Painter_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Painter_id);
                        if(am->getAppAbilityCurrentState(app->Assistant_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(app->Assistant_id);
                        if(am->getAppAbilityCurrentState(app->Gamepad_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(app->Gamepad_id);
                    break;
                }
            }
        }
    }

    void apl::JpegDecoderInputInfoCallBack(void* input_info_user_data, int Number, int index)
    {
        struct apl_lv_boot_t* lv_boot = (struct apl_lv_boot_t*)input_info_user_data;
    
        if((index + 1) == Number){
            if(lv_boot != NULL){
                lv_obj_move_background(lv_boot->background);
                lv_obj_move_background(lv_boot->label);
                /* Reset lvgl inactive time */
                lv_disp_trig_activity(NULL);
            }
        }else{
            if(index % 10 == 0){
                /*更新进度条（0-100范围）*/
                if(lv_boot != NULL){
                    /*计算新的宽度（基于百分比）*/
                    lv_obj_set_width(lv_boot->background, (APL_LV_BOOT_W * ((index + 1) * 100 / Number)) / 100);
                }
                lv_refr_now(lv_disp_get_default());
            }
        }
    }

    int apl::get_m_audio(std::vector<int16_t>& data)
    {
        std::vector<int32_t> m_audio_data(data.size());
        const int samples = fml::HdlManager::getInstance().get_audio_data(m_audio_data);
        for (int i = 0; i < samples; i++) {
            int32_t value = m_audio_data[i] >> 14;      /*>>8 噪声还是太大，改为>>14*/
            data[i] = (value > INT16_MAX) ? INT16_MAX : (value < -INT16_MAX) ? -INT16_MAX : (int16_t)value;
        }
        return samples;
    }

    int apl::set_m_audio(std::vector<int16_t>& data)
    {
        std::vector<int32_t> m_audio_data(data.size());
        /*单行完成转换*/
        std::transform(data.begin(), data.end(), m_audio_data.begin(),
                    [](int16_t sample) { return static_cast<int32_t>(sample); });
        
        return fml::HdlManager::getInstance().set_audio_data(m_audio_data);
    }

    void apl::lv_marquee_icon_event_cb(lv_event_t * e)
    {
        lv_event_code_t code = lv_event_get_code(e);
        apl* app = (apl*)lv_event_get_user_data(e);
        lv_obj_t* img = (lv_obj_t*)lv_event_get_current_target(e);

        if(code == LV_EVENT_CLICKED){
            const void* img_src = lv_img_get_src(img);
            if(img_src == &_assistant_icon_RGB565_40x40){
                app->tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_ASSISTANT);
            }else if(img_src == &_painter_icon_RGB565_40x40){
                app->tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_PAINTER);
            }else if(img_src == &_gamepad_icon_RGB565_40x40){
                app->tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_GAMEPAD);
            }
        }
    }

    void apl::init_lv_boot()
    {
        /*创建背景（渐变效果）*/
        lv_boot.background = lv_obj_create(screen);
        lv_obj_set_pos(lv_boot.background, APL_LV_BOOT_X, APL_LV_BOOT_Y);
        lv_obj_set_size(lv_boot.background, APL_LV_BOOT_W, APL_LV_BOOT_H);
        lv_obj_set_style_radius(lv_boot.background, 10, 0);
        lv_obj_set_style_shadow_width(lv_boot.background, 5, 0);
        lv_obj_set_style_shadow_color(lv_boot.background, lv_color_black(), 0);
        lv_obj_set_style_bg_color(lv_boot.background, lv_color_hex(0xf6d365), 0);
        lv_obj_set_style_bg_grad_color(lv_boot.background, lv_color_hex(0xfda085), 0);
        lv_obj_set_style_bg_grad_dir(lv_boot.background, LV_GRAD_DIR_HOR, 0);
        lv_obj_set_style_bg_main_stop(lv_boot.background, 0, 0);
        lv_obj_set_style_bg_grad_stop(lv_boot.background, 255, 0);
        /*创建文本标签*/
        lv_boot.label = lv_label_create(screen);
        lv_obj_set_pos(lv_boot.label, APL_LV_BOOT_X + 30, APL_LV_BOOT_Y + 5);
        lv_label_set_text(lv_boot.label, "LOADING........");
        lv_obj_set_style_text_color(lv_boot.label, lv_color_white(), 0);
        lv_obj_set_style_text_font(lv_boot.label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_letter_space(lv_boot.label, 5, 0);
    }

    void apl::tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_CONTAINER_DISPLAY_E cur_disp)
    {
        mooncake::AbilityManager* am = mc.extensionManager();
        cur_overlap_container_disp = cur_disp;
        /*切UI*/
        switch(cur_disp){
            case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_ASSISTANT:
                lv_obj_clear_flag(OverLap_tile_container_Assistant, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(OverLap_tile_container_Painter, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(OverLap_tile_container_Gamepad, LV_OBJ_FLAG_HIDDEN);
            break;

            case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_PAINTER:
                lv_obj_clear_flag(OverLap_tile_container_Painter, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(OverLap_tile_container_Assistant, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(OverLap_tile_container_Gamepad, LV_OBJ_FLAG_HIDDEN);
            break;

            case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_GAMEPAD:
                lv_obj_clear_flag(OverLap_tile_container_Gamepad, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(OverLap_tile_container_Painter, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(OverLap_tile_container_Assistant, LV_OBJ_FLAG_HIDDEN);
            break;
        }

        /*若是已经在tile里，就直接切换APP*/
        if(lv_tileview_get_tile_active(tileview) == OverLap_tile){
            switch(cur_disp){
                case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_ASSISTANT:
                    if(am->getAppAbilityCurrentState(Gamepad_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(Gamepad_id);
                    if(am->getAppAbilityCurrentState(Painter_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(Painter_id);
                    if(am->getAppAbilityCurrentState(Assistant_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(Assistant_id);
                break;

                case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_PAINTER:
                    if(am->getAppAbilityCurrentState(Gamepad_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(Gamepad_id);
                    if(am->getAppAbilityCurrentState(Assistant_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(Assistant_id);
                    if(am->getAppAbilityCurrentState(Painter_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(Painter_id);
                break;

                case APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_GAMEPAD:
                    if(am->getAppAbilityCurrentState(Painter_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(Painter_id);
                    if(am->getAppAbilityCurrentState(Assistant_id) !=  mooncake::AppAbility::State_t::StateSleeping)am->closeAppAbility(Assistant_id);
                    if(am->getAppAbilityCurrentState(Gamepad_id) !=  mooncake::AppAbility::State_t::StateRunning)am->openAppAbility(Gamepad_id);
                break;
            }
        }else{
            lv_tileview_set_tile_by_index(tileview, APL_TILEVIEW_TILE_COL_ID_OVERLAP, APL_TILEVIEW_TILE_ROW_ID_OVERLAP, LV_ANIM_ON);
        }
    }

    void apl::init()
    {
        /*初始化屏幕控件*/
        screen = lv_scr_act();
        /*初始化视图控件*/
        tileview = lv_tileview_create(screen);
        /*初始化背景进度条控件*/
        init_lv_boot();
        /*隐藏滚动条代码*/
        lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
        /*设置屏幕背景为纯黑*/
        lv_obj_set_style_bg_color(tileview, lv_color_black(), 0);
        /*添加各个APP的tile*/
        WatchDial_tile = lv_tileview_add_tile(tileview, APL_TILEVIEW_TILE_COL_ID_WATCHDIAL, APL_TILEVIEW_TILE_ROW_ID_WATCHDIAL, LV_DIR_RIGHT);
        AppStore_tile = lv_tileview_add_tile(tileview, APL_TILEVIEW_TILE_COL_ID_APPSTORE, APL_TILEVIEW_TILE_ROW_ID_APPSTORE, (lv_dir_t)(LV_DIR_LEFT | LV_DIR_TOP));
        Setting_tile = lv_tileview_add_tile(tileview, APL_TILEVIEW_TILE_COL_ID_SETTING, APL_TILEVIEW_TILE_ROW_ID_SETTING, LV_DIR_BOTTOM);
        OverLap_tile = lv_tileview_add_tile(tileview, APL_TILEVIEW_TILE_COL_ID_OVERLAP, APL_TILEVIEW_TILE_ROW_ID_OVERLAP, LV_DIR_LEFT);
        OverLap_tile_container_Assistant = lv_obj_create(OverLap_tile);
        lv_obj_remove_style_all(OverLap_tile_container_Assistant);
        lv_obj_set_size(OverLap_tile_container_Assistant, APL_SCREEN_W, APL_SCREEN_H);
        lv_obj_add_flag(OverLap_tile_container_Assistant, LV_OBJ_FLAG_HIDDEN);
        OverLap_tile_container_Painter = lv_obj_create(OverLap_tile);
        lv_obj_remove_style_all(OverLap_tile_container_Painter);
        lv_obj_set_size(OverLap_tile_container_Painter, APL_SCREEN_W, APL_SCREEN_H);
        lv_obj_add_flag(OverLap_tile_container_Painter, LV_OBJ_FLAG_HIDDEN);
        OverLap_tile_container_Gamepad = lv_obj_create(OverLap_tile);
        lv_obj_remove_style_all(OverLap_tile_container_Gamepad);
        lv_obj_set_size(OverLap_tile_container_Gamepad, APL_SCREEN_W, APL_SCREEN_H);
        lv_obj_add_flag(OverLap_tile_container_Gamepad, LV_OBJ_FLAG_HIDDEN);
        /*添加视图回调事件*/
        lv_obj_add_event_cb(tileview, tileview_event_cb, LV_EVENT_ALL, this);



        /*初始化功能模块层*/
        fml::JpegDecoder::getInstance().Init(BLL_JPEG_DIR_PATH,BLL_JPEG_PIXEL_FORMAT,BLL_JPEG_ROTATE,JpegDecoderInputInfoCallBack,&lv_boot);
        fml::SpeechRecongnition::getInstance().sr_register_get_audio_callback(get_m_audio);
        fml::SpeechRecongnition::getInstance().init("M", cmd_phoneme, sizeof(cmd_phoneme) / sizeof(cmd_phoneme[0]));
        fml::TextToSpeech::getInstance().tts_register_set_audio_callback(set_m_audio);
        fml::TextToSpeech::getInstance().init();
        fml::BigModel::getInstance().init();
        /*初始化业务逻辑层*/
        bll::ArtificialIntelligence::getInstance().init();


        
        /*初始化各个应用,安装扩展*/
        WatchDial_id = mc.createExtension(std::make_unique<WatchDial>(screen, tileview, WatchDial_tile));
        AppStore_id = mc.createExtension(std::make_unique<AppStore>(screen, tileview, AppStore_tile, lv_marquee_icon_event_cb, this));
        Setting_id = mc.createExtension(std::make_unique<Setting>(screen, tileview, Setting_tile));
        Assistant_id = mc.createExtension(std::make_unique<Assistant>(screen, tileview, OverLap_tile_container_Assistant));
        Painter_id = mc.createExtension(std::make_unique<Painter>(screen, tileview, OverLap_tile_container_Painter));
        Gamepad_id = mc.createExtension(std::make_unique<GamePad>(screen, tileview, OverLap_tile_container_Gamepad));
    }

    void apl::update()
    {
        static struct fml::TextToSpeech::tts_speechinfo_t tts_resp = {
            .speech_string = "好的",
            .speech_speed = 3,
        };

        /*获取语音命令*/
        if(pdPASS == fml::SpeechRecongnition::getInstance().sr_get_result(&sr_result, 0)){
            ESP_LOGI(TAG, "sr_result:%d, %d, %s\n",sr_result.state, sr_result.command_id, sr_result.out_string.c_str());
            if(sr_result.state == 1){
                if(sr_result.command_id == sr_cmd[0].id){
                    tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_ASSISTANT);
                    fml::TextToSpeech::getInstance().tts_set_speech(&tts_resp, 0);
                }else if(sr_result.command_id == sr_cmd[1].id){
                    tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_PAINTER);
                    fml::TextToSpeech::getInstance().tts_set_speech(&tts_resp, 0);
                }else if(sr_result.command_id == sr_cmd[2].id){
                    tileview_overlap_container_set(APL_TILEVIEW_TILE_OVERLAP_CONTAINER_DISPLAY_GAMEPAD);
                    fml::TextToSpeech::getInstance().tts_set_speech(&tts_resp, 0);
                }else if(sr_result.command_id == sr_cmd[3].id){
                    lv_tileview_set_tile_by_index(tileview, APL_TILEVIEW_TILE_COL_ID_WATCHDIAL, APL_TILEVIEW_TILE_ROW_ID_WATCHDIAL, LV_ANIM_ON);
                    fml::TextToSpeech::getInstance().tts_set_speech(&tts_resp, 0);
                }else if(sr_result.command_id == sr_cmd[4].id){
                    lv_tileview_set_tile_by_index(tileview, APL_TILEVIEW_TILE_COL_ID_SETTING,APL_TILEVIEW_TILE_ROW_ID_SETTING, LV_ANIM_ON);
                    fml::TextToSpeech::getInstance().tts_set_speech(&tts_resp, 0);
                }
            }
        }
        
        
        mc.update();
    }




}





/**
 * @file SpeechRecongnition.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "SpeechRecongnition.hpp"

static bool is_record = false;
static std::string third_alog_lib_log;

/*声明原始 write 函数（由链接器提供）*/
extern "C" ssize_t __real_write(int fd, const void *buf, size_t count);
/*包装的 write 函数 - 捕获所有标准输出*/
extern "C" ssize_t __wrap_write(int fd, const void *buf, size_t nbytes) {

    const char* cbuf = static_cast<const char*>(buf);
    static std::string current_record;  /*当前记录缓存*/
    /*若检测到 "unfinished"，开始新记录*/
    if(strstr(cbuf, "unfinished") != NULL){
        current_record.assign(cbuf, nbytes);/*覆盖旧记录*/
        is_record = true;
    }else if(is_record){
        /*若正在记录中，追加内容*/
        current_record.append(cbuf, nbytes);
    }
    /*若检测到 "num_blanks"，结束记录并保存*/
    if(is_record &&  strstr(cbuf, "num_blanks") != NULL){
        third_alog_lib_log = current_record;  /*保存完整记录*/
        current_record.clear();               /*清空缓存*/
        is_record = false;
    }

    /*对于其他文件描述符，调用原始 write*/
    return __real_write(fd, buf, nbytes);
}

namespace fml{

    void SpeechRecongnition::sr_send_result(SpeechRecongnition* sr, esp_mn_results_t *mn_result, bool is_clean_trigger)
    {
        std::string result_str = "";
        /* 从后向前查找最后一个 "str: " 的位置 */
        size_t start_pos = third_alog_lib_log.rfind("str: ");/*使用 rfind 反向搜索*/
        if (start_pos != std::string::npos) {
            /*计算子串起始位置（跳过 "str: "）*/
            start_pos += 5; /*5 是 "str: " 的长度*/
            /*查找子串结束位置（下一个逗号）*/
            size_t end_pos = third_alog_lib_log.find(',', start_pos);
            if (end_pos == std::string::npos) {
                end_pos = third_alog_lib_log.length(); /*如果无逗号则取到字符串末尾*/
            }
            /*截取目标子串*/
            result_str = third_alog_lib_log.substr(start_pos, end_pos - start_pos);
        }

        if(is_clean_trigger && sr->clean_trigger_callback != NULL){
            sr->clean_trigger_callback(sr->clean_trigger_user_data, (char*)(result_str.c_str()));
        }

        int sr_command_id = mn_result->command_id[0];
        sr->sr_result.state = mn_result->state;
        sr->sr_result.command_id = sr_command_id;
        sr->sr_result.out_string = result_str;
        xQueueOverwrite(sr->g_result_que, &sr->sr_result);
    }

    void SpeechRecongnition::FeedTask(void *pvParam)
    {
        SpeechRecongnition* sr = (SpeechRecongnition*)pvParam;
        std::vector<int16_t> buffer(sr->afe_iface->get_feed_chunksize(sr->afe_data) * sr->afe_config->pcm_config.total_ch_num);
        ESP_LOGI(sr->TAG, "audio_chunksize=%d, feed_channel=%d", sr->afe_iface->get_feed_chunksize(sr->afe_data), sr->afe_config->pcm_config.total_ch_num);
        while(true){
            /* Read audio data from I2S bus */
            if(sr->audio_data_cb != NULL){
                /*获取audio各个channel数据*/
                if(sr->audio_data_cb(buffer) > 0){
                    /* Feed samples of an audio stream to the AFE_SR */
                    sr->afe_iface->feed(sr->afe_data, buffer.data());
                }
            }
        }
    }

    void SpeechRecongnition::DetectTask(void *pvParam)
    {
        SpeechRecongnition* sr = (SpeechRecongnition*)pvParam;
        /* Check if audio data has same chunksize with multinet */
        assert(sr->multinet->get_samp_chunksize(sr->model_data) == sr->afe_iface->get_fetch_chunksize(sr->afe_data));
        while(true){
            afe_fetch_result_t* res = sr->afe_iface->fetch(sr->afe_data);
            if (!res || res->ret_value == ESP_FAIL) {
                ESP_LOGE(sr->TAG, "fetch error!");
            }else{
                esp_mn_state_t mn_state = sr->multinet->detect(sr->model_data, res->data);
                esp_mn_results_t *mn_result = sr->multinet->get_results(sr->model_data);
                if(mn_state != ESP_MN_STATE_DETECTING){
                    sr_send_result(sr, mn_result, false);
                    if(mn_state == ESP_MN_STATE_DETECTED && sr->result_callback != NULL)sr->result_callback(sr->result_user_data, mn_result->string);
                    sr->multinet->clean(sr->model_data);
                }else{
                    if(res->vad_state == VAD_SILENCE && sr->clean_trigger){
                        /*3S内不说话就返回*/
                        if((xTaskGetTickCount() - sr->clean_trigger_time)*portTICK_PERIOD_MS <= 3000)continue;
                        /*提前触发，返回结果*/
                        sr->multinet->clean(sr->model_data);    
                        sr->clean_trigger = false;
                        sr_send_result(sr, mn_result, true);
                        sr->multinet->clean(sr->model_data);    
                    }
                }
            }
        }
    }

    SpeechRecongnition::SpeechRecongnition()
    {
        sr_result.state = ESP_MN_STATE_TIMEOUT;
        sr_result.command_id = 0;
        sr_result.out_string = "";
        model_data = NULL;
        multinet = NULL;
        afe_iface = NULL;
        afe_config = NULL;
        g_result_que = NULL;
        models = NULL;
        FeedTask_handle = NULL;
        DetectTask_handle = NULL;
        audio_data_cb = NULL;
        clean_trigger = false;
        clean_trigger_user_data = NULL;
        clean_trigger_callback = NULL;
        result_user_data = NULL;
        result_callback = NULL;
    }

    SpeechRecongnition::~SpeechRecongnition()
    {
        if(multinet != NULL && model_data != NULL)multinet->destroy(model_data);
        if(g_result_que != NULL)vQueueDelete(g_result_que);
        if(models != NULL)esp_srmodel_deinit(models);
        if(afe_config != NULL)afe_config_free(afe_config);
        if(FeedTask_handle != NULL)vTaskDelete(FeedTask_handle);
        if(DetectTask_handle != NULL)vTaskDelete(DetectTask_handle);
    }

    void SpeechRecongnition::sr_register_get_audio_callback(AudioDataCallBack_t audio_data_cb_)
    {
        if(audio_data_cb_ != NULL){
            audio_data_cb = audio_data_cb_;
        }
    }

    esp_err_t SpeechRecongnition::init(const char *input_format, char** command_list, uint8_t command_list_num)
    {
        /*获取队列*/
        g_result_que = xQueueCreate(1, sizeof(struct sr_result_t));
        ESP_RETURN_ON_FALSE(NULL != g_result_que, ESP_ERR_NO_MEM, TAG, "Failed create result queue");
        /*获取模型列表*/
        models = esp_srmodel_init("model");
        /*获取音频前端处理配置:一个麦克风通道，一个参考通道，用于语音识别，高精度*/
        afe_config = afe_config_init(input_format, models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
        /********** 语音增强 关键参数 麦克风阵列处理（单个麦克风无效）**********/
        afe_config->se_init = false;                                                       
        /********** 噪声抑制关键参数 **********/
        afe_config->ns_init = false;                                                     
        /********** VAD 关键参数 **********/
        afe_config->vad_init = true;
        //afe_config->vad_mode = VAD_MODE_4;                                                /*超高灵敏度模式*/   
        //afe_config->vad_min_speech_ms = 180;                                              /*命令词最短持续时间，根据你的命令词长度调整（确保能覆盖单个命令词）*/ 
        //afe_config->vad_min_noise_ms = 65;                                                /*命令词间的最短静音时间（即两个命令词之间的间隔需要大于这个值才会被分割）*/   
        //afe_config->vad_delay_ms  = 0;                                                    /*不延迟，尽快输出VAD结果*/                                    
        /********** 自动增益 关键参数 **********/
        afe_config->agc_init = true;                                                       
        /********** 回声消除 关键参数 **********/
        afe_config->aec_init = false;                                                       
        afe_config->aec_mode = AEC_MODE_SR_HIGH_PERF;                                       /*高性能AEC语音识别*/
        /********** 唤醒词 关键参数 **********/
        afe_config->wakenet_init = false;                                                  
        //afe_config->wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL); /*从模型列表中过滤出唤醒词（WakeNet）模型的名称*/
        /********** afe 关键参数 **********/
        afe_config->debug_init = true;                                                     /*关闭debug*/
        afe_config->afe_perferred_core = 1;                                                 /*afe任务的首选核心，在afe_create函数中创建。*/
        afe_config->afe_perferred_priority = SPEECHRECONGNITION_FEED_TASK_PRIOR;            /*afe任务的优先级，在afe_create函数中创建。*/
        //afe_config->afe_linear_gain = 1.2;                                                /*小幅增益补偿*/
        afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;                        /*内存优先分配模式*/
        /*获取音频前端处理句柄*/
        afe_iface = esp_afe_handle_from_config(afe_config);
        /*获取数据容器*/
        afe_data = afe_iface->create_from_config(afe_config);

        char *mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_CHINESE);
        if (NULL == mn_name) {
            ESP_LOGE(TAG, "No multinet model found");
            return ESP_FAIL;
        }
        /*获取multinet句柄*/
        multinet = esp_mn_handle_from_name(mn_name);
        model_data = multinet->create(mn_name, SPEECHRECONGNITION_MN_CONFIG_TIMEOUT_MS);
        //multinet->set_det_threshold(model_data, 0.0);

        esp_mn_commands_clear();
        if(command_list != NULL && command_list_num != 0){
            for(int i = 1; i <= command_list_num; i++){
                esp_mn_commands_add(i, (char *)command_list[i - 1]);
            }
        }
        esp_mn_commands_update();
        esp_mn_commands_print();
        multinet->print_active_speech_commands(model_data);

        BaseType_t ret_val = xTaskCreatePinnedToCore(FeedTask, "FeedTask", 6*1024, this, SPEECHRECONGNITION_FEED_TASK_PRIOR, &FeedTask_handle, SPEECHRECONGNITION_FEED_TASK_CORE);
        ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create feed task");

        ret_val = xTaskCreatePinnedToCore(DetectTask, "DetectTask", 2*4096, this, SPEECHRECONGNITION_DETECT_TASK_PRIOR, &DetectTask_handle, SPEECHRECONGNITION_DETECT_TASK_CORE);
        ESP_RETURN_ON_FALSE(pdPASS == ret_val, ESP_FAIL, TAG,  "Failed create detect task");


        return ESP_OK;
    }

    void SpeechRecongnition::sr_multinet_clean(MultinetCleanCallBack_t cb, void* user_data)
    {
        if(multinet != NULL && model_data != NULL){
            clean_trigger_time = xTaskGetTickCount();
            multinet->clean(model_data);
            clean_trigger = true;
            clean_trigger_callback = cb;
            clean_trigger_user_data = user_data;
        }
    }

    void SpeechRecongnition::sr_multinet_result(MultinetResultCallBack_t cb, void* user_data)
    {
        if(multinet != NULL && model_data != NULL){
            result_callback = cb;
            result_user_data = user_data;
        }
    }

    BaseType_t SpeechRecongnition::sr_get_result(struct sr_result_t *result, TickType_t xTicksToWait)
    {
        return xQueueReceive(g_result_que, result, xTicksToWait);
    }

    bool SpeechRecongnition::sr_add_command_list(char** command_list, uint8_t command_list_num)
    {
        int s_index = 0;

        if(command_list != NULL && command_list_num != 0)
        {
            /*查找第一个index*/
            for(int i = 1; i < SPEECHRECONGNITION_MN_COMMAND_MAX_NUM; i++){
                if(esp_mn_commands_get_string(i) == NULL){s_index = i;break;}
            }
            /*判断是否足够位置*/
            if(SPEECHRECONGNITION_MN_COMMAND_MAX_NUM - s_index - 1 >= command_list_num){
                for(int i = 0; i < command_list_num; i++){
                    esp_mn_commands_add(s_index + i, command_list[i]);
                }
                esp_mn_commands_update();
                return true;
            }
        }

        return false;
    }

    void SpeechRecongnition::sr_remove_command_list(char** command_list, uint8_t command_list_num)
    {
        if(command_list != NULL && command_list_num != 0){
            for(int i = 0; i < command_list_num; i++){
                esp_mn_commands_remove(command_list[i]);
            }
            esp_mn_commands_update();
        }
    }

}






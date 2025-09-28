/**
 * @file JpegDecoder.cpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "JpegDecoder.hpp"

namespace fml{

    long JpegDecoder::get_dir_number(const char *filepath)
    {
        /*存储文件个数*/
        long FileNumber = 0;
        /*先打开文件*/
        DIR *dir = opendir(filepath);
        /*判断是否打开成功*/
        if (NULL == dir)				
        {
            perror("打开文件");
            exit(1);
        }
        /*dirent结构体指针，用于指向数据*/
        struct dirent *di;		
        /*用于拼接字符串。遇到子目录。*/		
        char p_file[1024];				

        while ((di = readdir(dir)) != NULL)
        {
            /*要忽略掉.和 .. 如果读到它们，就不要对他们操作。strcmp函数用于比较字符串，相等返回0；*/
            if (strcmp(di->d_name, ".") == 0 || strcmp(di->d_name, "..") == 0)
            {
                /*忽略掉*/
                continue;
            }
            else if (di->d_type == DT_DIR)
            {
                /*遇到目录就要进入，使用递归*/
                sprintf(p_file, "%s / %s", filepath, di->d_name);
                /*这里是+=*/
                FileNumber += get_dir_number(p_file);
            }
            else
            {
                /*我这里是统计的所有文件，不管是什么类型的。可以自定义条件，统计不同类型的文件数目*/
                FileNumber++;
            }
        }

        closedir(dir);
        return FileNumber;
    }
    /*函数用于获取目录下的第N个文件*/
    bool JpegDecoder::get_nth_file(const char *path, int n, char* filename, int filename_len)
    {
        DIR *dir;
        struct dirent *entry;
        int count = 0;

        if(filename_len < 256){
            return false;
        }
    
        if ((dir = opendir(path)) == NULL) {
            perror("Failed to open directory");
            return false;
        }
    
        while ((entry = readdir(dir)) != NULL) {
            /*跳过"."和".."目录*/
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            count++;
            if (count == n) {
                strncpy(filename, entry->d_name, 256);
                break;
            }
        }
        closedir(dir);
        if (count < n) {
            /*如果目录中的文件少于n个，返回NULL*/
            return false;
        }
        return true;
    }

    void JpegDecoder::get_jpeg_input_info(void* input_info_user_data, JpegDecoderInputInfoCallBack_t cb, const char* TAG, const char *dirpath, struct jpeg_input_info_t* jpeg_input_info)
    {
        char filePath[512];
        char filename[256];
        struct stat fileStat;
        FILE *f = NULL;

        /*获取文件总数*/
        jpeg_input_info->jpeg_number = get_dir_number(dirpath);
        /*获取各个文件的信息*/
        if(jpeg_input_info->jpeg_number != 0){
            jpeg_input_info->jpeg_buff = (uint8_t**)heap_caps_malloc(jpeg_input_info->jpeg_number * sizeof(uint8_t*), MALLOC_CAP_SPIRAM);
            if(jpeg_input_info->jpeg_buff != NULL){
                /*分配用于存储各个文件大小信息的缓存*/
                jpeg_input_info->jpeg_len = (int*)heap_caps_malloc(jpeg_input_info->jpeg_number * sizeof(int), MALLOC_CAP_SPIRAM);
                if(jpeg_input_info->jpeg_len == NULL){
                    heap_caps_free(jpeg_input_info->jpeg_buff);
                    jpeg_input_info->jpeg_number = 0;
                    return;
                }else{
                    for(int i = 0; i < jpeg_input_info->jpeg_number; i++){
                        /*获取文件路径*/
                        if(get_nth_file(dirpath, i + 1, filename, 256) != true){
                            memset(filename, 0, sizeof(filename));
                        }
                        snprintf(filePath, 512, "%s/%s", dirpath, filename);
                        if (stat(filePath, &fileStat) == 0) {
                            /*获取jpeg文件缓存空间和jpeg文件大小*/
                            jpeg_input_info->jpeg_buff[i] = (uint8_t*)heap_caps_malloc(fileStat.st_size, MALLOC_CAP_SPIRAM);
                            jpeg_input_info->jpeg_len[i] = fileStat.st_size;
                            /*读取文件*/
                            f = fopen(filePath, "r");
                            if(f == NULL){
                                ESP_LOGE(TAG, "Failed to open file for reading");
                                if(jpeg_input_info->jpeg_buff[i] != NULL)heap_caps_free(jpeg_input_info->jpeg_buff[i]);
                                jpeg_input_info->jpeg_len[i] = 0;
                            }else{
                                fread(jpeg_input_info->jpeg_buff[i], 1, jpeg_input_info->jpeg_len[i], f);
                                fclose(f);
                                f = NULL;
                            }
                        } else {
                            jpeg_input_info->jpeg_buff[i] = NULL;
                            jpeg_input_info->jpeg_len[i] = 0;
                        }
                        if(cb != NULL)cb(input_info_user_data, jpeg_input_info->jpeg_number, i);
                        //ESP_LOGI(TAG, "get_jpeg_input_info:%d,%p,%d",i,(void*)jpeg_input_info->jpeg_buff[i], jpeg_input_info->jpeg_len[i]);
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
            }
        }
    }

    void JpegDecoder::JpegDecTask(void * arg)
    {
        int ret;
        JpegDecoder* app = (JpegDecoder*)arg;
        //int start, end, duration_us;
        while(1)
        {
            xEventGroupWaitBits(app->xEventGroup,EVENTGROUP_JPEG_DEC_START_BIT,pdTRUE,pdTRUE,portMAX_DELAY);
            //start = esp_timer_get_time();
            if(app->jpeg_input_info.jpeg_index <= app->jpeg_input_info.jpeg_number){
                if(app->jpeg_input_info.jpeg_buff != NULL){
                    if(app->jpeg_input_info.jpeg_buff[app->jpeg_input_info.jpeg_index - 1] != NULL){
                        /*Set input buffer and buffer len to io_callback*/
                        app->jpeg_stream.jpeg_io.inbuf = app->jpeg_input_info.jpeg_buff[app->jpeg_input_info.jpeg_index - 1];
                        app->jpeg_stream.jpeg_io.inbuf_len = app->jpeg_input_info.jpeg_len[app->jpeg_input_info.jpeg_index - 1];
                        if(app->jpeg_output_info.jpeg_buff != NULL){
                            /*Parse jpeg picture header and get picture for user and decoder*/
                            ret = jpeg_dec_parse_header(app->jpeg_stream.jpeg_dec, &app->jpeg_stream.jpeg_io, &app->jpeg_stream.out_info);
                            if (ret != JPEG_ERR_OK) {
                                ESP_LOGE(app->TAG, "jpeg_dec_parse_header failed:%d", ret);
                            }else{
                                /*Start decode jpeg*/
                                ret = jpeg_dec_process(app->jpeg_stream.jpeg_dec, &app->jpeg_stream.jpeg_io);
                                if (ret != JPEG_ERR_OK) {
                                    ESP_LOGE(app->TAG, "jpeg_dec_process failed:%d", ret);
                                }else{
                                    //ESP_LOGI(app->TAG, "jpeg_dec_process success");
                                }
                            }
                        }else{
                            ESP_LOGE(app->TAG, "jpeg_output_info.jpeg_buff malloc buffer from PSRAM fialed");
                        }
                    }else{
                        ESP_LOGE(app->TAG, "app->jpeg_input_info.jpeg_buff[%d] malloc buffer from PSRAM fialed", app->jpeg_input_info.jpeg_index - 1);
                    }
                }else{
                    ESP_LOGE(app->TAG, "app->jpeg_input_info.jpeg_buff malloc buffer from PSRAM fialed");
                }
            }
            //end = esp_timer_get_time();
            //duration_us = end - start;
            //ESP_LOGI(app->TAG, "JpegDecTask hs:%d,%.3f", duration_us, (float)duration_us / 1000);
            //vTaskDelay(pdMS_TO_TICKS(LV_DEF_REFR_PERIOD));
            xEventGroupSetBits(app->xEventGroup, EVENTGROUP_JPEG_DEC_END_BIT);
        }
    }

    JpegDecoder::JpegDecoder()
    {       
        xEventGroup = NULL;
        memset(&jpeg_input_info, 0, sizeof(jpeg_input_info));
        memset(&jpeg_stream, 0, sizeof(jpeg_stream));
        JpegDecTask_handle = NULL;
        ESP_LOGI(TAG, "JpegDecoder on construct");
    }

    JpegDecoder::~JpegDecoder()
    {
        if(xEventGroup != NULL)vEventGroupDelete(xEventGroup);

        if(jpeg_input_info.jpeg_buff != NULL){
            for(int i = 0; i < jpeg_input_info.jpeg_number; i++){
                if(jpeg_input_info.jpeg_buff[i] != NULL){
                    heap_caps_free(jpeg_input_info.jpeg_buff[i]);
                }
            }
            heap_caps_free(jpeg_input_info.jpeg_buff);
        }
        if(jpeg_input_info.jpeg_len != NULL)heap_caps_free(jpeg_input_info.jpeg_len);
        
        if(jpeg_stream.jpeg_dec != NULL)jpeg_dec_close(jpeg_stream.jpeg_dec);
        if(JpegDecTask_handle != NULL)vTaskDelete(JpegDecTask_handle);

        ESP_LOGI(TAG, "JpegDecoder on deconstruct");
    }

    void JpegDecoder::Init(const char* dirpath, jpeg_pixel_format_t format, jpeg_rotate_t rotate, JpegDecoderInputInfoCallBack_t cb, void* input_info_user_data)
    {
        /*创建事件组*/
        xEventGroup = xEventGroupCreate();
        /*获取jpeg输入缓存信息，默认第一个文件开始*/
        jpeg_input_info.jpeg_index = 1;
        get_jpeg_input_info(input_info_user_data, cb, TAG, dirpath, &jpeg_input_info);
        /*设置解码配置*/
        jpeg_stream.config.output_type = format;
        jpeg_stream.config.rotate = rotate;
        /*Create jpeg_dec handle*/
        jpeg_dec_open(&jpeg_stream.config, &jpeg_stream.jpeg_dec);
        /*创建解码任务*/
        xTaskCreatePinnedToCore(JpegDecTask, 
                                "JpegDecTask", 
                                4096, 
                                this, 
                                JPEGDECODER_TASK_PRIOR, 
                                &JpegDecTask_handle, 
                                JPEGDECODER_TASK_CORE);
        ESP_LOGI(TAG, "JpegDecoder on create");
    }

    void JpegDecoder::StartJpegDec(int jpeg_index, struct jpeg_output_info_t output)
    {
        /*获取jpeg输出缓存信息*/
        jpeg_output_info = output;
        jpeg_stream.jpeg_io.outbuf = jpeg_output_info.jpeg_buff;
        /*设置需要解码的坐标*/
        jpeg_input_info.jpeg_index = jpeg_index;
        /*开启解码*/
        xEventGroupSetBits(xEventGroup, EVENTGROUP_JPEG_DEC_START_BIT);
    }

    bool JpegDecoder::WaitJpegDec(int* jpeg_index, TickType_t xTicksToWait)
    {
        /*等待解码*/
        if((xEventGroupWaitBits(xEventGroup,EVENTGROUP_JPEG_DEC_END_BIT,pdFALSE,pdTRUE,xTicksToWait) & 
                EVENTGROUP_JPEG_DEC_END_BIT) == 
                EVENTGROUP_JPEG_DEC_END_BIT){
                    /*手动清除*/
                    xEventGroupClearBits(xEventGroup, EVENTGROUP_JPEG_DEC_END_BIT);
                    if(jpeg_index != NULL)*jpeg_index = jpeg_input_info.jpeg_index;
                    return true;
                }
        return false;
    }

    int JpegDecoder::SafeStrtoi(const char *str, int *value)
    {
        if (!str || *str == '\0') {
            return -1;  /*空指针或空字符串*/
        }

        char *endptr;
        long result = strtol(str, &endptr, 10);
        
        /*检查转换错误*/
        if (str == endptr) {
            return -2;  /*无数字可转换*/
        }
        
        /*检查是否整个字符串都转换了*/
        if (*endptr != '\0') {
            return -3;  /*包含非数字字符*/
        }
        
        /*检查溢出*/
        if (result < INT_MIN || result > INT_MAX) {
            return -4;  /*超出int范围*/
        }
        
        *value = (int)result;
        return 0;
    }

    bool JpegDecoder::FindJpegMaterial(const char *path, const char *target, int* start, int* end)
    {
        if (!path || *path == '\0') {
            /*错误：路径字符串为空*/
            return false;
        }

        if (!target || *target == '\0') {
            /*错误：目标字符串为空*/
            return false;
        }

        if (!start || !end) {
            /*错误：输出参数无效*/
            return false;
        }

        FILE *file = fopen(path, "r");
        if (!file) {
            /*无法打开文件*/
            return false;
        }

        char buffer[256];
        bool found = false;

        while (fgets(buffer, sizeof(buffer), file)) {
            /*移除换行符（处理\r\n和\n两种情况）*/
            size_t len = strcspn(buffer, "\r\n");
            if (len < sizeof(buffer)) {
                buffer[len] = '\0';
            }
            
            /*分割键值对*/
            char *colon = strchr(buffer, ':');
            if (!colon) continue;
            
            /*分割名称和范围*/
            *colon = '\0';
            char *name = buffer;
            char *range = colon + 1;
            
            /*大小写不敏感比较*/
            if (strcasecmp(name, target) != 0) continue;
            
            /*分割范围*/
            char *dash = strchr(range, '-');
            if (!dash) {
                /*警告: target 范围格式无效*/
                continue;
            }
            
            /*提取第一个数字*/
            *dash = '\0';
            int temp_start, temp_end;
            
            if (SafeStrtoi(range, &temp_start) != 0) {
                /*错误: target 起始数字无效: range*/
                *dash = '-';  /*恢复字符串*/
                continue;
            }
            
            /*提取第二个数字*/
            if (SafeStrtoi(dash + 1, &temp_end) != 0) {
                /*错误: target 结束数字无效: dash + 1*/
                *dash = '-';  /*恢复字符串*/
                continue;
            }
            
            /*验证范围*/
            if (temp_start > temp_end) {
                /*错误: target 范围无效 (temp_start > temp_end)*/
                *dash = '-';
                continue;
            }
            
            /*成功找到有效范围*/
            *start = temp_start;
            *end = temp_end;
            found = true;
            break;
        }

        fclose(file);
        return found;
    }

    /*实现静态解码函数*/
    JpegDecoder::DecodeResult JpegDecoder::Decode(
        const uint8_t* jpeg_data, 
        size_t jpeg_size,
        int target_width,
        int target_height,
        jpeg_pixel_format_t output_format,
        jpeg_rotate_t rotate
    ) {
        DecodeResult result;
        
        /*验证JPEG签名*/
        if (jpeg_size < 2 || jpeg_data[0] != 0xFF || jpeg_data[1] != 0xD8) {
            result.success = false;
            result.error_message = "Invalid JPEG signature";
            return result;
        }
        
        /*配置解码参数*/
        jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
        config.output_type = output_format;
        config.rotate = rotate;
        
        /*设置缩放尺寸（如果需要）*/
        if (target_width > 0 && target_height > 0) {
            config.scale.width = target_width & ~0x7;  /*确保是8的倍数*/
            config.scale.height = target_height & ~0x7; /*确保是8的倍数*/
            
            /*如果计算后为0，则设置为8*/
            if (config.scale.width == 0) config.scale.width = 8;
            if (config.scale.height == 0) config.scale.height = 8;
        }
        
        jpeg_dec_handle_t jpeg_dec = nullptr;
        jpeg_error_t err = jpeg_dec_open(&config, &jpeg_dec);
        if (err != JPEG_ERR_OK) {
            result.success = false;
            result.error_message = "Failed to open JPEG decoder: " + std::to_string(err);
            return result;
        }
        
        jpeg_dec_io_t io = {
            .inbuf = const_cast<uint8_t*>(jpeg_data), 
            .inbuf_len = static_cast<int>(jpeg_size),
            .inbuf_remain = 0,
            .outbuf = nullptr,
            .out_size = 0
        };
        
        jpeg_dec_header_info_t info;
        err = jpeg_dec_parse_header(jpeg_dec, &io, &info);
        if (err != JPEG_ERR_OK) {
            jpeg_dec_close(jpeg_dec);
            result.success = false;
            result.error_message = "Failed to parse JPEG header: " + std::to_string(err);
            return result;
        }
        
        int outbuf_size = 0;
        err = jpeg_dec_get_outbuf_len(jpeg_dec, &outbuf_size);
        if (err != JPEG_ERR_OK || outbuf_size == 0) {
            jpeg_dec_close(jpeg_dec);
            result.success = false;
            result.error_message = "Failed to get output buffer size: " + std::to_string(err);
            return result;
        }
        
        /*分配输出缓冲区 (16字节对齐)*/
        uint8_t* outbuf = (uint8_t*)heap_caps_aligned_alloc(16, outbuf_size, MALLOC_CAP_SPIRAM);
        if (!outbuf) {
            jpeg_dec_close(jpeg_dec);
            result.success = false;
            result.error_message = "Failed to allocate output buffer";
            return result;
        }
        
        io.outbuf = outbuf;
        io.out_size = outbuf_size;
        
        err = jpeg_dec_process(jpeg_dec, &io);
        jpeg_dec_close(jpeg_dec);
        
        if (err != JPEG_ERR_OK) {
            free(outbuf);
            result.success = false;
            result.error_message = "JPEG decoding failed: " + std::to_string(err);
            return result;
        }
        
        /*设置结果*/
        result.width = config.scale.width > 0 ? config.scale.width : info.width;
        result.height = config.scale.height > 0 ? config.scale.height : info.height;
        result.data = std::unique_ptr<uint8_t[]>(outbuf); /*转移所有权*/
        result.data_size = outbuf_size;
        result.success = true;
        
        return result;
    }

}


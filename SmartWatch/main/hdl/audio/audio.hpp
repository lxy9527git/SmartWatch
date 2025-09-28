/**
 * @file audio.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <driver/gpio.h>
#include <driver/i2s_common.h>
#include <driver/i2s_std.h>
#include <esp_log.h>

namespace hdl{

    class audio
    {
        #define AUDIO_DEFAULT_OUTPUT_VOLUME                             (70)
        /*功放MAX98357支持16/24/32位数据; 麦克风MSM261S4030H0R是24位ADC*/
        struct Config_t{
            uint32_t input_sample_rate;
            uint32_t output_sample_rate;
            int16_t spk_bclk;
            int16_t spk_ws;
            int16_t spk_dout;
            int16_t mic_sck;
            int16_t mic_ws;
            int16_t mic_din;
        };

        private:
            const char *TAG = "audio";
            static constexpr int AUDIO_CODEC_DMA_DESC_NUM = 8;
            static constexpr int AUDIO_CODEC_DMA_FRAME_NUM = 512;
            struct Config_t _cfg;
            i2s_chan_handle_t tx_handle_ = NULL;
            i2s_chan_handle_t rx_handle_ = NULL;
            int output_volume = AUDIO_DEFAULT_OUTPUT_VOLUME; /*0 - 100*/
            

            /*私有构造函数，禁止外部直接实例化*/
            audio(){}
            /*禁止拷贝构造和赋值操作*/
            audio(const audio&) = delete;
            audio& operator = (const audio&) = delete;
            inline void init_speaker(){
                i2s_chan_config_t chan_cfg = {
                    .id = I2S_NUM_0,
                    .role = I2S_ROLE_MASTER,
                    .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
                    .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
                    .auto_clear_after_cb = true,
                    .auto_clear_before_cb = false,
                    .allow_pd = false,
                    .intr_priority = 0,
                };
                ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, NULL));

                i2s_std_config_t std_cfg = {
                    .clk_cfg = { 
                        .sample_rate_hz = _cfg.output_sample_rate,
                        .clk_src = I2S_CLK_SRC_DEFAULT,
                        .ext_clk_freq_hz = 0,
                        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
                    },
                    .slot_cfg = {
                        .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
                        .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
                        .slot_mode = I2S_SLOT_MODE_MONO,      
                        .slot_mask = I2S_STD_SLOT_LEFT, 
                        .ws_width = I2S_DATA_BIT_WIDTH_32BIT, 
                        .ws_pol = false, 
                        .bit_shift = true, 
                        .left_align = false, 
                        .big_endian = false, 
                        .bit_order_lsb = false 
                    },
                    .gpio_cfg = {
                        .mclk = I2S_GPIO_UNUSED,
                        .bclk = (gpio_num_t)_cfg.spk_bclk,
                        .ws = (gpio_num_t)_cfg.spk_ws,
                        .dout = (gpio_num_t)_cfg.spk_dout,
                        .din = I2S_GPIO_UNUSED,
                        .invert_flags = {
                            .mclk_inv = false,
                            .bclk_inv = false,
                            .ws_inv = false         /*MAX98357 需要 WS 不反相*/
                        }
                    }
                };

                ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));
                ESP_LOGI(TAG, "Speaker initialized in 32-bit mode");
            }

            inline void init_microphone(){
                i2s_chan_config_t chan_cfg = { 
                    .id = I2S_NUM_1, 
                    .role = I2S_ROLE_MASTER, 
                    .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM, 
                    .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM, 
                    .auto_clear_after_cb = true, 
                    .auto_clear_before_cb = false, 
                    .allow_pd = false, 
                    .intr_priority = 0, 
                };
                ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_handle_));

                i2s_std_config_t std_cfg = {
                    .clk_cfg = { 
                        .sample_rate_hz = _cfg.input_sample_rate, 
                        .clk_src = I2S_CLK_SRC_DEFAULT, 
                        .ext_clk_freq_hz = 0, 
                        .mclk_multiple = I2S_MCLK_MULTIPLE_256, 
                    },
                    .slot_cfg = { 
                        .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT, 
                        .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, 
                        .slot_mode = I2S_SLOT_MODE_MONO, 
                        .slot_mask = I2S_STD_SLOT_LEFT, 
                        .ws_width = I2S_DATA_BIT_WIDTH_32BIT, 
                        .ws_pol = false, 
                        .bit_shift = true, 
                        .left_align = false, 
                        .big_endian = false, 
                        .bit_order_lsb = false 
                    },
                    .gpio_cfg = {
                        .mclk = I2S_GPIO_UNUSED,
                        .bclk = (gpio_num_t)_cfg.mic_sck,
                        .ws = (gpio_num_t)_cfg.mic_ws,
                        .dout = I2S_GPIO_UNUSED,
                        .din = (gpio_num_t)_cfg.mic_din,
                        .invert_flags = {
                            .mclk_inv = false,
                            .bclk_inv = false,
                            .ws_inv = false              
                        }
                    }
                };
                ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
                ESP_LOGI(TAG, "Microphone initialized in 24-bit mode(32-bit container)");
            }
        public:
            /*获取单例实例的静态方法*/
            inline static audio& getInstance() {
                static audio instance;
                return instance;
            }

            inline void init(uint32_t input_sample_rate,
                            uint32_t output_sample_rate,
                            int16_t spk_bclk,
                            int16_t spk_ws,
                            int16_t spk_dout,
                            int16_t mic_sck,
                            int16_t mic_ws,
                            int16_t mic_din)
            {
                _cfg.input_sample_rate = input_sample_rate;
                _cfg.output_sample_rate = output_sample_rate;
                _cfg.spk_bclk = spk_bclk;
                _cfg.spk_ws = spk_ws;
                _cfg.spk_dout = spk_dout;
                _cfg.mic_sck = mic_sck;
                _cfg.mic_ws = mic_ws;
                _cfg.mic_din = mic_din;

                /*Create a new channel for speaker*/
                init_speaker();
                /*Create a new channel for MIC*/
                init_microphone();
                /*等待硬件稳定*/    
                vTaskDelay(pdMS_TO_TICKS(100));
                Start();
            }
            /*写入音频数据到MAX98357*/
            inline int write(const int32_t* data, int samples)
            {
                std::vector<int32_t> buffer(samples);
                /*output_volume: 0-100*/
                /*volume_factor_: 0-65536*/
                int32_t volume_factor = pow(double(output_volume) / 100.0, 2) * 65536 * 2;
                for(int i = 0; i < samples; i++) {
                    int64_t scaled_sample = static_cast<int64_t>(data[i]) * volume_factor;
                    /*饱和处理*/
                    if(scaled_sample > INT32_MAX) scaled_sample = INT32_MAX;
                    else if (scaled_sample < INT32_MIN) scaled_sample = INT32_MIN;
                    buffer[i] = static_cast<int32_t>(scaled_sample);
                }

                size_t bytes_written;
                esp_err_t err = i2s_channel_write(
                    tx_handle_, 
                    buffer.data(), 
                    samples * sizeof(int32_t),          
                    &bytes_written, 
                    portMAX_DELAY
                );
                
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
                    return 0;
                }
                return bytes_written / sizeof(int32_t);
            }
            /*从MSM261S4030H0R读取24位音频数据*/
            inline int read(int32_t* dest, int samples)
            {
                size_t bytes_read;
                esp_err_t err = i2s_channel_read(
                    rx_handle_, 
                    dest, 
                    samples * sizeof(int32_t), 
                    &bytes_read, 
                    portMAX_DELAY
                );
                
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Read failed: %s", esp_err_to_name(err));
                    return 0;
                }

                return bytes_read / sizeof(int32_t);
            }

            inline int OutputData(std::vector<int32_t>& data)
            {
                return write(data.data(), data.size());
            }

            inline int InputData(std::vector<int32_t>& data)
            {
                return read(data.data(), data.size());
            }

            inline void Start()
            {
                ESP_ERROR_CHECK(i2s_channel_enable(tx_handle_));
                ESP_ERROR_CHECK(i2s_channel_enable(rx_handle_));
                ESP_LOGI(TAG, "Audio channels enabled");
            }

            inline void Stop()
            {
                ESP_ERROR_CHECK(i2s_channel_disable(tx_handle_));
                ESP_ERROR_CHECK(i2s_channel_disable(rx_handle_));
                ESP_LOGI(TAG, "Audio channels disable");
            }

            inline void SetOutputVolume(int volume)
            {
                output_volume = std::clamp(volume, 0, 100);
                ESP_LOGI(TAG, "Volume set to %d%%", output_volume);
            }
    };

    // /*生成1kHz正弦波测试功放（24位范围）*/
    // static std::vector<int32_t> generate_sine_wave(int samples, float freq, int sr) {
    //     std::vector<int32_t> wave(samples);
    //     const double phase_inc = 2.0 * M_PI * freq / sr;
    //     double phase = 0.0;
        
    //     /*使用24位最大振幅 (0x7FFFFF = 8388607)*/
    //     const int32_t max_amp = 0x7FFFFF;
        
    //     for(int i = 0; i < samples; i++) {
    //         wave[i] = static_cast<int32_t>(max_amp * sin(phase));
    //         phase += phase_inc;
    //         if(phase > 2*M_PI) phase -= 2*M_PI;
    //     }
    //     return wave;
    // }

    // /* 1kHz正弦波测试 */
    // static void audio_test_1KHZ() {
    //     auto& audio = hdl::audio::getInstance();
    //     // 生成24位范围的正弦波
    //     auto sine_wave = generate_sine_wave(AUDIO_OUTPUT_SAMPLE_RATE, 1000, AUDIO_OUTPUT_SAMPLE_RATE);
        
    //     // 设置合理音量避免饱和（50%音量）
    //     audio.SetOutputVolume(30);
        
    //     while (true) {
    //         audio.OutputData(sine_wave);
    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
    // }

    // static void audio_loopback_test() {
    //     auto& audio = hdl::audio::getInstance();
    //     std::vector<int32_t> buffer(1024);

    //     audio.SetOutputVolume(30);
        
    //     while (true) {
    //         int samples = audio.InputData(buffer);
    //         if (samples > 0) {
    //             audio.OutputData(buffer);
    //         }
    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
    // }

    // static void audio_read_test() {
    //     auto& audio = hdl::audio::getInstance();
    //     std::vector<int32_t> buffer(10);
        
    //     while (true) {
    //         int samples = audio.InputData(buffer);
    //         for(auto s : buffer) {
    //             ESP_LOGI("audio_read_test", "%08X ", (unsigned int)s);
    //         }
    //         vTaskDelay(pdMS_TO_TICKS(10));
    //     }
    // }

}
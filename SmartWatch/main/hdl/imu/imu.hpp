/**
 * @file imu.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once
#include <esp_log.h>
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <algorithm>
#include "hdl_config.hpp"

namespace hdl {

    class imu
    {
        #define IMU_CS                      LSM_CS
        #define IMU_MOSI                    LSM_MOSI
        #define IMU_SCK                     LSM_SCK
        #define IMU_MISO                    LSM_MISO
        #define IMU_IQ                      LSM_IQ

        #define IMU_TASK_PRIOR              (2)
        #define IMU_TASK_CORE               (0)
        #define IMU_TASK_DELAY_MS           (50)  /*降低延迟以提高响应性*/
        /*常量定义*/
        #define GYRO_SCALE                  2000.0f  /*±2000dps*/
        #define ACCEL_SCALE                 8.0f     /*±8g*/
        #define GYRO_SENSITIVITY            (GYRO_SCALE / 32768.0f)  /*dps per LSB*/
        #define ACCEL_SENSITIVITY           (ACCEL_SCALE / 32768.0f) /*g per LSB*/
        #define RAD_TO_DEG                  (180.0f / M_PI)
        #define DEG_TO_RAD                  (M_PI / 180.0f)
        #define COMPLEMENTARY_FILTER_ALPHA  0.85f    /*互补滤波器系数，降低到0.85以获得更快响应*/

        private:
            const char *TAG = "LSM6DS3TR-C";
            uint16_t joystick_lx;
            uint16_t joystick_ly;
            spi_device_handle_t spi;
            spi_host_device_t spi_host;
            TaskHandle_t imu_task_handle;
            bool is_running = false;                        /*运行状态标志*/
            /*SPI总线是否初始化标志*/
            bool spi_initialized = false;
            /*SPI总线配置*/
            spi_bus_config_t bus_config;
            /*SPI设备配置*/
            spi_device_interface_config_t dev_config;
            /*左手佩戴标志*/
            bool left_handed = true;                       /* true for left hand, false for right hand */

            /*姿态估计相关变量*/
            float pitch, roll, yaw;                         /*俯仰角、横滚角和偏航角（度）*/
            float pitch_acc, roll_acc;                      /*加速度计计算的角度*/
            int16_t ax_offset, ay_offset, az_offset;        /*加速度计偏移*/
            int16_t gx_offset, gy_offset, gz_offset;        /*陀螺仪偏移*/
            uint32_t last_update_time;                      /*上次更新时间（毫秒）*/
            bool calibrated = false;                        /*校准标志*/
            uint32_t calibration_samples = 0;               /*校准采样计数*/
            int64_t ax_sum, ay_sum, az_sum;                 /*加速度计采样总和*/
            int64_t gx_sum, gy_sum, gz_sum;                 /*陀螺仪采样总和*/

            /*陀螺仪滤波变量*/
            float gx_filtered = 0, gy_filtered = 0, gz_filtered = 0;
            const float GYRO_FILTER_ALPHA = 0.1f;           /*陀螺仪滤波器系数，从0.7降低到0.1*/

            /*初始化SPI总线*/
            esp_err_t init_spi_bus() 
            {
                if (spi_initialized) {
                    return ESP_OK;          /*SPI已经初始化*/
                }
                
                bus_config = {
                    .mosi_io_num = IMU_MOSI,
                    .miso_io_num = IMU_MISO,
                    .sclk_io_num = IMU_SCK,
                    .quadwp_io_num = -1,
                    .quadhd_io_num = -1,
                    .data4_io_num = -1,
                    .data5_io_num = -1,
                    .data6_io_num = -1,
                    .data7_io_num = -1,
                    .max_transfer_sz = 0,
                    .flags = SPICOMMON_BUSFLAG_MASTER,
                    .intr_flags = 0
                };
                
                esp_err_t ret = spi_bus_initialize(spi_host, &bus_config, SPI_DMA_CH_AUTO);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to initialize SPI bus");
                    return ret;
                }
                
                spi_initialized = true;
                ESP_LOGI(TAG, "SPI bus initialized");
                return ESP_OK;
            }

            /*初始化SPI设备*/
            esp_err_t init_spi_device() 
            {
                if (spi != NULL) {
                    return ESP_OK;      /*设备已经初始化*/
                }
                
                dev_config = {
                    .command_bits = 0,
                    .address_bits = 8,
                    .dummy_bits = 0,
                    .mode = 3,                                  /*CPOL=1, CPHA=1*/
                    .duty_cycle_pos = 0,
                    .cs_ena_pretrans = 0,
                    .cs_ena_posttrans = 0,
                    .clock_speed_hz = 10 * 1000 * 1000,         /*10MHz*/
                    .input_delay_ns = 0,
                    .spics_io_num = IMU_CS,
                    .flags = 0,
                    .queue_size = 7,
                    .pre_cb = nullptr,
                    .post_cb = nullptr
                };
                
                esp_err_t ret = spi_bus_add_device(spi_host, &dev_config, &spi);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to add SPI device");
                    return ret;
                }
                
                ESP_LOGI(TAG, "SPI device initialized");
                return ESP_OK;
            }

            /*SPI写函数*/
            inline esp_err_t write_reg(uint8_t reg, uint8_t value) 
            {
                if (!spi_initialized || spi == NULL) return ESP_FAIL;

                spi_transaction_t t = {
                    .flags = SPI_TRANS_USE_TXDATA,
                    .cmd = 0,
                    .addr = reg & 0x7F,                     /*写操作，最高位为0*/
                    .length = 8,
                    .rxlength = 0,
                    .user = nullptr,
                    .tx_data = {value},
                    .rx_buffer = nullptr
                };
                return spi_device_polling_transmit(spi, &t);
            }
            
            /*SPI读函数*/
            inline esp_err_t read_reg(uint8_t reg, uint8_t *rx_buf, size_t length) 
            {
                if (!spi_initialized || spi == NULL) return ESP_FAIL;
                if (length > 16) {
                    ESP_LOGE(TAG, "Read length too long: %d", length);
                    return ESP_FAIL;
                }

                uint8_t dummy_tx[16] = {0xFF};              /*初始化dummy数据为0xFF*/

                spi_transaction_t t = {
                    .flags = 0,
                    .cmd = 0,
                    .addr = reg | 0x80,                     /*读操作，最高位为1*/
                    .length = 8 * length,                   /*数据阶段的总位数（字节数*8）*/
                    .rxlength = 8 * length,
                    .user = nullptr,
                    .tx_buffer = dummy_tx,                  /*发送dummy数据以生成时钟*/
                    .rx_buffer = rx_buf
                };

                esp_err_t ret = spi_device_polling_transmit(spi, &t);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "SPI read error: 0x%02X", ret);
                }
                return ret;
            }
            
            /*检查传感器连接*/
            bool check_sensor_connection() 
            {
                uint8_t who_am_i;
                if (read_reg(0x0F, &who_am_i, 1) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to read WHO_AM_I register");
                    return false;
                }
                
                if (who_am_i != 0x6A) {
                    ESP_LOGE(TAG, "Unexpected WHO_AM_I value: 0x%02X (expected 0x6A)", who_am_i);
                    return false;
                }
                
                ESP_LOGI(TAG, "Sensor connection verified");
                return true;
            }
            
            /*初始化传感器配置*/
            esp_err_t config_sensor() 
            {
                esp_err_t ret;
                
                /*先进行软复位*/
                if ((ret = write_reg(0x12, 0x01)) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to perform soft reset");
                    return ret;
                }
                
                vTaskDelay(pdMS_TO_TICKS(10));  /*等待复位完成*/
                
                /*验证复位是否完成*/
                uint8_t ctrl3_c;
                if ((ret = read_reg(0x12, &ctrl3_c, 1)) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to read CTRL3_C after reset");
                    return ret;
                }
                
                if (ctrl3_c & 0x01) {
                    ESP_LOGE(TAG, "Soft reset not completed, CTRL3_C: 0x%02X", ctrl3_c);
                    return ESP_FAIL;
                }
                
                /*配置加速度计: 416Hz, ±8g*/
                if ((ret = write_reg(0x10, 0x6C)) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to configure CTRL1_XL");
                    return ret;
                }
                
                /*配置陀螺仪: 416Hz, ±2000dps*/
                if ((ret = write_reg(0x11, 0x6C)) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to configure CTRL2_G");
                    return ret;
                }
                
                /*配置控制寄存器3: BDU=1, IF_INC=1*/
                if ((ret = write_reg(0x12, 0x64)) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to configure CTRL3_C");
                    return ret;
                }
                
                // /*配置高通滤波器*/
                // if ((ret = write_reg(0x17, 0x05)) != ESP_OK) {
                //     ESP_LOGE(TAG, "Failed to configure CTRL8_XL");
                //     return ret;
                // }
                
                ESP_LOGI(TAG, "Sensor configured for normal operation");
                return ESP_OK;
            }
            
            /*配置传感器进入低功耗模式并启用唤醒功能*/
            esp_err_t config_sensor_low_power_with_wakeup() 
            {
                esp_err_t ret;

                if ((ret = write_reg(0x12, 0x01)) != ESP_OK) return ret;     /*软复位一下，清空之前的配置*/

                vTaskDelay(pdMS_TO_TICKS(10));                              /*等待复位成功*/   
    
                /*配置加速度计为低功耗模式，12.5Hz，±2g*/
                if ((ret = write_reg(0x10, 0x10)) != ESP_OK) return ret;
                
                /*禁用陀螺仪*/
                if ((ret = write_reg(0x11, 0x00)) != ESP_OK) return ret;

                /*CTRL3_C: BDU使能 + IF_INC使能；BDU = 1 (块数据更新), IF_INC = 1 (地址自动递增)，低电平有效*/
                if ((ret = write_reg(0x12, 0x64)) != ESP_OK) return ret;     

                /*配置CTRL8_XL为低功耗模式优化*/
                /*使用低通路径(LPF1)，带宽为ODR/100 (适合低功耗模式)*/
                /*LPF1_BW_SEL = 0 (低延迟模式), HPCF_XL[1:0] = 01*/
                if ((ret = write_reg(0x17, 0x01)) != ESP_OK) return ret;

                /*配置TAP_CFG寄存器 - 使能基本中断和活动检测*/
                /*INTERRUPTS_ENABLE = 1 (使能基本中断)*/
                /*INACT_EN[1:0] = 11 (设置加速度计ODR为12.5Hz低功耗模式，陀螺仪进入关闭模式)*/
                /*LIR = 1 (锁存中断请求)*/
                if ((ret = write_reg(0x58, 0x80 | 0x60 | 0x01)) != ESP_OK) return ret;  /*TAP_CFG: 使能中断和活动检测*/
                
                /*配置TAP_THS_6D寄存器 - 设置6D检测阈值*/
                /*SIXD_THS[1:0] = 10 (60度阈值，适合检测抬手动作)*/
                /*不使用点击检测功能，TAP_THS[4:0]保持默认值*/
                if ((ret = write_reg(0x59, 0x40)) != ESP_OK) return ret;     /*TAP_THS_6D: 设置6D检测阈值*/
                
                /*配置唤醒功能*/
                /*根据寄存器描述，WK_THS[5:0]设置唤醒阈值*/
                /*1 LSB = FS_XL/(2的6次方) = 4g/(2的6次方) ≈ 0.0625g*/
                /*改为较小的阈值，例如 0x0A = 10 LSB ≈ 0.625g*/
                if ((ret = write_reg(0x5B, 0x0A)) != ESP_OK) return ret;     /*WAKE_UP_THS: 设置唤醒阈值*/
                
                /*设置唤醒持续时间 (0x04 = 3个ODR周期，约240ms)**/
                /*WAKE_DUR[1:0]设置唤醒持续时间，1LSB = 1 ODR_time*/
                /*对于12.5Hz ODR，1 ODR_time = 80ms*/
                /*需要设置WAKE_DUR[1:0]位，而不是整个寄存器*/
                if ((ret = write_reg(0x5C, 0x60)) != ESP_OK) return ret;     /*WAKE_UP_DUR: 设置唤醒持续时间*/
                
                /*配置中断为锁存模式*/
                if ((ret = write_reg(0x5E, 0x20)) != ESP_OK) return ret;     /*MD1_CFG: 将唤醒中断映射到INT1*/
                if ((ret = write_reg(0x5F, 0x00)) != ESP_OK) return ret;     /*MD2_CFG: 禁用INT2上的所有功能*/
                
                /*配置中断控制寄存器*/
                /*INT1_CTRL: 禁用所有INT1中断（唤醒中断通过MD1_CFG路由，不需要在此使能）*/
                if ((ret = write_reg(0x0D, 0x00)) != ESP_OK) return ret;     /*INT1_CTRL: 禁用所有INT1中断*/
                if ((ret = write_reg(0x0E, 0x00)) != ESP_OK) return ret;     /*INT2_CTRL: 禁用所有INT2中断*/
                
                ESP_LOGI(TAG, "Sensor configured for low power with wake-up");
                return ESP_OK;
            }
            
            /*校准传感器*/
            void calibrate_sensor(int16_t ax, int16_t ay, int16_t az, int16_t gx, int16_t gy, int16_t gz) 
            {
                const uint32_t required_samples = 10; /*减少样本数量以加快校准*/
                
                if (calibration_samples < required_samples) {
                    /*直接累加原始值*/
                    ax_sum += ax;
                    ay_sum += ay;
                    az_sum += az;
                    gx_sum += gx;
                    gy_sum += gy;
                    gz_sum += gz;
                    
                    calibration_samples++;
                    
                    if (calibration_samples % 10 == 0) {
                        ESP_LOGI(TAG, "Calibrating... %d%%", (calibration_samples * 100) / required_samples);
                        /*显示当前累加值*/
                        ESP_LOGI(TAG, "Current sums: Accel(%lld, %lld, %lld), Gyro(%lld, %lld, %lld)", 
                                 ax_sum, ay_sum, az_sum, gx_sum, gy_sum, gz_sum);
                    }
                    
                    if (calibration_samples < required_samples) {
                        return;
                    }
                    
                    /*计算平均值*/
                    int32_t ax_avg = ax_sum / required_samples;
                    int32_t ay_avg = ay_sum / required_samples;
                    int32_t az_avg = az_sum / required_samples;
                    int32_t gx_avg = gx_sum / required_samples;
                    int32_t gy_avg = gy_sum / required_samples;
                    int32_t gz_avg = gz_sum / required_samples;
                    
                    /*计算偏移量*/
                    ax_offset = -ax_avg;
                    ay_offset = -ay_avg;
                    
                    /*对于Z轴，考虑重力影响*/
                    int16_t expected_z = static_cast<int16_t>(1.0f / ACCEL_SENSITIVITY);
                    az_offset = expected_z - az_avg;  /*注意这里是 expected_z - az_avg*/
                    
                    gx_offset = -gx_avg;
                    gy_offset = -gy_avg;
                    gz_offset = -gz_avg;
                    
                    calibrated = true;
                    ESP_LOGI(TAG, "Calibration completed: Accel(%d, %d, %d), Gyro(%d, %d, %d)", 
                             ax_offset, ay_offset, az_offset, gx_offset, gy_offset, gz_offset);
                    
                    ESP_LOGI(TAG, "Averages: Accel(%d, %d, %d), Gyro(%d, %d, %d)", 
                             ax_avg, ay_avg, az_avg, gx_avg, gy_avg, gz_avg);
                }
            }

            /*更新姿态估计*/
            void update_attitude(float ax, float ay, float az, float gx, float gy, float gz, float dt) 
            {
                /*使用更快的角度计算 - 避免重复计算*/
                float ax2 = ax * ax;
                float ay2 = ay * ay;
                float az2 = az * az;
                
                /*快速计算加速度计角度 - 使用近似算法*/
                float denom1 = sqrtf(ay2 + az2);
                float denom2 = sqrtf(ax2 + az2);
                
                /*避免除零*/
                if (denom1 > 0.01f && denom2 > 0.01f) {
                    pitch_acc = atan2f(-ax, denom1) * RAD_TO_DEG;
                    roll_acc = atan2f(ay, denom2) * RAD_TO_DEG;
                }
                
                /*大幅降低陀螺仪滤波强度以获得更快响应*/
                const float FAST_GYRO_FILTER_ALPHA = 0.1f;  /*从0.3降低到0.1*/
                
                gx_filtered = gx;  /*几乎不使用滤波*/
                gy_filtered = gy;
                gz_filtered = gz;
                
                /*使用动态互补滤波器系数 - 根据运动状态调整*/
                float dynamic_alpha;
                
                /*检测快速运动：如果角速度大，更依赖陀螺仪*/
                float gyro_magnitude = sqrtf(gx*gx + gy*gy + gz*gz);
                if (gyro_magnitude > 1.0f) { /*快速运动*/
                    dynamic_alpha = 0.70f;   /*更依赖陀螺仪*/
                } else { /*慢速或静止*/
                    dynamic_alpha = 0.85f;   /*更依赖加速度计以便快速校正*/
                }
                
                /*应用互补滤波器*/
                pitch = dynamic_alpha * (pitch + gy_filtered * dt) + (1.0f - dynamic_alpha) * pitch_acc;
                roll = dynamic_alpha * (roll + gx_filtered * dt) + (1.0f - dynamic_alpha) * roll_acc;
                
                /*限制角度范围避免溢出*/
                pitch = fmodf(pitch, 360.0f);
                roll = fmodf(roll, 360.0f);
            }
            
            /*将姿态转换为摇杆值*/
            void attitude_to_joystick() 
            {
                /*进一步减小角度阈值以提高灵敏度*/
                const float max_angle = 12.0f;  /*从15°减小到12°*/
                
                /*直接计算归一化值，避免多次函数调用*/
                float x_norm = -roll / max_angle;
                float y_norm = -pitch / max_angle;
                
                /*限制在[-1, 1]范围内*/
                if (x_norm > 1.0f) x_norm = 1.0f;
                else if (x_norm < -1.0f) x_norm = -1.0f;
                
                if (y_norm > 1.0f) y_norm = 1.0f;
                else if (y_norm < -1.0f) y_norm = -1.0f;
                
                /*使用更强的非线性响应 - 平方曲线*/
                x_norm = x_norm * fabsf(x_norm);  /*平方曲线，比立方曲线计算更快*/
                y_norm = y_norm * fabsf(y_norm);
                
                /*减小死区大小*/
                const float dead_zone = 0.02f;  /*从0.05减小到0.02*/
                if (fabsf(x_norm) < dead_zone) x_norm = 0.0f;
                if (fabsf(y_norm) < dead_zone) y_norm = 0.0f;
                
                /*根据佩戴手调整方向*/
                if (left_handed) {
                    x_norm = -x_norm;
                }
                
                /*快速转换为摇杆值*/
                joystick_lx = static_cast<uint16_t>((x_norm + 1.0f) * 32767.5f);
                joystick_ly = static_cast<uint16_t>((y_norm + 1.0f) * 32767.5f);
                
                /*快速范围检查*/
                if (joystick_lx > 65535) joystick_lx = 65535;
                if (joystick_ly > 65535) joystick_ly = 65535;
            }
            
            /*清除所有中断标志*/
            esp_err_t clear_interrupt_flags()
            {
                uint8_t status_reg;
                esp_err_t ret;
                
                /*读取状态寄存器会自动清除其中的中断标志*/
                if ((ret = read_reg(0x1E, &status_reg, 1)) != ESP_OK) {  /*STATUS_REG*/
                    ESP_LOGE(TAG, "Failed to read STATUS_REG");
                    return ret;
                }
                
                /*读取唤醒源寄存器清除唤醒中断*/
                if ((ret = read_reg(0x1B, &status_reg, 1)) != ESP_OK) {  /*WAKE_UP_SRC*/
                    ESP_LOGE(TAG, "Failed to read WAKE_UP_SRC");
                    return ret;
                }

                /*读取点击源寄存器清除点击中断标志（如果使能了点击检测）*/
                if ((ret = read_reg(0x1C, &status_reg, 1)) != ESP_OK) {  /*TAP_SRC*/
                    ESP_LOGE(TAG, "Failed to read TAP_SRC");
                    return ret;
                }
                
                /*读取6D源寄存器清除6D方向中断标志（如果使能了6D检测）*/
                if ((ret = read_reg(0x1D, &status_reg, 1)) != ESP_OK) {  /*D6D_SRC*/
                    ESP_LOGE(TAG, "Failed to read D6D_SRC");
                    return ret;
                }
                
                /*读取功能源寄存器清除其他功能中断标志*/
                if ((ret = read_reg(0x53, &status_reg, 1)) != ESP_OK) {  /*FUNC_SRC1*/
                    ESP_LOGE(TAG, "Failed to read FUNC_SRC1");
                    return ret;
                }

                if ((ret = read_reg(0x54, &status_reg, 1)) != ESP_OK) {  /*FUNC_SRC2*/
                    ESP_LOGE(TAG, "Failed to read FUNC_SRC2");
                    return ret;
                }
                
                ESP_LOGI(TAG, "Interrupt flags cleared");
                return ESP_OK;
            }
            
            /*任务函数*/
            static void imu_task(void *arg) {
                imu *self = static_cast<imu*>(arg);
                uint8_t data[12];
                uint32_t last_time = xTaskGetTickCount();
                
                /*预计算常量以提高效率*/
                const float accel_sensitivity = ACCEL_SENSITIVITY;
                const float gyro_sensitivity = GYRO_SENSITIVITY;
                const float deg_to_rad = DEG_TO_RAD;
                const TickType_t task_delay = pdMS_TO_TICKS(IMU_TASK_DELAY_MS);
                
                while (1) {

                    /*检查运行状态*/
                    if (!self->is_running) {
                        vTaskSuspend(NULL);                 /*挂起任务直到被唤醒*/
                        continue;
                    }
                    
                    /*读取加速度计和陀螺仪数据*/
                    if (self->read_reg(0x22, data, 12) != ESP_OK) {
                        ESP_LOGE(self->TAG, "Failed to read sensor data");
                        vTaskDelay(pdMS_TO_TICKS(10));
                        continue;
                    }
                    
                    /*解析原始数据*/
                    int16_t gx_raw = (data[1] << 8) | data[0];  // OUTX_L_G (22h) + OUTX_H_G (23h)
                    int16_t gy_raw = (data[3] << 8) | data[2];  // OUTY_L_G (24h) + OUTY_H_G (25h)
                    int16_t gz_raw = (data[5] << 8) | data[4];  // OUTZ_L_G (26h) + OUTZ_H_G (27h)
                    int16_t ax_raw = (data[7] << 8) | data[6];  // OUTX_L_XL (28h) + OUTX_H_XL (29h)
                    int16_t ay_raw = (data[9] << 8) | data[8];  // OUTY_L_XL (2Ah) + OUTY_H_XL (2Bh)
                    int16_t az_raw = (data[11] << 8) | data[10]; // OUTZ_L_XL (2Ch) + OUTZ_H_XL (2Dh)

                    // /*调试输出原始数据*/
                    // static uint32_t debug_count = 0;
                    // if (++debug_count % 50 == 0) {
                    //     ESP_LOGI(self->TAG, "Raw data: Accel(%d, %d, %d), Gyro(%d, %d, %d)", 
                    //             ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw);
                    // }
                    
                    /*应用校准偏移*/
                    int16_t ax = ax_raw + self->ax_offset;
                    int16_t ay = ay_raw + self->ay_offset;
                    int16_t az = az_raw + self->az_offset;
                    int16_t gx = gx_raw + self->gx_offset;
                    int16_t gy = gy_raw + self->gy_offset;
                    int16_t gz = gz_raw + self->gz_offset;
                    
                    /*快速转换为物理单位*/
                    float ax_g = ax * accel_sensitivity;
                    float ay_g = ay * accel_sensitivity;
                    float az_g = az * accel_sensitivity;
                    float gx_rad = (gx * gyro_sensitivity) * deg_to_rad;
                    float gy_rad = (gy * gyro_sensitivity) * deg_to_rad;
                    float gz_rad = (gz * gyro_sensitivity) * deg_to_rad;
                    
                    /*计算精确的时间增量*/
                    uint32_t current_time = xTaskGetTickCount();
                    float dt = (current_time - last_time) * portTICK_PERIOD_MS / 1000.0f;
                    last_time = current_time;
                    
                    /*校准传感器（如果尚未校准）*/
                    if (!self->calibrated) {
                        self->calibrate_sensor(ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw);
                        vTaskDelay(10); /*校准期间短暂延迟*/
                        continue;
                    }
                    
                    /*更新姿态估计*/
                    self->update_attitude(ax_g, ay_g, az_g, gx_rad, gy_rad, gz_rad, dt);
                    
                    /*转换为摇杆值*/
                    self->attitude_to_joystick();
                    
                    /*调试输出*/
                    static uint32_t count = 0;
                    if (++count % 20 == 0) { /*每20次输出一次，减少日志输出*/
                        ESP_LOGI(self->TAG, "Pitch: %.1f°, Roll: %.1f°, LX: %d, LY: %d", 
                                self->pitch, self->roll, self->joystick_lx, self->joystick_ly);
                    }
                    
                    vTaskDelay(pdMS_TO_TICKS(IMU_TASK_DELAY_MS));  
                }
            }

            /*私有构造函数，禁止外部直接实例化*/
            imu()
            {
                joystick_lx = 32768;
                joystick_ly = 32768;
                spi = NULL;
                imu_task_handle = NULL;
                is_running = false;
                spi_initialized = false;
                spi_host = SPI3_HOST;
                left_handed = true;         /*默认设置为左手佩戴*/
                
                /*初始化姿态估计相关变量*/
                pitch = 0.0f;
                roll = 0.0f;
                yaw = 0.0f;
                pitch_acc = 0.0f;
                roll_acc = 0.0f;
                ax_offset = 0;
                ay_offset = 0;
                az_offset = 0;
                gx_offset = 0;
                gy_offset = 0;
                gz_offset = 0;
                last_update_time = 0;
                calibrated = false;
                calibration_samples = 0;
                ax_sum = 0;
                ay_sum = 0;
                az_sum = 0;
                gx_sum = 0;
                gy_sum = 0;
                gz_sum = 0;
                /*初始化陀螺仪滤波变量*/
                gx_filtered = 0;
                gy_filtered = 0;
                gz_filtered = 0;
            }
            
            /*禁止拷贝构造和赋值操作*/
            imu(const imu&) = delete;
            imu& operator = (const imu&) = delete;
            
        public:
            /*获取单例实例的静态方法*/
            inline static imu& getInstance() {
                static imu instance;
                return instance;
            }

            inline esp_err_t init(void)
            {
                esp_err_t ret;
                
                /*初始化SPI总线和设备*/
                if ((ret = init_spi_bus()) != ESP_OK) {
                    return ret;
                }
                
                if ((ret = init_spi_device()) != ESP_OK) {
                    return ret;
                }
                
                /*检查传感器连接*/
                if (!check_sensor_connection()) {
                    ESP_LOGE(TAG, "Sensor connection check failed");
                    return ESP_FAIL;
                }
                
                /*创建任务处理数据（初始处于挂起状态）*/
                if (xTaskCreatePinnedToCore(imu_task, "imu_task", 4096, this, IMU_TASK_PRIOR, &imu_task_handle, IMU_TASK_CORE) != pdPASS) {
                    ESP_LOGE(TAG, "Failed to create IMU task");
                    return ESP_FAIL;
                }
                if(imu_task_handle != NULL)vTaskSuspend(imu_task_handle);  /*初始时挂起任务*/

                /*配置传感器为低功耗模式并启用唤醒功能（初始状态）*/
                if ((ret = config_sensor_low_power_with_wakeup()) != ESP_OK) {
                    return ret;
                }

                /*清除可能存在的残留中断*/
                clear_interrupt_flags();
                
                ESP_LOGI(TAG, "IMU initialization complete, wake-on-raise configured");
                return ESP_OK;
            }
            
            /*启动IMU功能*/
            inline esp_err_t start(void)
            {
                if (is_running) {
                    ESP_LOGW(TAG, "IMU is already running");
                    return ESP_OK;
                }

                /*确保SPI已初始化*/
                if (!spi_initialized) {
                    esp_err_t ret = init_spi_bus();
                    if (ret != ESP_OK) return ret;
                }

                /*确保SPI设备已初始化*/
                if (spi == NULL) {
                    esp_err_t ret = init_spi_device();
                    if (ret != ESP_OK) return ret;
                }
                
                /*配置传感器为正常工作模式*/
                esp_err_t ret = config_sensor();
                if (ret != ESP_OK) return ret;
                
                /*重置校准（可选，根据需求决定是否每次启动都重新校准）*/
                reset_calibration();
                
                /*设置运行标志*/
                is_running = true;
                
                /*恢复任务*/
                if(imu_task_handle != NULL)vTaskResume(imu_task_handle);

                /*清除可能存在的残留中断*/
                clear_interrupt_flags();
                
                ESP_LOGI(TAG, "IMU started");
                return ESP_OK;
            }
            
            /*停止IMU功能以降低功耗，并配置抬手唤醒*/
            inline esp_err_t stop(void)
            {
                if (!is_running) {
                    ESP_LOGW(TAG, "IMU is already stopped");
                    return ESP_OK;
                }
                
                /*设置运行标志*/
                is_running = false;

                /*挂起任务*/
                if(imu_task_handle != NULL)vTaskSuspend(imu_task_handle);
                
                /*配置传感器为低功耗模式并启用唤醒功能*/
                esp_err_t ret = config_sensor_low_power_with_wakeup();
                if (ret != ESP_OK) return ret;

                /*清除中断标志*/
                clear_interrupt_flags();
                
                ESP_LOGI(TAG, "IMU stopped, wake-on-raise configured");
                return ESP_OK;
            }
            
            /*完全关闭IMU以达成最低功耗*/
            inline esp_err_t power_off(void)
            {
                esp_err_t ret = ESP_OK;
                
                /*如果正在运行，先停止*/
                if (is_running) {
                    stop();
                }
                
                /*配置传感器进入完全关闭模式*/
                /*关闭加速度计和陀螺仪*/
                if ((ret = write_reg(0x10, 0x00)) != ESP_OK) {  /*CTRL1_XL: 关闭加速度计*/
                    ESP_LOGE(TAG, "Failed to disable accelerometer");
                }
                
                if ((ret = write_reg(0x11, 0x00)) != ESP_OK) {  /*CTRL2_G: 关闭陀螺仪*/
                    ESP_LOGE(TAG, "Failed to disable gyroscope");
                }
                
                /*禁用所有中断*/
                if ((ret = write_reg(0x0D, 0x00)) != ESP_OK) {  /*INT1_CTRL: 禁用所有INT1中断*/
                    ESP_LOGE(TAG, "Failed to disable INT1 interrupts");
                }
                
                if ((ret = write_reg(0x0E, 0x00)) != ESP_OK) {  /*INT2_CTRL: 禁用所有INT2中断*/
                    ESP_LOGE(TAG, "Failed to disable INT2 interrupts");
                }
                
                /*清除所有中断标志*/
                clear_interrupt_flags();
                
                /*如果SPI设备已初始化，则移除设备*/
                if (spi != NULL) {
                    spi_bus_remove_device(spi);
                    spi = NULL;
                    ESP_LOGI(TAG, "SPI device removed");
                }
                
                /*如果SPI总线已初始化，则释放总线*/
                if (spi_initialized) {
                    spi_bus_free(spi_host);
                    spi_initialized = false;
                    ESP_LOGI(TAG, "SPI bus freed");
                }
                
                /*删除任务*/
                if (imu_task_handle != NULL) {
                    vTaskDelete(imu_task_handle);
                    imu_task_handle = NULL;
                    ESP_LOGI(TAG, "IMU task deleted");
                }
                
                ESP_LOGI(TAG, "IMU completely powered off");
                return ret;
            }
            
            /*获取当前运行状态*/
            inline bool get_status(void) const { return is_running; }
            inline uint16_t get_joystick_lx(void){return joystick_lx;}
            inline uint16_t get_joystick_ly(void){return joystick_ly;}
            
            /*清除所有中断标志（公共接口）*/
            inline esp_err_t clear_all_interrupts() {return clear_interrupt_flags();}
            
            /*设置左手佩戴标志*/
            inline void set_left_handed(bool left) { left_handed = left; }
            
            /*获取左手佩戴标志*/
            inline bool get_left_handed() const { return left_handed; }
            
            /*重置校准*/
            inline void reset_calibration() 
            {
                calibrated = false;
                calibration_samples = 0;
                ax_sum = 0;
                ay_sum = 0;
                az_sum = 0;
                gx_sum = 0;
                gy_sum = 0;
                gz_sum = 0;
                ax_offset = 0;
                ay_offset = 0;
                az_offset = 0;
                gx_offset = 0;
                gy_offset = 0;
                gz_offset = 0;
                pitch = 0.0f;
                roll = 0.0f;
                yaw = 0.0f;
                /*重置陀螺仪滤波器*/
                gx_filtered = 0;
                gy_filtered = 0;
                gz_filtered = 0;
                ESP_LOGI(TAG, "Calibration reset");
            }
            
            /**
             * @brief IMU自检程序，检查水平静止状态下的数据是否正常
             * @param verbose 是否输出详细的自检信息
             * @return true 自检通过，false 自检失败
             */
            bool self_test(bool verbose = true)
            {
                bool original_state = is_running;
                
                if (!is_running) {
                    /*如果IMU未运行，临时启动它*/
                    if (start() != ESP_OK) {
                        if (verbose) ESP_LOGE(TAG, "Failed to start IMU for self-test");
                        return false;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100)); /*等待传感器稳定*/
                }

                /*读取并记录关键寄存器状态*/
                uint8_t ctrl1_xl, ctrl2_g, ctrl3_c, status_reg, who_am_i;
                if (verbose) {
                    if (read_reg(0x10, &ctrl1_xl, 1) == ESP_OK &&
                        read_reg(0x11, &ctrl2_g, 1) == ESP_OK &&
                        read_reg(0x12, &ctrl3_c, 1) == ESP_OK &&
                        read_reg(0x1E, &status_reg, 1) == ESP_OK &&
                        read_reg(0x0F, &who_am_i, 1) == ESP_OK) {
                        ESP_LOGI(TAG, "CTRL1_XL: 0x%02X, CTRL2_G: 0x%02X, CTRL3_C: 0x%02X, STATUS: 0x%02X, WHO_AM_I: 0x%02X", 
                                ctrl1_xl, ctrl2_g, ctrl3_c, status_reg, who_am_i);
                    }
                }

                const int num_samples = 100; /*采样次数*/
                int32_t ax_sum = 0, ay_sum = 0, az_sum = 0;
                int32_t gx_sum = 0, gy_sum = 0, gz_sum = 0;
                
                uint8_t data[12];
                
                /*采集多组数据求平均*/
                for (int i = 0; i < num_samples; i++) {
                    /*检查数据就绪状态*/
                    uint8_t status;
                    if (read_reg(0x1E, &status, 1) != ESP_OK) {
                        if (verbose) ESP_LOGE(TAG, "Failed to read STATUS_REG");
                        if (!original_state) stop();
                        return false;
                    }
                    
                    /*等待加速度计和陀螺仪数据就绪*/
                    if (!(status & 0x03)) { /*检查bit0(加速度计就绪)和bit1(陀螺仪就绪)*/
                        if (verbose && i == 0) ESP_LOGW(TAG, "Waiting for sensor data ready... STATUS: 0x%02X", status);
                        vTaskDelay(pdMS_TO_TICKS(5));
                        i--; /*重试这一次*/
                        continue;
                    }
                    
                    if (read_reg(0x22, data, 12) != ESP_OK) {
                        if (verbose) ESP_LOGE(TAG, "Failed to read sensor data during self-test");
                        if (!original_state) stop();
                        return false;
                    }
                    
                    /*打印前几个样本的原始字节数据用于调试*/
                    if (verbose && i < 5) {
                        ESP_LOGI(TAG, "Raw data [%d]: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", 
                                i, data[0], data[1], data[2], data[3], data[4], data[5],
                                data[6], data[7], data[8], data[9], data[10], data[11]);
                    }
                    
                    int16_t gx = (data[1] << 8) | data[0];  // OUTX_L_G (22h) + OUTX_H_G (23h)
                    int16_t gy = (data[3] << 8) | data[2];  // OUTY_L_G (24h) + OUTY_H_G (25h)
                    int16_t gz = (data[5] << 8) | data[4];  // OUTZ_L_G (26h) + OUTZ_H_G (27h)
                    int16_t ax = (data[7] << 8) | data[6];  // OUTX_L_XL (28h) + OUTX_H_XL (29h)
                    int16_t ay = (data[9] << 8) | data[8];  // OUTY_L_XL (2Ah) + OUTY_H_XL (2Bh)
                    int16_t az = (data[11] << 8) | data[10]; // OUTZ_L_XL (2Ch) + OUTZ_H_XL (2Dh)
                    
                    /*打印前几个样本的原始值用于调试*/
                    if (verbose && i < 5) {
                        ESP_LOGI(TAG, "Sample [%d]: Accel(%d, %d, %d), Gyro(%d, %d, %d)", 
                                i, ax, ay, az, gx, gy, gz);
                    }
                    
                    ax_sum += ax;
                    ay_sum += ay;
                    az_sum += az;
                    gx_sum += gx;
                    gy_sum += gy;
                    gz_sum += gz;
                    
                    vTaskDelay(pdMS_TO_TICKS(10)); /*10ms间隔*/
                }
                
                /*计算平均值*/
                int16_t ax_avg = ax_sum / num_samples;
                int16_t ay_avg = ay_sum / num_samples;
                int16_t az_avg = az_sum / num_samples;
                int16_t gx_avg = gx_sum / num_samples;
                int16_t gy_avg = gy_sum / num_samples;
                int16_t gz_avg = gz_sum / num_samples;
                
                /*转换为物理量（根据你的量程配置）*/
                /*加速度计量程为±8g，灵敏度为4096 LSB/g*/
                /*陀螺仪量程为±2000dps，灵敏度为16.4 LSB/dps*/
                float ax_g = ax_avg / 4096.0f;
                float ay_g = ay_avg / 4096.0f;
                float az_g = az_avg / 4096.0f;
                float gx_dps = gx_avg / 16.4f;
                float gy_dps = gy_avg / 16.4f;
                float gz_dps = gz_avg / 16.4f;
                
                /*计算总加速度（应该是1g左右）*/
                float total_accel = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
                
                /*检查条件（水平静止状态）*/
                bool accel_ok = true;
                bool gyro_ok = true;
                
                /*加速度计检查：总加速度应该接近1g（重力）*/
                if (verbose) {
                    ESP_LOGI(TAG, "Accel - X: %.2fg, Y: %.2fg, Z: %.2fg", ax_g, ay_g, az_g);
                    ESP_LOGI(TAG, "Total acceleration: %.2fg", total_accel);
                    ESP_LOGI(TAG, "Gyro - X: %.2fdps, Y: %.2fdps, Z: %.2fdps", gx_dps, gy_dps, gz_dps);
                }
                
                /*检查总加速度是否接近1g*/
                if (fabs(total_accel - 1.0f) > 0.2f) {
                    if (verbose) ESP_LOGE(TAG, "Total acceleration abnormal: %.2fg (expected ~1g)", total_accel);
                    accel_ok = false;
                }
                
                /*检查哪个轴有最大的加速度（应该是重力方向）*/
                float max_accel = std::max({fabs(ax_g), fabs(ay_g), fabs(az_g)});
                if (max_accel < 0.8f) {
                    if (verbose) ESP_LOGE(TAG, "No axis shows significant gravity (max: %.2fg)", max_accel);
                    accel_ok = false;
                }
                
                /*陀螺仪检查：所有轴应该接近0（静止状态）*/
                if (fabs(gx_dps) > 10.0f) { /*X轴角速度应小于10dps*/
                    if (verbose) ESP_LOGE(TAG, "X-axis gyro too high: %.2fdps", gx_dps);
                    gyro_ok = false;
                }
                
                if (fabs(gy_dps) > 10.0f) { /*Y轴角速度应小于10dps*/
                    if (verbose) ESP_LOGE(TAG, "Y-axis gyro too high: %.2fdps", gy_dps);
                    gyro_ok = false;
                }
                
                if (fabs(gz_dps) > 10.0f) { /*Z轴角速度应小于10dps*/
                    if (verbose) ESP_LOGE(TAG, "Z-axis gyro too high: %.2fdps", gz_dps);
                    gyro_ok = false;
                }
                
                /*恢复IMU原始状态*/
                if (!original_state) {
                    stop();
                }
                
                if (verbose) {
                    if (accel_ok && gyro_ok) {
                        ESP_LOGI(TAG, "Self-test PASSED");
                    } else {
                        ESP_LOGE(TAG, "Self-test FAILED");
                    }
                }
                
                return accel_ok && gyro_ok;
            }
            
            /**
             * @brief 调试函数：读取并打印所有关键寄存器
             */
            void debug_read_all_registers()
            {
                ESP_LOGI(TAG, "Reading all key registers...");
                
                uint8_t regs[] = {
                    0x0F, 0x10, 0x11, 0x12, 0x17, 0x1E, 
                    0x58, 0x59, 0x5B, 0x5C, 0x5E, 0x5F
                };
                
                const char* reg_names[] = {
                    "WHO_AM_I", "CTRL1_XL", "CTRL2_G", "CTRL3_C", "CTRL8_XL", "STATUS_REG",
                    "TAP_CFG", "TAP_THS_6D", "WAKE_UP_THS", "WAKE_UP_DUR", "MD1_CFG", "MD2_CFG"
                };
                
                for (int i = 0; i < sizeof(regs)/sizeof(regs[0]); i++) {
                    uint8_t value;
                    if (read_reg(regs[i], &value, 1) == ESP_OK) {
                        ESP_LOGI(TAG, "%s (0x%02X): 0x%02X", reg_names[i], regs[i], value);
                    } else {
                        ESP_LOGE(TAG, "Failed to read register 0x%02X", regs[i]);
                    }
                }
            }
    };

}
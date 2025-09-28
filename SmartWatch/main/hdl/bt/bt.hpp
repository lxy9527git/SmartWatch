/**
 * @file bt.hpp
 * @author 李威延
 * @brief
 * @version 0.1
 * @date 2025-08-31
 *
 * @copyright Copyright (c) 2025
 *
 */
#pragma once 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h> 
#include <freertos/event_groups.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_bt.h>
#include <host/ble_hs.h>
#include <nimble/nimble_port.h>
#include <nimble/nimble_port_freertos.h>
#include <esp_hidd.h>
#include <esp_hid_common.h>
#include <host/ble_gap.h>
#include <host/ble_hs_adv.h>
#include <nimble/ble.h>
#include <host/ble_sm.h>
#include <services/gap/ble_svc_gap.h>
#include "ble_store_config.h"

namespace hdl{

    class bt
    {
        #define BT_DEVICE_NAME                                          "SmartWatch-Gamepad"

        #define GATT_SVR_SVC_HID_UUID                                   (0x1812)

        #define BT_HID_DEVICE_APPEARANCE                                (0x03C4)


        private:
            const char* TAG = "bt";
            esp_hidd_dev_t *hid_dev;
            uint8_t report_buffer[14];
            bool connected;
            bool advertising;                           /*广播状态标志*/
            uint16_t conn_handle;                       /*存储当前连接的句柄*/
            struct ble_hs_adv_fields fields;
            struct ble_hs_adv_fields scan_rsp_fields;   /*添加扫描响应字段*/
            /*存储服务UUID*/
            ble_uuid16_t service_uuid;
            /*host task is done*/
            bool host_task_is_done;
            /*游戏手柄报告描述符*/
            const unsigned char gamepadReportMap[120] = {
                0x05, 0x01,        /*Usage Page (Generic Desktop Ctrls)*/
                0x09, 0x05,        /*Usage (Game Pad)*/
                0xA1, 0x01,        /*Collection (Application)*/
                0xA1, 0x00,        /*Collection (Physical)*/
                0x09, 0x30,        /*Usage (X)*/
                0x09, 0x31,        /*Usage (Y)*/
                0x15, 0x00,        /*Logical Minimum (0)*/
                0x26, 0xFF, 0xFF,  /*Logical Maximum (-1)*/
                0x35, 0x00,        /*Physical Minimum (0)*/
                0x46, 0xFF, 0xFF,  /*Physical Maximum (-1)*/
                0x95, 0x02,        /*Report Count (2)*/
                0x75, 0x10,        /*Report Size (16)*/
                0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
                0xC0,              /*End Collection*/
                0xA1, 0x00,        /*Collection (Physical)*/
                0x09, 0x33,        /*Usage (Rx)*/
                0x09, 0x34,        /*Usage (Ry)*/
                0x15, 0x00,        /*Logical Minimum (0)*/
                0x26, 0xFF, 0xFF,  /*Logical Maximum (-1)*/
                0x35, 0x00,        /*Physical Minimum (0)*/
                0x46, 0xFF, 0xFF,  /*Physical Maximum (-1)*/
                0x95, 0x02,        /*Report Count (2)*/
                0x75, 0x10,        /*Report Size (16)*/
                0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
                0xC0,              /*End Collection*/
                0xA1, 0x00,        /*Collection (Physical)*/
                0x09, 0x32,        /*Usage (Z)*/
                0x15, 0x00,        /*Logical Minimum (0)*/
                0x26, 0xFF, 0xFF,  /*Logical Maximum (-1)*/
                0x35, 0x00,        /*Physical Minimum (0)*/
                0x46, 0xFF, 0xFF,  /*Physical Maximum (-1)*/
                0x95, 0x01,        /*Report Count (1)*/
                0x75, 0x10,        /*Report Size (16)*/
                0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
                0xC0,              /*End Collection*/
                0x05, 0x09,        /*Usage Page (Button)*/
                0x19, 0x01,        /*Usage Minimum (0x01)*/
                0x29, 0x0A,        /*Usage Maximum (0x0A)*/
                0x95, 0x0A,        /*Report Count (10)*/
                0x75, 0x01,        /*Report Size (1)*/
                0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
                0x05, 0x01,        /*Usage Page (Generic Desktop Ctrls)*/
                0x09, 0x39,        /*Usage (Hat switch)*/
                0x15, 0x01,        /*Logical Minimum (1)*/
                0x25, 0x08,        /*Logical Maximum (8)*/
                0x35, 0x00,        /*Physical Minimum (0)*/
                0x46, 0x3B, 0x10,  /*Physical Maximum (4155)*/
                0x66, 0x0E, 0x00,  /*Unit (None)*/
                0x75, 0x04,        /*Report Size (4)*/
                0x95, 0x01,        /*Report Count (1)*/
                0x81, 0x42,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)*/
                0x75, 0x02,        /*Report Size (2)*/
                0x95, 0x01,        /*Report Count (1)*/
                0x81, 0x03,        /*Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
                0x75, 0x08,        /*Report Size (8)*/
                0x95, 0x02,        /*Report Count (2)*/
                0x81, 0x03,        /*Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
                0xC0,              /*End Collection*/
            };

            /*HID设备配置*/
            esp_hid_raw_report_map_t report_maps[1] = {{
                .data = gamepadReportMap,
                .len = sizeof(gamepadReportMap)
            }};

            esp_hid_device_config_t hid_config = {
                .vendor_id = 0x16C0,
                .product_id = 0x05DF,
                .version = 0x0110,
                .device_name = BT_DEVICE_NAME,
                .manufacturer_name = "SmartWatch",
                .serial_number = "SW-12345",
                .report_maps = report_maps,
                .report_maps_len = 1
            };

            /*事件回调处理*/
            static void event_callback(void* arg, esp_event_base_t base, int32_t id, void* data) {
                bt& instance = bt::getInstance();
                esp_hidd_event_t event = (esp_hidd_event_t)id;
                esp_hidd_event_data_t *param = (esp_hidd_event_data_t *)data;

                switch (event) {
                    case ESP_HIDD_CONNECT_EVENT:
                        ESP_LOGI(instance.TAG, "CONNECTED");
                        instance.connected = true;
                        instance.advertising = false;
                        break;
                        
                    case ESP_HIDD_DISCONNECT_EVENT:
                        ESP_LOGI(instance.TAG, "DISCONNECTED");
                        instance.connected = false;
                        instance.conn_handle = BLE_HS_CONN_HANDLE_NONE; /*重置连接句柄*/
                        instance.start_advertising(); /*重新广播*/
                        break;
                        
                    default:
                        break;
                }
            }

            inline static int nimble_hid_gap_event(struct ble_gap_event *event, void *arg)
            {
                struct ble_gap_conn_desc desc;
                int rc;
                bt& instance = bt::getInstance();

                switch (event->type) {
                case BLE_GAP_EVENT_CONNECT:
                    if (event->connect.status == 0) {
                        instance.conn_handle = event->connect.conn_handle; /*保存连接句柄*/
                        instance.advertising = false; /*连接成功后停止广播*/
                        /*获取连接参数*/
                        rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                        if (rc == 0) {
                            ESP_LOGI(instance.TAG, "Connected: interval=%.2fms, latency=%d, timeout=%dms",
                                    desc.conn_itvl * 1.25,
                                    desc.conn_latency,
                                    desc.supervision_timeout * 10);
                        }
                    }
                    return 0;
                    break;
                case BLE_GAP_EVENT_DISCONNECT:
                    ESP_LOGI(instance.TAG, "disconnect; reason=%d", event->disconnect.reason);
                    instance.conn_handle = BLE_HS_CONN_HANDLE_NONE; /*重置连接句柄*/
                    return 0;
                case BLE_GAP_EVENT_CONN_UPDATE:
                    /* 连接参数更新事件 */
                    rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
                    if (rc == 0) {
                        ESP_LOGI(instance.TAG, "Params updated: interval=%.2fms, latency=%d, timeout=%dms, status=%d",
                                desc.conn_itvl * 1.25,
                                desc.conn_latency,
                                desc.supervision_timeout * 10,
                                event->conn_update.status);
                    }else{
                        /* The central has updated the connection parameters. */
                        ESP_LOGI(instance.TAG, "connection updated; status=%d",
                                event->conn_update.status);
                    }        
                    return 0;

                case BLE_GAP_EVENT_ADV_COMPLETE:
                    ESP_LOGI(instance.TAG, "advertise complete; reason=%d",
                            event->adv_complete.reason);
                    instance.advertising = false;       /*广播完成*/
                    return 0;

                case BLE_GAP_EVENT_SUBSCRIBE:
                    ESP_LOGI(instance.TAG, "subscribe event; conn_handle=%d attr_handle=%d "
                            "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                            event->subscribe.conn_handle,
                            event->subscribe.attr_handle,
                            event->subscribe.reason,
                            event->subscribe.prev_notify,
                            event->subscribe.cur_notify,
                            event->subscribe.prev_indicate,
                            event->subscribe.cur_indicate);
                    return 0;

                case BLE_GAP_EVENT_MTU:
                    ESP_LOGI(instance.TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                            event->mtu.conn_handle,
                            event->mtu.channel_id,
                            event->mtu.value);
                    return 0;

                case BLE_GAP_EVENT_ENC_CHANGE:
                    /* Encryption has been enabled or disabled for this connection. */
                    ESP_LOGI(instance.TAG, "encryption change event; status=%d ",
                            event->enc_change.status);
                    rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
                    assert(rc == 0);
                    return 0;

                case BLE_GAP_EVENT_NOTIFY_TX:
                    // ESP_LOGI(instance.TAG, "notify_tx event; conn_handle=%d attr_handle=%d "
                    //         "status=%d is_indication=%d",
                    //         event->notify_tx.conn_handle,
                    //         event->notify_tx.attr_handle,
                    //         event->notify_tx.status,
                    //         event->notify_tx.indication);
                    return 0;

                case BLE_GAP_EVENT_REPEAT_PAIRING:
                    /* We already have a bond with the peer, but it is attempting to
                    * establish a new secure link.  This app sacrifices security for
                    * convenience: just throw away the old bond and accept the new link.
                    */

                    /* Delete the old bond. */
                    rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
                    assert(rc == 0);
                    ble_store_util_delete_peer(&desc.peer_id_addr);

                    /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
                    * continue with the pairing operation.
                    */
                    return BLE_GAP_REPEAT_PAIRING_RETRY;

                case BLE_GAP_EVENT_PASSKEY_ACTION:
                    ESP_LOGI(instance.TAG, "PASSKEY_ACTION_EVENT started");
                    struct ble_sm_io pkey = {0};
                    int key = 0;

                    if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
                        pkey.action = event->passkey.params.action;
                        pkey.passkey = 123456; // This is the passkey to be entered on peer
                        ESP_LOGI(instance.TAG, "Enter passkey %" PRIu32 "on the peer side", pkey.passkey);
                        rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                        ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
                    } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
                        ESP_LOGI(instance.TAG, "Accepting passkey..");
                        pkey.action = event->passkey.params.action;
                        pkey.numcmp_accept = key;
                        rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                        ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
                    } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
                        static uint8_t tem_oob[16] = {0};
                        pkey.action = event->passkey.params.action;
                        for (int i = 0; i < 16; i++) {
                            pkey.oob[i] = tem_oob[i];
                        }
                        rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                        ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
                    } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
                        ESP_LOGI(instance.TAG, "Input not supported passing -> 123456");
                        pkey.action = event->passkey.params.action;
                        pkey.passkey = 123456;
                        rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
                        ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
                    }
                    return 0;    
                }

                return 0;
            }

            inline esp_err_t esp_hid_ble_gap_adv_init(const char *device_name)
            {
                /**
                 *  Set the advertisement data included in our advertisements:
                 *     o Flags (indicates advertisement type and other general info).
                 *     o Advertising tx power.
                 *     o Device name.
                 *     o 16-bit service UUIDs (HID).
                 */

                memset(&fields, 0, sizeof(fields));
                memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));

                /* Advertise two flags:
                *     o Discoverability in forthcoming advertisement (general)
                *     o BLE-only (BR/EDR unsupported).
                */
                fields.flags = BLE_HS_ADV_F_DISC_GEN |
                            BLE_HS_ADV_F_BREDR_UNSUP;

                /* Indicate that the TX power level field should be included; have the
                * stack fill this value automatically.  This is done by assigning the
                * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
                */
                fields.tx_pwr_lvl_is_present = 1;
                fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

                /* 服务UUID */
                service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_HID_UUID);
                fields.uuids16 = &service_uuid;
                fields.num_uuids16 = 1;
                fields.uuids16_is_complete = 1;

                /* 设备外观 */
                fields.appearance = BT_HID_DEVICE_APPEARANCE;
                fields.appearance_is_present = 1;

                /* 扫描响应中包含设备名称 */
                scan_rsp_fields.name = (uint8_t *)device_name;
                scan_rsp_fields.name_len = strlen(device_name);
                scan_rsp_fields.name_is_complete = 1;

                /* 安全配置 */
                ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;
                ble_hs_cfg.sm_bonding = 1;      
                ble_hs_cfg.sm_mitm = 1;         
                ble_hs_cfg.sm_sc = 1;           
                ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ID | BLE_SM_PAIR_KEY_DIST_ENC;      
                ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID | BLE_SM_PAIR_KEY_DIST_ENC;    

                /*设置GAP设备名称*/
                ble_svc_gap_device_name_set(device_name);

                return ESP_OK;
            }

            inline static void ble_hid_device_host_task(void *param)
            {
                bt& instance = bt::getInstance();
                /* This function will return only when nimble_port_stop() is executed */
                nimble_port_run();
                instance.host_task_is_done = true;
                nimble_port_freertos_deinit();
            }

            inline esp_err_t start_advertising(void)
            {
                int rc;
                struct ble_gap_adv_params adv_params;
                /* maximum possible duration for hid device(180s) */
                int32_t adv_duration_ms = 180000;

                /*如果已经在广播，先停止*/
                if (advertising) {
                    rc = ble_gap_adv_stop();
                    if (rc != 0 && rc != BLE_HS_EALREADY) {
                        ESP_LOGE(TAG, "ble_gap_adv_stop failed: %d", rc);
                    }
                    advertising = false;
                    vTaskDelay(pdMS_TO_TICKS(20)); /*等待停止完成*/
                }

                /*设置广播字段*/
                rc = ble_gap_adv_set_fields(&fields);
                if (rc != 0) {
                    ESP_LOGE(TAG, "ble_gap_adv_set_fields failed: %d", rc);
                    return rc;
                }

                /* 设置扫描响应数据 */
                rc = ble_gap_adv_rsp_set_fields(&scan_rsp_fields);
                if (rc != 0) {
                    ESP_LOGE(TAG, "ble_gap_adv_rsp_set_fields failed: %d", rc);
                    return rc;
                }

                /* Begin advertising. */
                memset(&adv_params, 0, sizeof(adv_params));
                adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
                adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
                adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(30);/* Recommended interval 30ms to 50ms */
                adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(50);
                rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, adv_duration_ms,
                                    &adv_params, nimble_hid_gap_event, NULL);
                if (rc != 0) {
                    ESP_LOGE(TAG, "error enabling advertisement; rc=%d\n", rc);
                    return rc;
                }

                advertising = true; /*更新广播状态*/
                return rc;
            }

            /*私有构造函数，禁止外部直接实例化*/
            bt(){
                hid_dev = NULL;
                memset(report_buffer, 0, sizeof(report_buffer));
                connected = false;
                advertising = false;
                conn_handle = BLE_HS_CONN_HANDLE_NONE;
                memset(&fields, 0, sizeof(fields));
                memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));
                service_uuid = BLE_UUID16_INIT(0);
            }
            /*禁止拷贝构造和赋值操作*/
            bt(const bt&) = delete;
            bt& operator = (const bt&) = delete;
        public:
            /*获取单例实例的静态方法*/
            inline static bt& getInstance() {
                static bt instance;
                return instance;
            }

            inline esp_err_t init(const char* device_name = BT_DEVICE_NAME)
            {
                esp_err_t ret;

                ESP_LOGI(TAG, "Initializing nimble_port");
                ret = nimble_port_init();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "nimble_port_init failed: %d", ret);
                    return ret;
                }
                
                /*创建HID设备*/
                ESP_LOGI(TAG, "Creating HID device");
                ret = esp_hidd_dev_init(&hid_config, ESP_HID_TRANSPORT_BLE, event_callback, &hid_dev);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "HID device creation failed: %d,%p", ret,hid_dev);
                    return ret;
                }

                /* XXX Need to have template for store */
                ble_store_config_init();     
                ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

                ESP_LOGI(TAG, "Initializing nimble_port_freertos");
                nimble_port_freertos_init(ble_hid_device_host_task);
                
                /*初始化BLE广播*/
                ESP_LOGI(TAG, "Initializing BLE advertising");
                ret = esp_hid_ble_gap_adv_init(device_name);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "BLE advertising initialization failed: %d", ret);
                    return ret;
                }

                /*开始广播*/
                ESP_LOGI(TAG, "Starting advertising");
                ret = start_advertising();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Advertising start failed: %d", ret);
                    return ret;
                }
                
                return ESP_OK;
            }

            inline esp_err_t deinit()
            {
                esp_err_t ret = ESP_OK;

                /* 先停止事件处理 */
                host_task_is_done = false;
                nimble_port_stop();
                
                /* 等待主机任务停止 */
                int timeout = 50;
                while(!host_task_is_done && timeout-- > 0) {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
                if (!host_task_is_done) {
                    ESP_LOGW(TAG, "Host task didn't finish properly, forcing cleanup");
                }

                /* 停止广播 */
                if (advertising) {
                    int rc = ble_gap_adv_stop();
                    if (rc != 0 && rc != BLE_HS_EALREADY) {
                        ESP_LOGE(TAG, "ble_gap_adv_stop failed: %d", rc);
                        ret = ESP_FAIL;
                    }
                    advertising = false;
                }
                
                /* 断开连接 */
                if (connected) {
                    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                        // 使用更温和的方式断开连接
                        int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
                        if (rc != 0 && rc != BLE_HS_EALREADY) {
                            ESP_LOGE(TAG, "Failed to terminate connection: %d", rc);
                            ret = ESP_FAIL;
                        } else {
                            ESP_LOGI(TAG, "Connection termination initiated");
                        }
                    }
                    connected = false;
                    vTaskDelay(pdMS_TO_TICKS(100)); // 等待断开完成
                }

                /* 反初始化HID设备 */
                if (hid_dev != NULL) {
                    /*先重置服务索引*/
                    esp_hidd_reset_service_index();
                    
                    /*延迟执行GATT重置，确保所有事件处理完成*/
                    vTaskDelay(pdMS_TO_TICKS(50));
                    
                    int rc = ble_gatts_reset();
                    if(rc != 0) {
                        ESP_LOGE(TAG, "ble_gatts_reset: %d", rc);
                    }
                    
                    esp_hidd_dev_deinit(hid_dev);
                    hid_dev = NULL;
                }

                /* 最后执行NimBLE端口反初始化 */
                nimble_port_deinit();

                /* 重置报告缓冲区 */
                memset(report_buffer, 0, sizeof(report_buffer));
                
                /* 重置连接状态 */
                conn_handle = BLE_HS_CONN_HANDLE_NONE;
                
                ESP_LOGI(TAG, "Bluetooth deinitialized successfully");
                return ret;
            }

            inline esp_err_t stop()
            {
                esp_err_t ret = ESP_OK;

                /*停止广播*/
                if (advertising) {
                    int rc = ble_gap_adv_stop();
                    if (rc != 0 && rc != BLE_HS_EALREADY) {
                        ESP_LOGE(TAG, "ble_gap_adv_stop failed: %d", rc);
                        ret = ESP_FAIL;
                    }
                    advertising = false;
                }
                
                /*断开连接*/
                if (connected) {
                    if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                        int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
                        if (rc != 0) {
                            ESP_LOGE(TAG, "Failed to terminate connection: %d", rc);
                            ret = ESP_FAIL;
                        } else {
                            ESP_LOGI(TAG, "Connection terminated");
                        }
                    }
                    connected = false;
                }

                /*重置报告缓冲区*/
                memset(report_buffer, 0, sizeof(report_buffer));

                ESP_LOGI(TAG, "Bluetooth Stop");

                return ret;
            }

            inline esp_err_t start()
            {
                esp_err_t ret = ESP_OK;

                /*开始广播*/
                ESP_LOGI(TAG, "Starting advertising");
                ret = start_advertising();
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Advertising start failed: %d", ret);
                    return ret;
                }

                ESP_LOGI(TAG, "Bluetooth Start");

                return ret;
            }

             /*设置按钮状态 (0-9)*/
            inline void set_button(uint8_t button_index, bool state)
            {
                if (button_index >= 10) return; /* 只支持10个按钮 (0-9) */
                
                /* 按钮数据存储在字节10和11 */
                if (button_index < 8) {
                    /* 按钮0-7在字节10 */
                    uint8_t bit_index = button_index;
                    if (state) {
                        report_buffer[10] |= (1 << bit_index);
                    } else {
                        report_buffer[10] &= ~(1 << bit_index);
                    }
                } else {
                    /* 按钮8-9在字节11的第0位和第1位 */
                    uint8_t bit_index = button_index - 8;
                    if (state) {
                        report_buffer[11] |= (1 << bit_index);
                    } else {
                        report_buffer[11] &= ~(1 << bit_index);
                    }
                }
            }

            /*设置帽式开关 (0-8, 0=中心/空状态)*/
            inline void set_hat_switch(uint8_t value)
            {
                if (value > 8) return; /* 有效值0-8 */
                
                /* 帽式开关存储在字节11的第2-5位 (高4位) */
                report_buffer[11] &= 0xC3; /* 清除原有值 (保留按钮8-9和常量位) */
                report_buffer[11] |= ((value & 0x0F) << 2); /* 设置新值 */
            }

            /*设置摇杆值 (0-65535)*/
            inline void set_joystick(uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry)
            {
                /*
                              90° (上)
                            (0,65535) ★
                                |
                            180° --●-- 0° (右) ● = 中心(32768,32768)
                            (0,0) |     (65535,32768)
                                |
                            270° (下)
                            (32768,0)
                */
               /* X/Y 在字节 0-3 */
               report_buffer[0] = lx & 0xFF;
               report_buffer[1] = (lx >> 8) & 0xFF;
               report_buffer[2] = ly & 0xFF;
               report_buffer[3] = (ly >> 8) & 0xFF;
               
               /* Rx/Ry 在字节 4-7 */
               report_buffer[4] = rx & 0xFF;
               report_buffer[5] = (rx >> 8) & 0xFF;
               report_buffer[6] = ry & 0xFF;
               report_buffer[7] = (ry >> 8) & 0xFF;
            }

            /*设置Z轴值 (0-65535)*/
            inline void set_z_axis(uint16_t z)
            {
               /* Z轴在字节 8-9 */
               report_buffer[8] = z & 0xFF;
               report_buffer[9] = (z >> 8) & 0xFF;
            }

            inline bool is_connected()
            {
                return connected;
            }

            inline void send_report()
            {
                if (connected && hid_dev) {
                    /*单次发送完整报告*/
                    esp_hidd_dev_input_set(hid_dev, 
                                        0,     /*map_index*/
                                        0,     /*report_id (统一使用ID=0)*/
                                        report_buffer, 
                                        sizeof(report_buffer));  
                }
            }

            inline void test_gamepad()
            {
                if (!is_connected()) {
                    ESP_LOGI(TAG, "Waiting for connection...");
                    start_advertising();  /*确保广播已启动*/
                    
                    /*等待连接建立（最多15秒）*/
                    int timeout = 150; /*150 * 100ms = 15秒*/
                    while (!is_connected() && timeout-- > 0) {
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                    
                    if (!is_connected()) {
                        ESP_LOGE(TAG, "Connection failed! Test aborted.");
                        return;
                    }
                }

                ESP_LOGI(TAG, "Starting gamepad test...");

                /*1. 测试所有按钮*/
                ESP_LOGI(TAG, "Testing buttons 0-9...");
                for (int i = 0; i < 10; i++) {
                    set_button(i, true);
                    send_report();
                    vTaskDelay(pdMS_TO_TICKS(200));
                    set_button(i, false);
                    send_report();
                    vTaskDelay(pdMS_TO_TICKS(50));
                }

                /*2. 测试帽式开关*/
                ESP_LOGI(TAG, "Testing hat switch...");
                for (int i = 0; i <= 8; i++) {
                    set_hat_switch(i);
                    send_report();
                    vTaskDelay(pdMS_TO_TICKS(300));
                }

                /*3. 测试摇杆和Z轴*/
                ESP_LOGI(TAG, "Testing joysticks and Z-axis...");

                /*摇杆中心位置*/
                const uint16_t center = 32768;
                
                /*摇杆极限位置测试*/
                set_joystick(0, 0, 0, 0); /*左下*/
                set_z_axis(0);
                send_report();
                vTaskDelay(pdMS_TO_TICKS(500));
                
                set_joystick(65535, 65535, 65535, 65535); /*右上*/
                set_z_axis(65535);
                send_report();
                vTaskDelay(pdMS_TO_TICKS(500));

                /*摇杆圆周运动*/
                for (int angle = 0; angle < 360; angle += 15) {
                    float rad = angle * M_PI / 180.0f;
                    uint16_t x = center + (uint16_t)(center * cosf(rad));
                    uint16_t y = center + (uint16_t)(center * sinf(rad));
                    
                    set_joystick(x, y, 65535 - x, 65535 - y);
                    set_z_axis(angle * 182); /*182 = 65535/360*/
                    send_report();
                    vTaskDelay(pdMS_TO_TICKS(50));
                }

                /*4. 组合测试*/
                ESP_LOGI(TAG, "Testing combined inputs...");
                
                /*组合1: 方向键+按钮*/
                set_hat_switch(1);      /*上*/
                set_button(0, true);    /*A键*/
                set_button(1, true);    /*B键*/
                send_report();
                vTaskDelay(pdMS_TO_TICKS(1000));
                
                /*组合2: 摇杆+按钮*/
                set_hat_switch(0);      /*释放方向键*/
                set_joystick(center, 0, 65535, center);
                set_button(3, true);    /*X键*/
                set_button(7, true);    /*RB键*/
                send_report();
                vTaskDelay(pdMS_TO_TICKS(1000));

                /*重置所有输入*/
                set_joystick(center, center, center, center);
                set_z_axis(center);
                set_hat_switch(0);
                for (int i = 0; i < 10; i++) set_button(i, false);
                send_report();
                
                ESP_LOGI(TAG, "Gamepad test completed!");

            }

    };

}



/*--------------------------------------------废弃方案--------------------------------------------*/
// #pragma once 
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <inttypes.h>
// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h> 
// #include <freertos/event_groups.h>
// #include <esp_system.h>
// #include <esp_event.h>
// #include <esp_log.h>
// #include <nvs_flash.h>
// #include <esp_bt.h>
// #include <host/ble_hs.h>
// #include <nimble/nimble_port.h>
// #include <nimble/nimble_port_freertos.h>
// #include <esp_hidd.h>
// #include <esp_hid_common.h>
// #include <host/ble_gap.h>
// #include <host/ble_hs_adv.h>
// #include <nimble/ble.h>
// #include <host/ble_sm.h>
// #include <services/gap/ble_svc_gap.h>
// #include <cmath>
// #include "ble_store_config.h"

// namespace hdl{

//     class bt
//     {
//         #define BT_DEVICE_NAME                                          "SmartWatch-Gamepad"

//         #define GATT_SVR_SVC_HID_UUID                                   (0x1812)
//         #define GATT_SVR_SVC_GAP_UUID                                   (0x1800)
//         #define GATT_SVR_SVC_GATT_UUID                                  (0x1801)
//         #define GATT_SVR_SVC_SCAN_PARAMS_UUID                           (0x1813)
//         #define GATT_SVR_SVC_BATTERY_UUID                               (0x180F)
//         #define GATT_SVR_SVC_DEVICE_INFO_UUID                           (0x180A)

//         #define BT_HID_DEVICE_APPEARANCE                                (0x03C4)

//         /*HID Service UUIDs*/
//         #define GATT_SVR_CHR_HID_INFO_UUID                              (0x2A4A)
//         #define GATT_SVR_CHR_HID_REPORT_MAP_UUID                        (0x2A4B)
//         #define GATT_SVR_CHR_HID_REPORT_UUID                            (0x2A4D)
//         #define GATT_SVR_CHR_HID_CTRL_PT_UUID                           (0x2A4C)
//         #define GATT_SVR_CHR_HID_PROTOCOL_MODE_UUID                     (0x2A4E)

//         /*GAP Service UUIDs*/
//         #define GATT_SVR_CHR_DEVICE_NAME_UUID                           (0x2A00)
//         #define GATT_SVR_CHR_APPEARANCE_UUID                            (0x2A01)

//         /*GATT Service UUIDs*/
//         #define GATT_SVR_CHR_SERVICE_CHANGED_UUID                       (0x2A05)
//         #define GATT_SVR_CHR_FEATURE_UUID                               (0x2B3A)
//         #define GATT_SVR_CHR_CLIENT_FEATURE_UUID                        (0x2B29)

//         /*Scan Parameters Service UUIDs*/
//         #define GATT_SVR_CHR_SCAN_REFRESH_UUID                          (0x2A4F)
//         #define GATT_SVR_CHR_SCAN_INTERVAL_WINDOW_UUID                  (0x2A31)

//         /*Battery Service UUIDs*/
//         #define GATT_SVR_CHR_BATTERY_LEVEL_UUID                         (0x2A19)

//         /*Device Information Service UUIDs*/
//         #define GATT_SVR_CHR_MODEL_NUMBER_UUID                          (0x2A24)
//         #define GATT_SVR_CHR_FIRMWARE_REVISION_UUID                     (0x2A2A)
//         #define GATT_SVR_CHR_MANUFACTURER_NAME_UUID                     (0x2BFF)

//         #define BLE_GATT_DSC_EXT_PROP_UUID16                            (0x2900)        /*扩展属性描述符*/
//         #define BLE_GATT_DSC_REPORT_REF_UUID16                          (0x2908)        /*报告引用描述符*/

//         private:
//             const char* TAG = "bt";
//             uint8_t report_buffer[14];
//             bool connected;
//             bool advertising;                           /*广播状态标志*/
//             uint16_t conn_handle;                       /*存储当前连接的句柄*/
//             struct ble_hs_adv_fields fields;
//             struct ble_hs_adv_fields scan_rsp_fields;   /*添加扫描响应字段*/
//             /*存储服务UUID*/
//             ble_uuid16_t service_uuid;
//             /*host task is done*/
//             bool host_task_is_done;
            
//             /*新服务的属性句柄*/
//             uint16_t gap_service_handle;
//             uint16_t device_name_handle;
//             uint16_t appearance_handle;
            
//             uint16_t gatt_service_handle;
//             uint16_t service_changed_handle;
//             uint16_t feature_handle;
//             uint16_t client_feature_handle;
//             uint16_t service_changed_ccc_handle;
            
//             uint16_t scan_params_service_handle;
//             uint16_t scan_refresh_handle;
//             uint16_t scan_interval_window_handle;
//             uint16_t scan_interval_window_ccc_handle;
            
//             uint16_t battery_service_handle;
//             uint16_t battery_level_handle;
            
//             uint16_t device_info_service_handle;
//             uint16_t model_number_handle;
//             uint16_t firmware_revision_handle;
//             uint16_t manufacturer_name_handle;
            
//             /*HID服务相关的属性句柄*/
//             uint16_t hid_service_handle;
//             uint16_t hid_info_handle;
//             uint16_t hid_report_map_handle;
//             uint16_t hid_report_handle;
//             uint16_t hid_ctrl_pt_handle;
//             uint16_t hid_protocol_mode_handle;
//             /*HID报告特征值的CCC描述符句柄*/
//             uint16_t hid_report_ccc_handle;
//             /*HID报告映射描述符句柄*/
//             uint16_t hid_report_map_ext_desc_handle;
//             /*HID报告描述符句柄*/
//             uint16_t hid_report_ref_desc_handle;
            
//             /*当前协议模式 (0=Boot Protocol, 1=Report Protocol)*/
//             uint8_t protocol_mode;
//             /*添加CCC值存储*/
//             uint16_t ccc_value; 
            
//             /*设备信息特征值*/
//             const char *model_number = "Model1";
//             const char *firmware_revision = "1.0.0";
//             const char *manufacturer_name = "Manufacturer";
            
//             /*电池电量*/
//             uint8_t battery_level = 100;
            
//             /*扫描间隔窗口*/
//             uint16_t scan_interval = 0x0010; // 16 * 0.625ms = 10ms
//             uint16_t scan_window = 0x0010;   // 16 * 0.625ms = 10ms
            
//             /*游戏手柄报告描述符 - 添加了报告ID (0x01) */
//             const unsigned char gamepadReportMap[122] = {
//                 0x05, 0x01,        /*Usage Page (Generic Desktop Ctrls)*/
//                 0x09, 0x05,        /*Usage (Game Pad)*/
//                 0xA1, 0x01,        /*Collection (Application)*/
//                 0x85, 0x01,        /*Report ID (1)*/  // 添加报告ID
//                 0xA1, 0x00,        /*Collection (Physical)*/
//                 0x09, 0x30,        /*Usage (X)*/
//                 0x09, 0x31,        /*Usage (Y)*/
//                 0x15, 0x00,        /*Logical Minimum (0)*/
//                 0x26, 0xFF, 0xFF,  /*Logical Maximum (-1)*/
//                 0x35, 0x00,        /*Physical Minimum (0)*/
//                 0x46, 0xFF, 0xFF,  /*Physical Maximum (-1)*/
//                 0x95, 0x02,        /*Report Count (2)*/
//                 0x75, 0x10,        /*Report Size (16)*/
//                 0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
//                 0xC0,              /*End Collection*/
//                 0xA1, 0x00,        /*Collection (Physical)*/
//                 0x09, 0x33,        /*Usage (Rx)*/
//                 0x09, 0x34,        /*Usage (Ry)*/
//                 0x15, 0x00,        /*Logical Minimum (0)*/
//                 0x26, 0xFF, 0xFF,  /*Logical Maximum (-1)*/
//                 0x35, 0x00,        /*Physical Minimum (0)*/
//                 0x46, 0xFF, 0xFF,  /*Physical Maximum (-1)*/
//                 0x95, 0x02,        /*Report Count (2)*/
//                 0x75, 0x10,        /*Report Size (16)*/
//                 0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
//                 0xC0,              /*End Collection*/
//                 0xA1, 0x00,        /*Collection (Physical)*/
//                 0x09, 0x32,        /*Usage (Z)*/
//                 0x15, 0x00,        /*Logical Minimum (0)*/
//                 0x26, 0xFF, 0xFF,  /*Logical Maximum (-1)*/
//                 0x35, 0x00,        /*Physical Minimum (0)*/
//                 0x46, 0xFF, 0xFF,  /*Physical Maximum (-1)*/
//                 0x95, 0x01,        /*Report Count (1)*/
//                 0x75, 0x10,        /*Report Size (16)*/
//                 0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
//                 0xC0,              /*End Collection*/
//                 0x05, 0x09,        /*Usage Page (Button)*/
//                 0x19, 0x01,        /*Usage Minimum (0x01)*/
//                 0x29, 0x0A,        /*Usage Maximum (0x0A)*/
//                 0x95, 0x0A,        /*Report Count (10)*/
//                 0x75, 0x01,        /*Report Size (1)*/
//                 0x81, 0x02,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
//                 0x05, 0x01,        /*Usage Page (Generic Desktop Ctrls)*/
//                 0x09, 0x39,        /*Usage (Hat switch)*/
//                 0x15, 0x01,        /*Logical Minimum (1)*/
//                 0x25, 0x08,        /*Logical Maximum (8)*/
//                 0x35, 0x00,        /*Physical Minimum (0)*/
//                 0x46, 0x3B, 0x10,  /*Physical Maximum (4155)*/
//                 0x66, 0x0E, 0x00,  /*Unit (None)*/
//                 0x75, 0x04,        /*Report Size (4)*/
//                 0x95, 0x01,        /*Report Count (1)*/
//                 0x81, 0x42,        /*Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)*/
//                 0x75, 0x02,        /*Report Size (2)*/
//                 0x95, 0x01,        /*Report Count (1)*/
//                 0x81, 0x03,        /*Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
//                 0x75, 0x08,        /*Report Size (8)*/
//                 0x95, 0x02,        /*Report Count (2)*/
//                 0x81, 0x03,        /*Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)*/
//                 0xC0,              /*End Collection*/
//             };

//             /*HID信息特征值 (版本号、国家码、标志)*/
//             const uint8_t hid_info[4] = {
//                 0x01, 0x10,  /*bcdHID (版本号 1.10)*/
//                 0x00,        /*bCountryCode (国家码 0)*/
//                 0x02         /*Flags (远程唤醒 | 通常连接)*/
//             };

//             /*静态成员变量声明*/
//             ble_uuid16_t hid_service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_HID_UUID);
//             ble_uuid16_t hid_info_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_HID_INFO_UUID);
//             ble_uuid16_t hid_report_map_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_HID_REPORT_MAP_UUID);
//             ble_uuid16_t hid_report_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_HID_REPORT_UUID);
//             ble_uuid16_t hid_ctrl_pt_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_HID_CTRL_PT_UUID);
//             ble_uuid16_t hid_protocol_mode_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_HID_PROTOCOL_MODE_UUID);
//             ble_uuid16_t cccd_uuid = BLE_UUID16_INIT(BLE_GATT_DSC_CLT_CFG_UUID16);
//             ble_uuid16_t ext_desc_uuid = BLE_UUID16_INIT(BLE_GATT_DSC_EXT_PROP_UUID16);
//             ble_uuid16_t report_ref_desc_uuid = BLE_UUID16_INIT(BLE_GATT_DSC_REPORT_REF_UUID16);
            
//             /*新服务的UUID*/
//             ble_uuid16_t gap_service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_GAP_UUID);
//             ble_uuid16_t device_name_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_DEVICE_NAME_UUID);
//             ble_uuid16_t appearance_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_APPEARANCE_UUID);
            
//             ble_uuid16_t gatt_service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_GATT_UUID);
//             ble_uuid16_t service_changed_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_SERVICE_CHANGED_UUID);
//             ble_uuid16_t feature_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_FEATURE_UUID);
//             ble_uuid16_t client_feature_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_CLIENT_FEATURE_UUID);
            
//             ble_uuid16_t scan_params_service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_SCAN_PARAMS_UUID);
//             ble_uuid16_t scan_refresh_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_SCAN_REFRESH_UUID);
//             ble_uuid16_t scan_interval_window_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_SCAN_INTERVAL_WINDOW_UUID);
            
//             ble_uuid16_t battery_service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_BATTERY_UUID);
//             ble_uuid16_t battery_level_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_BATTERY_LEVEL_UUID);
            
//             ble_uuid16_t device_info_service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_DEVICE_INFO_UUID);
//             ble_uuid16_t model_number_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_MODEL_NUMBER_UUID);
//             ble_uuid16_t firmware_revision_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_FIRMWARE_REVISION_UUID);
//             ble_uuid16_t manufacturer_name_chr_uuid = BLE_UUID16_INIT(GATT_SVR_CHR_MANUFACTURER_NAME_UUID);

//             /*事件回调处理*/
//             inline static int gap_event_callback(struct ble_gap_event *event, void *arg)
//             {
//                 struct ble_gap_conn_desc desc;
//                 int rc;
//                 bt& instance = bt::getInstance();

//                 switch (event->type) {
//                     case BLE_GAP_EVENT_CONNECT:
//                         if (event->connect.status == 0) {
//                             instance.conn_handle = event->connect.conn_handle; /*保存连接句柄*/
//                             instance.connected = true;
//                             instance.advertising = false; /*连接成功后停止广播*/
//                             /*获取连接参数*/
//                             rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
//                             if (rc == 0) {
//                                 ESP_LOGI(instance.TAG, "Connected: interval=%.2fms, latency=%d, timeout=%dms",
//                                         desc.conn_itvl * 1.25,
//                                         desc.conn_latency,
//                                         desc.supervision_timeout * 10);
//                             }
//                         }
//                     return 0;

//                     case BLE_GAP_EVENT_DISCONNECT:
//                         ESP_LOGI(instance.TAG, "disconnect; reason=%d", event->disconnect.reason);
//                         instance.connected = false;
//                         instance.conn_handle = BLE_HS_CONN_HANDLE_NONE; /*重置连接句柄*/
//                         instance.start_advertising(); /*重新广播*/
//                     return 0;

//                     case BLE_GAP_EVENT_CONN_UPDATE:
//                         /* 连接参数更新事件 */
//                         rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
//                         if (rc == 0) {
//                             ESP_LOGI(instance.TAG, "Params updated: interval=%.2fms, latency=%d, timeout=%dms, status=%d",
//                                     desc.conn_itvl * 1.25,
//                                     desc.conn_latency,
//                                     desc.supervision_timeout * 10,
//                                     event->conn_update.status);
//                         }else{
//                             /* The central has updated the connection parameters. */
//                             ESP_LOGI(instance.TAG, "connection updated; status=%d",
//                                     event->conn_update.status);
//                         }        
//                     return 0;

//                     case BLE_GAP_EVENT_ADV_COMPLETE:
//                         ESP_LOGI(instance.TAG, "advertise complete; reason=%d",
//                                 event->adv_complete.reason);
//                         instance.advertising = false;       /*广播完成*/
//                         if (!instance.connected) {
//                             instance.start_advertising();   /*如果没有连接，重新广播*/
//                         }
//                     return 0;

//                     case BLE_GAP_EVENT_SUBSCRIBE:
//                         ESP_LOGI(instance.TAG, "subscribe event; conn_handle=%d attr_handle=%d "
//                                 "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
//                                 event->subscribe.conn_handle,
//                                 event->subscribe.attr_handle,
//                                 event->subscribe.reason,
//                                 event->subscribe.prev_notify,
//                                 event->subscribe.cur_notify,
//                                 event->subscribe.prev_indicate,
//                                 event->subscribe.cur_indicate);
//                     return 0;

//                     case BLE_GAP_EVENT_MTU:
//                         ESP_LOGI(instance.TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
//                                 event->mtu.conn_handle,
//                                 event->mtu.channel_id,
//                                 event->mtu.value);
//                     return 0;

//                     case BLE_GAP_EVENT_ENC_CHANGE:
//                         /* Encryption has been enabled or disabled for this connection. */
//                         ESP_LOGI(instance.TAG, "encryption change event; status=%d ",
//                                 event->enc_change.status);
//                         rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
//                         assert(rc == 0);
//                     return 0;

//                     case BLE_GAP_EVENT_NOTIFY_TX:
//                         ESP_LOGI(instance.TAG, "notify_tx event; conn_handle=%d attr_handle=%d "
//                                 "status=%d is_indication=%d",
//                                 event->notify_tx.conn_handle,
//                                 event->notify_tx.attr_handle,
//                                 event->notify_tx.status,
//                                 event->notify_tx.indication);
//                     return 0;

//                     case BLE_GAP_EVENT_REPEAT_PAIRING:
//                         /* We already have a bond with the peer, but it is attempting to
//                         * establish a new secure link.  This app sacrifices security for
//                         * convenience: just throw away the old bond and accept the new link.
//                         */

//                         /* Delete the old bond. */
//                         rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
//                         assert(rc == 0);
//                         ble_store_util_delete_peer(&desc.peer_id_addr);

//                         /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
//                         * continue with the pairing operation.
//                         */
//                     return BLE_GAP_REPEAT_PAIRING_RETRY;

//                     case BLE_GAP_EVENT_PASSKEY_ACTION:
//                         ESP_LOGI(instance.TAG, "PASSKEY_ACTION_EVENT started");
//                         struct ble_sm_io pkey = {0};
//                         int key = 0;

//                         if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
//                             pkey.action = event->passkey.params.action;
//                             pkey.passkey = 123456; // This is the passkey to be entered on peer
//                             ESP_LOGI(instance.TAG, "Enter passkey %" PRIu32 "on the peer side", pkey.passkey);
//                             rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
//                             ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
//                         } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
//                             ESP_LOGI(instance.TAG, "Accepting passkey..");
//                             pkey.action = event->passkey.params.action;
//                             pkey.numcmp_accept = key;
//                             rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
//                             ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
//                         } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
//                             static uint8_t tem_oob[16] = {0};
//                             pkey.action = event->passkey.params.action;
//                             for (int i = 0; i < 16; i++) {
//                                 pkey.oob[i] = tem_oob[i];
//                             }
//                             rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
//                             ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
//                         } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
//                             ESP_LOGI(instance.TAG, "Input not supported passing -> 123456");
//                             pkey.action = event->passkey.params.action;
//                             pkey.passkey = 123456;
//                             rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
//                             ESP_LOGI(instance.TAG, "ble_sm_inject_io result: %d", rc);
//                         }
//                     return 0;    
//                 }

//                 return 0;
//             }

//             /*GATT服务访问回调*/
//             inline static int gatt_svr_access_cb(uint16_t conn_handle, uint16_t attr_handle,struct ble_gatt_access_ctxt *ctxt, void *arg)
//             {
//                 bt& instance = bt::getInstance();

//                 //ESP_LOGI(instance.TAG, "gatt_svr_access_cb: attr_handle=%d, op=%d", attr_handle, ctxt->op);
                
//                 switch (ctxt->op) {
//                     case BLE_GATT_ACCESS_OP_READ_CHR:  /*0 = 读取特征值*/
//                         if (attr_handle == instance.hid_info_handle) {
//                             /*读取HID信息特征值*/
//                             os_mbuf_append(ctxt->om, instance.hid_info, sizeof(instance.hid_info));
//                             return 0;
//                         } else if (attr_handle == instance.hid_report_map_handle) {
//                             /*读取报告描述符*/
//                             os_mbuf_append(ctxt->om, instance.gamepadReportMap, sizeof(instance.gamepadReportMap));
//                             return 0;
//                         } else if (attr_handle == instance.hid_protocol_mode_handle) {
//                             /*读取协议模式*/
//                             os_mbuf_append(ctxt->om, &instance.protocol_mode, 1);
//                             return 0;
//                         } else if (attr_handle == instance.hid_report_handle) {
//                             /*读取HID报告特征值*/
//                             os_mbuf_append(ctxt->om, instance.report_buffer, sizeof(instance.report_buffer));
//                             return 0;
//                         } else if (attr_handle == instance.device_name_handle) {
//                             /*读取设备名称*/
//                             os_mbuf_append(ctxt->om, BT_DEVICE_NAME, strlen(BT_DEVICE_NAME));
//                             return 0;
//                         } else if (attr_handle == instance.appearance_handle) {
//                             /*读取外观*/
//                             uint16_t appearance = BT_HID_DEVICE_APPEARANCE;
//                             os_mbuf_append(ctxt->om, &appearance, sizeof(appearance));
//                             return 0;
//                         } else if (attr_handle == instance.feature_handle) {
//                             /*读取GATT特征*/
//                             uint8_t feature = 0x00; // No features supported
//                             os_mbuf_append(ctxt->om, &feature, sizeof(feature));
//                             return 0;
//                         } else if (attr_handle == instance.client_feature_handle) {
//                             /*读取客户端特征*/
//                             uint8_t client_feature = 0x00; // No client features
//                             os_mbuf_append(ctxt->om, &client_feature, sizeof(client_feature));
//                             return 0;
//                         } else if (attr_handle == instance.scan_interval_window_handle) {
//                             /*读取扫描间隔窗口*/
//                             uint8_t scan_data[4];
//                             scan_data[0] = instance.scan_interval & 0xFF;
//                             scan_data[1] = (instance.scan_interval >> 8) & 0xFF;
//                             scan_data[2] = instance.scan_window & 0xFF;
//                             scan_data[3] = (instance.scan_window >> 8) & 0xFF;
//                             os_mbuf_append(ctxt->om, scan_data, sizeof(scan_data));
//                             return 0;
//                         } else if (attr_handle == instance.battery_level_handle) {
//                             /*读取电池电量*/
//                             os_mbuf_append(ctxt->om, &instance.battery_level, sizeof(instance.battery_level));
//                             return 0;
//                         } else if (attr_handle == instance.model_number_handle) {
//                             /*读取型号号码*/
//                             os_mbuf_append(ctxt->om, instance.model_number, strlen(instance.model_number));
//                             return 0;
//                         } else if (attr_handle == instance.firmware_revision_handle) {
//                             /*读取固件版本*/
//                             os_mbuf_append(ctxt->om, instance.firmware_revision, strlen(instance.firmware_revision));
//                             return 0;
//                         } else if (attr_handle == instance.manufacturer_name_handle) {
//                             /*读取制造商名称*/
//                             os_mbuf_append(ctxt->om, instance.manufacturer_name, strlen(instance.manufacturer_name));
//                             return 0;
//                         }
//                         break;
                        
//                     case BLE_GATT_ACCESS_OP_WRITE_CHR:  /*1 = 写入特征值*/
//                         if (attr_handle == instance.hid_ctrl_pt_handle) {
//                             /*写入控制点*/
//                             uint8_t cmd;
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, &cmd, 1, NULL);
//                             if (rc != 0) {
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
//                             ESP_LOGI(instance.TAG, "HID Control Point command: %d", cmd);
//                             return 0;
//                         } else if (attr_handle == instance.hid_protocol_mode_handle) {
//                             /*写入协议模式*/
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, &instance.protocol_mode, 1, NULL);
//                             if (rc != 0) {
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
//                             ESP_LOGI(instance.TAG, "Protocol mode set to: %d", instance.protocol_mode);
//                             return 0;
//                         } else if (attr_handle == instance.scan_refresh_handle) {
//                             /*写入扫描刷新*/
//                             uint8_t value;
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, &value, 1, NULL);
//                             if (rc != 0) {
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
//                             ESP_LOGI(instance.TAG, "Scan refresh value: %d", value);
//                             return 0;
//                         } else if (attr_handle == instance.client_feature_handle) {
//                             /*写入客户端特征*/
//                             uint8_t value;
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, &value, 1, NULL);
//                             if (rc != 0) {
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
//                             ESP_LOGI(instance.TAG, "Client feature value: %d", value);
//                             return 0;
//                         } else if (attr_handle == instance.hid_report_handle) {
//                             /*写入HID报告 - 处理输入报告*/
//                             // 获取写入数据的长度
//                             uint16_t om_len = OS_MBUF_PKTLEN(ctxt->om);
//                             if (om_len > sizeof(instance.report_buffer)) {
//                                 ESP_LOGE(instance.TAG, "HID report too long: %d", om_len);
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
                            
//                             /*清空报告缓冲区*/
//                             memset(instance.report_buffer, 0, sizeof(instance.report_buffer));
                            
//                             /*从mbuf中提取数据*/
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, instance.report_buffer, om_len, NULL);
//                             if (rc != 0) {
//                                 ESP_LOGE(instance.TAG, "Failed to extract report data: %d", rc);
//                                 return BLE_ATT_ERR_UNLIKELY;
//                             }
                            
//                             ESP_LOGI(instance.TAG, "HID report received, length: %d", om_len);
//                             /*根据需要记录报告内容，避免过长的日志*/
//                             if (om_len > 0) {
//                                 ESP_LOGI(instance.TAG, "First byte: %02X", instance.report_buffer[0]);
//                             }
//                             return 0;
//                         }
//                         break;
                        
//                     case BLE_GATT_ACCESS_OP_READ_DSC:  /*2 = 读取描述符*/
//                         if (attr_handle == instance.hid_report_ccc_handle) {
//                             /*读取CCC描述符 - 返回存储的值*/
//                             os_mbuf_append(ctxt->om, &instance.ccc_value, 2);
//                             return 0;
//                         } else if (attr_handle == instance.service_changed_ccc_handle) {
//                             /*读取服务变更CCC描述符*/
//                             uint16_t ccc_val = 0x0000; // Notifications disabled
//                             os_mbuf_append(ctxt->om, &ccc_val, sizeof(ccc_val));
//                             return 0;
//                         } else if (attr_handle == instance.scan_interval_window_ccc_handle) {
//                             /*读取扫描间隔窗口CCC描述符*/
//                             uint16_t ccc_val = 0x0000; // Notifications disabled
//                             os_mbuf_append(ctxt->om, &ccc_val, sizeof(ccc_val));
//                             return 0;
//                         } else if (attr_handle == instance.hid_report_map_ext_desc_handle) {
//                             /*读取报告映射扩展描述符*/
//                             uint16_t ext_prop = 0x0000; // No extended properties
//                             os_mbuf_append(ctxt->om, &ext_prop, sizeof(ext_prop));
//                             return 0;
//                         } else if (attr_handle == instance.hid_report_ref_desc_handle) {
//                             /*读取报告引用描述符*/
//                             uint8_t report_ref[2] = {0x01, 0x01}; // Report ID 1, Input report
//                             os_mbuf_append(ctxt->om, report_ref, sizeof(report_ref));
//                             return 0;
//                         }
//                         break;
                        
//                     case BLE_GATT_ACCESS_OP_WRITE_DSC:  /*3 = 写入描述符*/
//                         if (attr_handle == instance.hid_report_ccc_handle) {
//                             /*写入CCC描述符 - 保存值并处理通知状态*/
//                             uint16_t ccc_val;
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, &ccc_val, 2, NULL);
//                             if (rc != 0) {
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
                            
//                             /*保存CCC值*/
//                             instance.ccc_value = ccc_val;
                            
//                             /*检查通知是否启用*/
//                             bool notify_enabled = (ccc_val & BLE_GATT_CHR_PROP_NOTIFY) != 0;
//                             ESP_LOGI(instance.TAG, "CCC value set to: %d, Notify %s", 
//                                     ccc_val, notify_enabled ? "enabled" : "disabled");
//                             return 0;
//                         } else if (attr_handle == instance.service_changed_ccc_handle) {
//                             /*写入服务变更CCC描述符*/
//                             uint16_t ccc_val;
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, &ccc_val, 2, NULL);
//                             if (rc != 0) {
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
//                             ESP_LOGI(instance.TAG, "Service changed CCC value: %d", ccc_val);
//                             return 0;
//                         } else if (attr_handle == instance.scan_interval_window_ccc_handle) {
//                             /*写入扫描间隔窗口CCC描述符*/
//                             uint16_t ccc_val;
//                             int rc = ble_hs_mbuf_to_flat(ctxt->om, &ccc_val, 2, NULL);
//                             if (rc != 0) {
//                                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
//                             }
//                             ESP_LOGI(instance.TAG, "Scan interval window CCC value: %d", ccc_val);
//                             return 0;
//                         }
//                         break;
//                 }

//                 ESP_LOGE(instance.TAG, "gatt_svr_access_cb err: attr_handle=%d, op=%d", attr_handle, ctxt->op);
                
//                 return BLE_ATT_ERR_UNLIKELY;
//             }

//             /*GATT注册回调*/
//             inline static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
//             {
//                 bt& instance = bt::getInstance();
//                 char buf[BLE_UUID_STR_LEN];
                
//                 switch (ctxt->op) {
//                     case BLE_GATT_REGISTER_OP_SVC:
//                         ESP_LOGI(instance.TAG, "registered service %s with handle=%d",
//                                 ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
//                                 ctxt->svc.handle);
//                         if (ble_uuid_cmp(ctxt->svc.svc_def->uuid, &instance.gap_service_uuid.u) == 0) {
//                             instance.gap_service_handle = ctxt->svc.handle;
//                         } else if (ble_uuid_cmp(ctxt->svc.svc_def->uuid, &instance.gatt_service_uuid.u) == 0) {
//                             instance.gatt_service_handle = ctxt->svc.handle;
//                         } else if (ble_uuid_cmp(ctxt->svc.svc_def->uuid, &instance.scan_params_service_uuid.u) == 0) {
//                             instance.scan_params_service_handle = ctxt->svc.handle;
//                         } else if (ble_uuid_cmp(ctxt->svc.svc_def->uuid, &instance.battery_service_uuid.u) == 0) {
//                             instance.battery_service_handle = ctxt->svc.handle;
//                         } else if (ble_uuid_cmp(ctxt->svc.svc_def->uuid, &instance.device_info_service_uuid.u) == 0) {
//                             instance.device_info_service_handle = ctxt->svc.handle;
//                         } else if (ble_uuid_cmp(ctxt->svc.svc_def->uuid, &instance.hid_service_uuid.u) == 0) {
//                             instance.hid_service_handle = ctxt->svc.handle;
//                         }
//                         break;
                        
//                     case BLE_GATT_REGISTER_OP_CHR:
//                         ESP_LOGI(instance.TAG, "registering characteristic %s with "
//                                 "def_handle=%d val_handle=%d",
//                                 ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
//                                 ctxt->chr.def_handle,
//                                 ctxt->chr.val_handle);      
                        
//                         if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.device_name_chr_uuid.u) == 0) {
//                             instance.device_name_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.appearance_chr_uuid.u) == 0) {
//                             instance.appearance_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.service_changed_chr_uuid.u) == 0) {
//                             instance.service_changed_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.feature_chr_uuid.u) == 0) {
//                             instance.feature_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.client_feature_chr_uuid.u) == 0) {
//                             instance.client_feature_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.scan_refresh_chr_uuid.u) == 0) {
//                             instance.scan_refresh_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.scan_interval_window_chr_uuid.u) == 0) {
//                             instance.scan_interval_window_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.battery_level_chr_uuid.u) == 0) {
//                             instance.battery_level_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.model_number_chr_uuid.u) == 0) {
//                             instance.model_number_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.firmware_revision_chr_uuid.u) == 0) {
//                             instance.firmware_revision_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.manufacturer_name_chr_uuid.u) == 0) {
//                             instance.manufacturer_name_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.hid_info_chr_uuid.u) == 0) {
//                             instance.hid_info_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.hid_report_map_chr_uuid.u) == 0) {
//                             instance.hid_report_map_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.hid_report_chr_uuid.u) == 0) {
//                             instance.hid_report_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.hid_ctrl_pt_chr_uuid.u) == 0) {
//                             instance.hid_ctrl_pt_handle = ctxt->chr.val_handle;
//                         } else if (ble_uuid_cmp(ctxt->chr.chr_def->uuid, &instance.hid_protocol_mode_chr_uuid.u) == 0) {
//                             instance.hid_protocol_mode_handle = ctxt->chr.val_handle;
//                         }
//                         break;
                        
//                     case BLE_GATT_REGISTER_OP_DSC:
//                         ESP_LOGI(instance.TAG, "registering descriptor %s with handle=%d",
//                                 ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
//                                 ctxt->dsc.handle);
                        
//                         if (ble_uuid_cmp(ctxt->dsc.dsc_def->uuid, &instance.cccd_uuid.u) == 0) {
//                             if (ctxt->dsc.chr_def->uuid == &instance.service_changed_chr_uuid.u) {
//                                 instance.service_changed_ccc_handle = ctxt->dsc.handle;
//                             } else if (ctxt->dsc.chr_def->uuid == &instance.scan_interval_window_chr_uuid.u) {
//                                 instance.scan_interval_window_ccc_handle = ctxt->dsc.handle;
//                             } else if (ctxt->dsc.chr_def->uuid == &instance.hid_report_chr_uuid.u) {
//                                 instance.hid_report_ccc_handle = ctxt->dsc.handle;
//                             }
//                         } else if (ble_uuid_cmp(ctxt->dsc.dsc_def->uuid, &instance.ext_desc_uuid.u) == 0) {
//                             instance.hid_report_map_ext_desc_handle = ctxt->dsc.handle;
//                         } else if (ble_uuid_cmp(ctxt->dsc.dsc_def->uuid, &instance.report_ref_desc_uuid.u) == 0) {
//                             instance.hid_report_ref_desc_handle = ctxt->dsc.handle;
//                         }
//                         break;
//                 }
//             }

//             /*注册所有服务*/
//             inline static void gatt_svr_register_services(void)
//             {
//                 bt& instance = bt::getInstance();
//                 int rc;

//                 /*GAP服务*/
//                 static struct ble_gatt_chr_def gap_chars[] = {
//                     {
//                         .uuid = &instance.device_name_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.device_name_handle
//                     },
//                     {
//                         .uuid = &instance.appearance_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.appearance_handle
//                     },
//                     { 0 }
//                 };

//                 /*GATT服务*/
//                 static struct ble_gatt_dsc_def service_changed_descriptors[] = {
//                     {
//                         .uuid = &instance.cccd_uuid.u,
//                         .att_flags = BLE_ATT_F_READ | BLE_ATT_F_WRITE,
//                         .min_key_size = 0,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL
//                     },
//                     { 0 }
//                 };
                
//                 static struct ble_gatt_chr_def gatt_chars[] = {
//                     {
//                         .uuid = &instance.service_changed_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = service_changed_descriptors,
//                         .flags = BLE_GATT_CHR_F_INDICATE,
//                         .min_key_size = 0,
//                         .val_handle = &instance.service_changed_handle
//                     },
//                     {
//                         .uuid = &instance.feature_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.feature_handle
//                     },
//                     {
//                         .uuid = &instance.client_feature_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
//                         .min_key_size = 0,
//                         .val_handle = &instance.client_feature_handle
//                     },
//                     { 0 }
//                 };

//                 /*扫描参数服务*/
//                 static struct ble_gatt_dsc_def scan_interval_window_descriptors[] = {
//                     {
//                         .uuid = &instance.cccd_uuid.u,
//                         .att_flags = BLE_ATT_F_READ | BLE_ATT_F_WRITE,
//                         .min_key_size = 0,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL
//                     },
//                     { 0 }
//                 };
                
//                 static struct ble_gatt_chr_def scan_params_chars[] = {
//                     {
//                         .uuid = &instance.scan_refresh_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
//                         .min_key_size = 0,
//                         .val_handle = &instance.scan_refresh_handle
//                     },
//                     {
//                         .uuid = &instance.scan_interval_window_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = scan_interval_window_descriptors,
//                         .flags = BLE_GATT_CHR_F_NOTIFY,
//                         .min_key_size = 0,
//                         .val_handle = &instance.scan_interval_window_handle
//                     },
//                     { 0 }
//                 };

//                 /*电池服务*/
//                 static struct ble_gatt_chr_def battery_chars[] = {
//                     {
//                         .uuid = &instance.battery_level_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.battery_level_handle
//                     },
//                     { 0 }
//                 };

//                 /*设备信息服务*/
//                 static struct ble_gatt_chr_def device_info_chars[] = {
//                     {
//                         .uuid = &instance.model_number_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.model_number_handle
//                     },
//                     {
//                         .uuid = &instance.firmware_revision_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.firmware_revision_handle
//                     },
//                     {
//                         .uuid = &instance.manufacturer_name_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.manufacturer_name_handle
//                     },
//                     { 0 }
//                 };

//                 /*HID服务*/
//                 static struct ble_gatt_dsc_def report_descriptors[] = {
//                     {
//                         .uuid = &instance.cccd_uuid.u,
//                         .att_flags = BLE_ATT_F_READ | BLE_ATT_F_WRITE,
//                         .min_key_size = 0,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL
//                     },
//                     {
//                         .uuid = &instance.report_ref_desc_uuid.u,
//                         .att_flags = BLE_ATT_F_READ,
//                         .min_key_size = 0,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL
//                     },
//                     { 0 }
//                 };
                
//                 static struct ble_gatt_dsc_def report_map_descriptors[] = {
//                     {
//                         .uuid = &instance.ext_desc_uuid.u,
//                         .att_flags = BLE_ATT_F_READ,
//                         .min_key_size = 0,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL
//                     },
//                     { 0 }
//                 };
                
//                 static struct ble_gatt_chr_def hid_chars[] = {
//                     {
//                         /*HID协议模式特征*/
//                         .uuid = &instance.hid_protocol_mode_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE_NO_RSP,
//                         .min_key_size = 0,
//                         .val_handle = &instance.hid_protocol_mode_handle
//                     },
//                     {
//                         /*HID报告映射特征*/
//                         .uuid = &instance.hid_report_map_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = report_map_descriptors,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.hid_report_map_handle
//                     },
//                     {
//                         /*HID报告特征*/
//                         .uuid = &instance.hid_report_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = report_descriptors,
//                         .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
//                         .min_key_size = 0,
//                         .val_handle = &instance.hid_report_handle
//                     },
//                     {
//                         /*HID信息特征*/
//                         .uuid = &instance.hid_info_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_READ,
//                         .min_key_size = 0,
//                         .val_handle = &instance.hid_info_handle
//                     },
//                     {
//                         /*HID控制点特征*/
//                         .uuid = &instance.hid_ctrl_pt_chr_uuid.u,
//                         .access_cb = gatt_svr_access_cb,
//                         .arg = NULL,
//                         .descriptors = NULL,
//                         .flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
//                         .min_key_size = 0,
//                         .val_handle = &instance.hid_ctrl_pt_handle
//                     },
//                     { 0 }
//                 };

//                 /*所有服务的定义*/
//                 static struct ble_gatt_svc_def gatt_svr_svcs[] = {
//                     {
//                         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//                         .uuid = &instance.gap_service_uuid.u,
//                         .characteristics = gap_chars,
//                     },
//                     {
//                         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//                         .uuid = &instance.gatt_service_uuid.u,
//                         .characteristics = gatt_chars,
//                     },
//                     {
//                         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//                         .uuid = &instance.scan_params_service_uuid.u,
//                         .characteristics = scan_params_chars,
//                     },
//                     {
//                         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//                         .uuid = &instance.battery_service_uuid.u,
//                         .characteristics = battery_chars,
//                     },
//                     {
//                         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//                         .uuid = &instance.device_info_service_uuid.u,
//                         .characteristics = device_info_chars,
//                     },
//                     {
//                         .type = BLE_GATT_SVC_TYPE_PRIMARY,
//                         .uuid = &instance.hid_service_uuid.u,
//                         .characteristics = hid_chars,
//                     },
//                     { 0 }
//                 };
                
//                 /*注册服务*/
//                 rc = ble_gatts_count_cfg(gatt_svr_svcs);
//                 if (rc != 0) {
//                     ESP_LOGE(instance.TAG, "Failed to count GATTS configuration: %d", rc);
//                     return;
//                 }
                
//                 rc = ble_gatts_add_svcs(gatt_svr_svcs);
//                 if (rc != 0) {
//                     ESP_LOGE(instance.TAG, "Failed to add GATTS services: %d", rc);
//                     return;
//                 }
                
//                 /*设置初始协议模式为报告协议*/
//                 instance.protocol_mode = 1;
                
//                 ESP_LOGI(instance.TAG, "All services registered");
//             }

//             inline esp_err_t esp_hid_ble_gap_adv_init(const char *device_name)
//             {
//                 /**
//                  *  Set the advertisement data included in our advertisements:
//                  *     o Flags (indicates advertisement type and other general info).
//                  *     o Advertising tx power.
//                  *     o Device name.
//                  *     o 16-bit service UUIDs (HID).
//                  */

//                 memset(&fields, 0, sizeof(fields));
//                 memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));

//                 /* Advertise two flags:
//                 *     o Discoverability in forthcoming advertisement (general)
//                 *     o BLE-only (BR/EDR unsupported).
//                 */
//                 fields.flags = BLE_HS_ADV_F_DISC_GEN |
//                             BLE_HS_ADV_F_BREDR_UNSUP;

//                 /* Indicate that the TX power level field should be included; have the
//                 * stack fill this value automatically.  This is done by assigning the
//                 * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
//                 */
//                 fields.tx_pwr_lvl_is_present = 1;
//                 fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

//                 /* 服务UUID */
//                 service_uuid = BLE_UUID16_INIT(GATT_SVR_SVC_HID_UUID);
//                 fields.uuids16 = &service_uuid;
//                 fields.num_uuids16 = 1;
//                 fields.uuids16_is_complete = 1;

//                 /* 设备外观 */
//                 fields.appearance = BT_HID_DEVICE_APPEARANCE;
//                 fields.appearance_is_present = 1;

//                 /* 扫描响应中包含设备名称 */
//                 scan_rsp_fields.name = (uint8_t *)device_name;
//                 scan_rsp_fields.name_len = strlen(device_name);
//                 scan_rsp_fields.name_is_complete = 1;

//                 /*设置GAP设备名称*/
//                 ble_svc_gap_device_name_set(device_name);

//                 return ESP_OK;
//             }

//             inline static void ble_hid_device_host_task(void *param)
//             {
//                 bt& instance = bt::getInstance();
//                 /* This function will return only when nimble_port_stop() is executed */
//                 nimble_port_run();
//                 instance.host_task_is_done = true;
//                 nimble_port_freertos_deinit();
//             }

//             inline esp_err_t start_advertising(void)
//             {
//                 int rc;
//                 struct ble_gap_adv_params adv_params;
//                 /* maximum possible duration for hid device(180s) */
//                 int32_t adv_duration_ms = 180000;

//                 /*如果已经在广播，先停止*/
//                 if (advertising) {
//                     rc = ble_gap_adv_stop();
//                     if (rc != 0 && rc != BLE_HS_EALREADY) {
//                         ESP_LOGE(TAG, "ble_gap_adv_stop failed: %d", rc);
//                     }
//                     advertising = false;
//                     vTaskDelay(pdMS_TO_TICKS(20)); /*等待停止完成*/
//                 }

//                 /*设置广播字段*/
//                 rc = ble_gap_adv_set_fields(&fields);
//                 if (rc != 0) {
//                     ESP_LOGE(TAG, "ble_gap_adv_set_fields failed: %d", rc);
//                     return rc;
//                 }

//                 /* 设置扫描响应数据 */
//                 rc = ble_gap_adv_rsp_set_fields(&scan_rsp_fields);
//                 if (rc != 0) {
//                     ESP_LOGE(TAG, "ble_gap_adv_rsp_set_fields failed: %d", rc);
//                     return rc;
//                 }

//                 /* Begin advertising. */
//                 memset(&adv_params, 0, sizeof(adv_params));
//                 adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
//                 adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
//                 adv_params.itvl_min = BLE_GAP_ADV_ITVL_MS(30);/* Recommended interval 30ms to 50ms */
//                 adv_params.itvl_max = BLE_GAP_ADV_ITVL_MS(50);
//                 rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, adv_duration_ms,
//                                     &adv_params, gap_event_callback, NULL);
//                 if (rc != 0) {
//                     ESP_LOGE(TAG, "error enabling advertisement; rc=%d\n", rc);
//                     return rc;
//                 }

//                 advertising = true; /*更新广播状态*/
//                 return rc;
//             }

//             /*私有构造函数，禁止外部直接实例化*/
//             bt(){
//                 memset(report_buffer, 0, sizeof(report_buffer));
//                 connected = false;
//                 advertising = false;
//                 conn_handle = BLE_HS_CONN_HANDLE_NONE;
//                 memset(&fields, 0, sizeof(fields));
//                 memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));
//                 service_uuid = BLE_UUID16_INIT(0);
//                 host_task_is_done = false;
                
//                 /*初始化新服务句柄*/
//                 gap_service_handle = 0;
//                 device_name_handle = 0;
//                 appearance_handle = 0;
                
//                 gatt_service_handle = 0;
//                 service_changed_handle = 0;
//                 feature_handle = 0;
//                 client_feature_handle = 0;
//                 service_changed_ccc_handle = 0;
                
//                 scan_params_service_handle = 0;
//                 scan_refresh_handle = 0;
//                 scan_interval_window_handle = 0;
//                 scan_interval_window_ccc_handle = 0;
                
//                 battery_service_handle = 0;
//                 battery_level_handle = 0;
                
//                 device_info_service_handle = 0;
//                 model_number_handle = 0;
//                 firmware_revision_handle = 0;
//                 manufacturer_name_handle = 0;
                
//                 /*初始化HID服务句柄*/
//                 hid_service_handle = 0;
//                 hid_info_handle = 0;
//                 hid_report_map_handle = 0;
//                 hid_report_handle = 0;
//                 hid_ctrl_pt_handle = 0;
//                 hid_protocol_mode_handle = 0;
//                 hid_report_ccc_handle = 0;
//                 hid_report_map_ext_desc_handle = 0;
//                 hid_report_ref_desc_handle = 0;
                
//                 protocol_mode = 1; /*默认使用报告协议*/
//                 ccc_value = 0;
//             }
//             /*禁止拷贝构造和赋值操作*/
//             bt(const bt&) = delete;
//             bt& operator = (const bt&) = delete;
//         public:
//             /*获取单例实例的静态方法*/
//             inline static bt& getInstance() {
//                 static bt instance;
//                 return instance;
//             }

//             inline esp_err_t init(const char* device_name = BT_DEVICE_NAME)
//             {
//                 esp_err_t ret;

//                 ESP_LOGI(TAG, "Initializing nimble_port");
//                 ret = nimble_port_init();
//                 if (ret != ESP_OK) {
//                     ESP_LOGE(TAG, "nimble_port_init failed: %d", ret);
//                     return ret;
//                 }

//                 /* XXX Need to have template for store */
//                 ble_store_config_init();     
//                 ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

//                 /*设置GATT注册回调*/
//                 ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
//                 ble_hs_cfg.gatts_register_arg = NULL;
//                 /* 安全配置 */
//                 ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_NO_IO;
//                 ble_hs_cfg.sm_bonding = 1;      
//                 ble_hs_cfg.sm_mitm = 1;         
//                 ble_hs_cfg.sm_sc = 1;           
//                 ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC;      /*仅分发加密密钥*/
//                 ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC;    /*仅需要加密密钥*/

//                 /*注册所有服务*/
//                 gatt_svr_register_services();

//                 ESP_LOGI(TAG, "Initializing nimble_port_freertos");
//                 nimble_port_freertos_init(ble_hid_device_host_task);
                
//                 /*初始化BLE广播*/
//                 ESP_LOGI(TAG, "Initializing BLE advertising");
//                 ret = esp_hid_ble_gap_adv_init(device_name);
//                 if (ret != ESP_OK) {
//                     ESP_LOGE(TAG, "BLE advertising initialization failed: %d", ret);
//                     return ret;
//                 }

//                 /*开始广播*/
//                 ESP_LOGI(TAG, "Starting advertising");
//                 ret = start_advertising();
//                 if (ret != ESP_OK) {
//                     ESP_LOGE(TAG, "Advertising start failed: %d", ret);
//                     return ret;
//                 }
                
//                 return ESP_OK;
//             }

//             inline esp_err_t deinit()
//             {
//                 esp_err_t ret = ESP_OK;
                
//                 /*断开连接*/
//                 if (connected) {
//                     if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                         int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
//                         if (rc != 0) {
//                             ESP_LOGE(TAG, "Failed to terminate connection: %d", rc);
//                             ret = ESP_FAIL;
//                         } else {
//                             ESP_LOGI(TAG, "Connection terminated");
//                         }
//                     }
//                     connected = false;
//                 }

//                 /*停止广播*/
//                 if (advertising) {
//                     int rc = ble_gap_adv_stop();
//                     if (rc != 0 && rc != BLE_HS_EALREADY) {
//                         ESP_LOGE(TAG, "ble_gap_adv_stop failed: %d", rc);
//                         ret = ESP_FAIL;
//                     }
//                     advertising = false;
//                 }

//                 /*停止NimBLE*/
//                 host_task_is_done = false;
//                 nimble_port_stop();                 /*先停止NimBLE端口*/
//                 /*增加等待时间*/
//                 int timeout = 50; /*最多等待500ms*/
//                 while(!host_task_is_done && timeout-- > 0) {
//                     vTaskDelay(pdMS_TO_TICKS(10));
//                 }
                
//                 if (!host_task_is_done) {
//                     ESP_LOGW(TAG, "Host task didn't finish properly");
//                 }
//                 nimble_port_deinit();               /*内部调用了esp_bt_controller_disable 和 esp_bt_controller_deinit*/

//                 /*重置报告缓冲区*/
//                 memset(report_buffer, 0, sizeof(report_buffer));
                
//                 ESP_LOGI(TAG, "Bluetooth deinitialized");
//                 return ret;
//             }

//             inline esp_err_t stop()
//             {
//                 esp_err_t ret = ESP_OK;
                
//                 /*断开连接*/
//                 if (connected) {
//                     if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                         int rc = ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
//                         if (rc != 0) {
//                             ESP_LOGE(TAG, "Failed to terminate connection: %d", rc);
//                             ret = ESP_FAIL;
//                         } else {
//                             ESP_LOGI(TAG, "Connection terminated");
//                         }
//                     }
//                     connected = false;
//                 }

//                 /*停止广播*/
//                 if (advertising) {
//                     int rc = ble_gap_adv_stop();
//                     if (rc != 0 && rc != BLE_HS_EALREADY) {
//                         ESP_LOGE(TAG, "ble_gap_adv_stop failed: %d", rc);
//                         ret = ESP_FAIL;
//                     }
//                     advertising = false;
//                 }

//                 /*重置报告缓冲区*/
//                 memset(report_buffer, 0, sizeof(report_buffer));

//                 ESP_LOGI(TAG, "Bluetooth Stop");

//                 return ret;
//             }

//             inline esp_err_t start()
//             {
//                 esp_err_t ret = ESP_OK;

//                 /*开始广播*/
//                 ESP_LOGI(TAG, "Starting advertising");
//                 ret = start_advertising();
//                 if (ret != ESP_OK) {
//                     ESP_LOGE(TAG, "Advertising start failed: %d", ret);
//                     return ret;
//                 }

//                 ESP_LOGI(TAG, "Bluetooth Start");

//                 return ret;
//             }

//              /*设置按钮状态 (0-9)*/
//             inline void set_button(uint8_t button_index, bool state)
//             {
//                 if (button_index >= 10) return; /* 只支持10个按钮 (0-9) */
                
//                 /* 按钮数据存储在字节10和11 */
//                 if (button_index < 8) {
//                     /* 按钮0-7在字节10 */
//                     uint8_t bit_index = button_index;
//                     if (state) {
//                         report_buffer[10] |= (1 << bit_index);
//                     } else {
//                         report_buffer[10] &= ~(1 << bit_index);
//                     }
//                 } else {
//                     /* 按钮8-9在字节11的第0位和第1位 */
//                     uint8_t bit_index = button_index - 8;
//                     if (state) {
//                         report_buffer[11] |= (1 << bit_index);
//                     } else {
//                         report_buffer[11] &= ~(1 << bit_index);
//                     }
//                 }
//             }

//             /*设置帽式开关 (0-8, 0=中心/空状态)*/
//             inline void set_hat_switch(uint8_t value)
//             {
//                 if (value > 8) return; /* 有效值0-8 */
                
//                 /* 帽式开关存储在字节11的第2-5位 (高4位) */
//                 report_buffer[11] &= 0xC3; /* 清除原有值 (保留按钮8-9和常量位) */
//                 report_buffer[11] |= ((value & 0x0F) << 2); /* 设置新值 */
//             }

//             /*设置摇杆值 (0-65535)*/
//             inline void set_joystick(uint16_t lx, uint16_t ly, uint16_t rx, uint16_t ry)
//             {
//                 /*
//                               90° (上)
//                             (0,65535) ★
//                                 |
//                             180° --●-- 0° (右) ● = 中心(32768,32768)
//                             (0,0) |     (65535,32768)
//                                 |
//                             270° (下)
//                             (32768,0)
//                 */
//                /* X/Y 在字节 0-3 */
//                report_buffer[0] = lx & 0xFF;
//                report_buffer[1] = (lx >> 8) & 0xFF;
//                report_buffer[2] = ly & 0xFF;
//                report_buffer[3] = (ly >> 8) & 0xFF;
               
//                /* Rx/Ry 在字节 4-7 */
//                report_buffer[4] = rx & 0xFF;
//                report_buffer[5] = (rx >> 8) & 0xFF;
//                report_buffer[6] = ry & 0xFF;
//                report_buffer[7] = (ry >> 8) & 0xFF;
//             }

//             /*设置Z轴值 (0-65535)*/
//             inline void set_z_axis(uint16_t z)
//             {
//                /* Z轴在字节 8-9 */
//                report_buffer[8] = z & 0xFF;
//                report_buffer[9] = (z >> 8) & 0xFF;
//             }

//             inline bool is_connected()
//             {
//                 return connected;
//             }

//             inline void send_report()
//             {
//                 if (connected && conn_handle != BLE_HS_CONN_HANDLE_NONE) {
//                     /*使用NimBLE API发送报告通知*/
//                     struct os_mbuf *om;
//                     int rc;
                    
//                     /*创建mbuf来存储报告数据*/
//                     om = ble_hs_mbuf_from_flat(report_buffer, sizeof(report_buffer));
//                     if (!om) {
//                         ESP_LOGE(TAG, "Failed to create mbuf for report");
//                         return;
//                     }
                    
//                     /*发送通知*/
//                     rc = ble_gatts_notify_custom(conn_handle, hid_report_handle, om);
//                     if (rc != 0) {
//                         ESP_LOGE(TAG, "Failed to send report notification: %d", rc);
//                         os_mbuf_free_chain(om);
//                     }
//                 }
//             }

//             inline void test_gamepad()
//             {
//                 if (!is_connected()) {
//                     ESP_LOGI(TAG, "Waiting for connection...");
//                     start_advertising();  /*确保广播已启动*/
                    
//                     /*等待连接建立（最多15秒）*/
//                     int timeout = 150; /*150 * 100ms = 15秒*/
//                     while (!is_connected() && timeout-- > 0) {
//                         vTaskDelay(pdMS_TO_TICKS(100));
//                     }
                    
//                     if (!is_connected()) {
//                         ESP_LOGE(TAG, "Connection failed! Test aborted.");
//                         return;
//                     }
//                 }

//                 ESP_LOGI(TAG, "Starting gamepad test...");

//                 /*1. 测试所有按钮*/
//                 ESP_LOGI(TAG, "Testing buttons 0-9...");
//                 for (int i = 0; i < 10; i++) {
//                     set_button(i, true);
//                     send_report();
//                     vTaskDelay(pdMS_TO_TICKS(200));
//                     set_button(i, false);
//                     send_report();
//                     vTaskDelay(pdMS_TO_TICKS(50));
//                 }

//                 /*2. 测试帽式开关*/
//                 ESP_LOGI(TAG, "Testing hat switch...");
//                 for (int i = 0; i <= 8; i++) {
//                     set_hat_switch(i);
//                     send_report();
//                     vTaskDelay(pdMS_TO_TICKS(300));
//                 }

//                 /*3. 测试摇杆和Z轴*/
//                 ESP_LOGI(TAG, "Testing joysticks and Z-axis...");

//                 /*摇杆中心位置*/
//                 const uint16_t center = 32768;
                
//                 /*摇杆极限位置测试*/
//                 set_joystick(0, 0, 0, 0); /*左下*/
//                 set_z_axis(0);
//                 send_report();
//                 vTaskDelay(pdMS_TO_TICKS(500));
                
//                 set_joystick(65535, 65535, 65535, 65535); /*右上*/
//                 set_z_axis(65535);
//                 send_report();
//                 vTaskDelay(pdMS_TO_TICKS(500));

//                 /*摇杆圆周运动*/
//                 for (int angle = 0; angle < 360; angle += 15) {
//                     float rad = angle * M_PI / 180.0f;
//                     uint16_t x = center + (uint16_t)(center * cosf(rad));
//                     uint16_t y = center + (uint16_t)(center * sinf(rad));
                    
//                     set_joystick(x, y, 65535 - x, 65535 - y);
//                     set_z_axis(angle * 182); /*182 = 65535/360*/
//                     send_report();
//                     vTaskDelay(pdMS_TO_TICKS(50));
//                 }

//                 /*4. 组合测试*/
//                 ESP_LOGI(TAG, "Testing combined inputs...");
                
//                 /*组合1: 方向键+按钮*/
//                 set_hat_switch(1);      /*上*/
//                 set_button(0, true);    /*A键*/
//                 set_button(1, true);    /*B键*/
//                 send_report();
//                 vTaskDelay(pdMS_TO_TICKS(1000));
                
//                 /*组合2: 摇杆+按钮*/
//                 set_hat_switch(0);      /*释放方向键*/
//                 set_joystick(center, 0, 65535, center);
//                 set_button(3, true);    /*X键*/
//                 set_button(7, true);    /*RB键*/
//                 send_report();
//                 vTaskDelay(pdMS_TO_TICKS(1000));

//                 /*重置所有输入*/
//                 set_joystick(center, center, center, center);
//                 set_z_axis(center);
//                 set_hat_switch(0);
//                 for (int i = 0; i < 10; i++) set_button(i, false);
//                 send_report();
                
//                 ESP_LOGI(TAG, "Gamepad test completed!");

//             }

//     };

// }

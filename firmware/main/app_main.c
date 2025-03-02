
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_rmaker_console.h>
#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_schedule.h>
#include <esp_rmaker_console.h>
#include <esp_rmaker_scenes.h>
#include <esp_rmaker_standard_types.h>
#include <json_parser.h>
#include <esp_rmaker_cmd_resp.h>
#include <app_network.h>
#include <esp_rmaker_common_events.h>
#include <esp_rmaker_ota.h>
#include <network_provisioning/manager.h>

#include "app_priv.h"
#include "ws2812_led.h"
#include "helper.h"
#include "app_reset.h"

static const char *TAG = "app_main";

esp_rmaker_device_t *light_device;


/* Event handler for catching RainMaker events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == RMAKER_EVENT) {
        switch (event_id) {
        case RMAKER_EVENT_INIT_DONE:
            ESP_LOGI(TAG, "RainMaker Initialised.");
            break;
        case RMAKER_EVENT_CLAIM_STARTED:
            ESP_LOGI(TAG, "RainMaker Claim Started.");
            break;
        case RMAKER_EVENT_CLAIM_SUCCESSFUL:
            ESP_LOGI(TAG, "RainMaker Claim Successful.");
            break;
        case RMAKER_EVENT_CLAIM_FAILED:
            ESP_LOGI(TAG, "RainMaker Claim Failed.");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled RainMaker Event: %"PRIi32, event_id);
        }
    } else if (event_base == RMAKER_COMMON_EVENT) {
        switch (event_id) {
        case RMAKER_EVENT_REBOOT:
            ESP_LOGI(TAG, "Rebooting in %d seconds.", *((uint8_t *)event_data));
            break;
        case RMAKER_EVENT_WIFI_RESET:
            ESP_LOGI(TAG, "Wi-Fi credentials reset.");
            break;
        case RMAKER_EVENT_FACTORY_RESET:
            ESP_LOGI(TAG, "Node reset to factory defaults.");
            break;
        case RMAKER_MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected.");
            app_light_set_hue(180);
            ws2812_play_mode_enable(1, 3000, 0);

            break;
        case RMAKER_MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected.");
            break;
        case RMAKER_MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT Published. Msg id: %d.", *((int *)event_data));
            break;
        default:
            ESP_LOGW(TAG, "Unhandled RainMaker Common Event: %"PRIi32, event_id);
        }
    } else if (event_base == APP_WIFI_EVENT) {
        switch (event_id) {
        case APP_WIFI_EVENT_QR_DISPLAY:
            ESP_LOGI(TAG, "Provisioning QR : %s", (char *)event_data);
            break;
        case APP_WIFI_EVENT_PROV_TIMEOUT:
            ESP_LOGI(TAG, "Provisioning Timed Out. Please reboot.");
            break;
        case APP_WIFI_EVENT_PROV_RESTART:
            ESP_LOGI(TAG, "Provisioning has restarted due to failures.");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled App Wi-Fi Event: %"PRIi32, event_id);
            break;
        }
    } else if (event_base == RMAKER_OTA_EVENT) {
        switch (event_id) {
        case RMAKER_OTA_EVENT_STARTING:
            ESP_LOGI(TAG, "Starting OTA.");
            break;
        case RMAKER_OTA_EVENT_IN_PROGRESS:
            ESP_LOGI(TAG, "OTA is in progress.");
            break;
        case RMAKER_OTA_EVENT_SUCCESSFUL:
            ESP_LOGI(TAG, "OTA successful.");
            break;
        case RMAKER_OTA_EVENT_FAILED:
            ESP_LOGI(TAG, "OTA Failed.");
            break;
        case RMAKER_OTA_EVENT_REJECTED:
            ESP_LOGI(TAG, "OTA Rejected.");
            break;
        case RMAKER_OTA_EVENT_DELAYED:
            ESP_LOGI(TAG, "OTA Delayed.");
            break;
        case RMAKER_OTA_EVENT_REQ_FOR_REBOOT:
            ESP_LOGI(TAG, "Firmware image downloaded. Please reboot your device to apply the upgrade.");
            break;
        default:
            ESP_LOGW(TAG, "Unhandled OTA Event: %"PRIi32, event_id);
            break;
        }
    } else if (event_base == NETWORK_PROV_EVENT) {
        switch (event_id) {
        case NETWORK_PROV_START:
            ws2812_set_hue(0);
            ws2812_play_mode_enable(1, 3000, 0);

            break;
        case NETWORK_PROV_WIFI_CRED_RECV: {
            ws2812_play_mode_enable(0, 0, 1);
            ws2812_set_hue(0);

            break;
        }
        case NETWORK_PROV_WIFI_CRED_FAIL: {

            break;
        }
        case NETWORK_PROV_WIFI_CRED_SUCCESS:
            break;

        default:
            break;
        }
    } else {
        ESP_LOGW(TAG, "Invalid event received!");
    }
}


/* Callback to handle param updates received from the RainMaker cloud */
static esp_err_t bulk_write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_write_req_t write_req[],
                               uint8_t count, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (ctx) {
        ESP_LOGI(TAG, "Received write request via : %s", esp_rmaker_device_cb_src_to_str(ctx->src));
    }

    ESP_LOGI(TAG, "Light received %d params in write", count);
    for (int i = 0; i < count; i++) {
        const esp_rmaker_param_t *param = write_req[i].param;
        esp_rmaker_param_val_t val = write_req[i].val;
        const char *device_name = esp_rmaker_device_get_name(device);
        const char *param_name = esp_rmaker_param_get_name(param);
        ESP_LOGI(TAG, "Received value = %d for %s - %s", val.val.i, device_name, param_name);

        if (strcmp(param_name, ESP_RMAKER_DEF_POWER_NAME) == 0) {
            app_light_set_power(val.val.b);
        } else if (strcmp(param_name, ESP_RMAKER_DEF_BRIGHTNESS_NAME) == 0) {
            app_light_set_brightness(val.val.i * 5 / 20); // 限制最大亮度
        } else if (strcmp(param_name, ESP_RMAKER_DEF_HUE_NAME) == 0) {
            app_light_set_hue(val.val.i);
        } else if (strcmp(param_name, ESP_RMAKER_DEF_SATURATION_NAME) == 0) {
            app_light_set_saturation(val.val.i);
        } else if (strcmp(param_name, ESP_RMAKER_DEF_SPEED_NAME) == 0) {
            app_motor_set_speed(val.val.i);
        } else if (strcmp(param_name, "Mode") == 0) {
            app_light_set_mode(val.val.s);
        } else if (strcmp(param_name, "Text") == 0) {
            app_light_set_text(val.val.s);
        } else {
            ESP_LOGI(TAG, "Updating for %s", param_name);
        }
        esp_rmaker_param_update(param, val);
    }
    return ESP_OK;
}


static esp_err_t add_device(esp_rmaker_node_t *node)
{
    /* Create a device and add the relevant parameters to it */
    light_device = esp_rmaker_lightbulb_device_create("Ferris Wheel", NULL, ws2812_get_power());
    esp_rmaker_device_add_bulk_cb(light_device, bulk_write_cb, NULL);

    esp_rmaker_device_add_param(light_device, esp_rmaker_brightness_param_create(ESP_RMAKER_DEF_BRIGHTNESS_NAME, ws2812_get_brightness()));
    esp_rmaker_device_add_param(light_device, esp_rmaker_hue_param_create(ESP_RMAKER_DEF_HUE_NAME, ws2812_get_hue()));
    esp_rmaker_device_add_param(light_device, esp_rmaker_saturation_param_create(ESP_RMAKER_DEF_SATURATION_NAME, ws2812_get_saturation()));

    esp_rmaker_param_t *param = esp_rmaker_param_create("Mode", ESP_RMAKER_PARAM_MODE, esp_rmaker_str("normal"), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_DROPDOWN);
        uint8_t valid_strs_len;
        const char **valid_strs = ws2812_effect_get_mode_name_array(&valid_strs_len);
        esp_rmaker_param_add_valid_str_list(param, valid_strs, valid_strs_len);
    }
    esp_rmaker_device_add_param(light_device, param);

    param = esp_rmaker_param_create(ESP_RMAKER_DEF_SPEED_NAME, ESP_RMAKER_PARAM_SPEED, esp_rmaker_int(0), PROP_FLAG_READ | PROP_FLAG_WRITE);
    if (param) {
        esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
        esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(10), esp_rmaker_int(1));
    }
    esp_rmaker_device_add_param(light_device, param);

    param = esp_rmaker_param_create("Text", "esp.param.text", esp_rmaker_str(ws2812_effect_get_text()), PROP_FLAG_READ | PROP_FLAG_WRITE);
    esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_TEXT);
    esp_rmaker_device_add_param(light_device, param);

    esp_rmaker_node_add_device(node, light_device);

    // /* Create a Fan device and add the relevant parameters to it */
    // fan_device = esp_rmaker_fan_device_create("Wheel", NULL, false);
    // esp_rmaker_device_add_cb(fan_device, write_cb, NULL);
    // param = esp_rmaker_param_create(ESP_RMAKER_DEF_SPEED_NAME, ESP_RMAKER_PARAM_SPEED, esp_rmaker_int(3), PROP_FLAG_READ | PROP_FLAG_WRITE);
    // if (param) {
    //     esp_rmaker_param_add_ui_type(param, ESP_RMAKER_UI_SLIDER);
    //     esp_rmaker_param_add_bounds(param, esp_rmaker_int(0), esp_rmaker_int(100), esp_rmaker_int(1));
    // }
    // esp_rmaker_device_add_param(fan_device, param);
    // esp_rmaker_node_add_device(node, fan_device);
    return ESP_OK;
}

void app_main()
{
    /* Initialize NVS. */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    /* Initialize Application specific hardware drivers and
     * set initial state.
     */
    // esp_rmaker_console_init();
    app_driver_init();

    /** Determine whether to restore the settings by reading the restart count */
    int restart_cnt = restart_count_get();
    ESP_LOGW(TAG, "Restart count=[%d]", restart_cnt);


    /* Initialize Wi-Fi/Thread. Note that, this should be called before esp_rmaker_node_init()
     */
    ws2812_set_hue(311);
    app_network_init();

    /* Register an event handler to catch RainMaker events */
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_COMMON_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(APP_WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(RMAKER_OTA_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(NETWORK_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    /* Initialize the ESP RainMaker Agent.
     * Note that this should be called after app_network_init() but before app_network_start()
     * */
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    esp_rmaker_node_t *node = esp_rmaker_node_init(&rainmaker_cfg, "Ferris Wheel", "Lightbulb");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }

    add_device(node);

    /* Enable OTA */
    esp_rmaker_ota_enable_default();

    /* Enable timezone service which will be require for setting appropriate timezone
     * from the phone apps for scheduling to work correctly.
     * For more information on the various ways of setting timezone, please check
     * https://rainmaker.espressif.com/docs/time-service.html.
     */
    esp_rmaker_timezone_service_enable();

    /* Enable scheduling. */
    esp_rmaker_schedule_enable();

    /* Enable Scenes */
    esp_rmaker_scenes_enable();

    /* Enable Insights. Requires CONFIG_ESP_INSIGHTS_ENABLED=y */
    // app_insights_enable();


    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();

    if (restart_cnt >= RESTART_COUNT_RESET) {
        ESP_LOGW(TAG, "Erase information saved in flash and restart");
        wifi_reset_trigger(NULL);
        vTaskDelay(portMAX_DELAY);
    }

    // err = app_network_set_custom_mfg_data(MGF_DATA_DEVICE_TYPE_LIGHT, MFG_DATA_DEVICE_SUBTYPE_LIGHT);
    /* Start the Wi-Fi/Thread.
     * If the node is provisioned, it will start connection attempts,
     * else, it will start Wi-Fi provisioning. The function will return
     * after a connection has been successfully established
     */
    err = app_network_start(POP_TYPE_RANDOM);
    ws2812_set_hue(250);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Could not start network. Aborting!!!");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
        abort();
    }
}

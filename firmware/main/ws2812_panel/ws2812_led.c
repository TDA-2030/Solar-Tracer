/*  WS2812 RGB LED helper functions

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* It is recommended to copy this code in your example so that you can modify as
 * per your application's needs.
 */


#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <esp_log.h>
#include "driver/rmt_tx.h"
#include "led_strip_encoder.h"
#include "led_loc_map.h"
#include "ws2812_led.h"
#include "esp_random.h"
#include "paint.h"
#include "../helper.h"

#define RMT_LED_STRIP_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_LED_STRIP_GPIO_NUM      8
#define EXAMPLE_LED_NUMBERS         208

#define EFFECT_TEXT_NAME_SPACE "ws2812"
#define EFFECT_TEXT_KEY "Text"

static const char *TAG = "ws2812";



typedef struct {
    uint8_t *led_strip_pixels;
    rmt_channel_handle_t led_chan;
    rmt_encoder_handle_t led_encoder;
    rmt_transmit_config_t tx_config;
    QueueHandle_t mode_Queue;
    bool power;
    LightMode mode;
    uint16_t brightness;
    uint16_t hue;
    uint16_t saturation;
    epd_paint_t *paint;
    char text[128];
} ws2812_panel_t;

static ws2812_panel_t g_panel;
static TaskHandle_t play_task_handle = NULL;

static void effect_task(void *args);

static void _rgb2hsv(uint8_t r, uint8_t g, uint8_t b, uint32_t *h, uint32_t *s, uint32_t *v)
{

    // uint16_t h, s, v;

    uint8_t minRGB, maxRGB;
    uint8_t delta;

    minRGB = r < g ? (r < b ? r : b) : (g < b ? g : b);
    maxRGB = r > g ? (r > b ? r : b) : (g > b ? g : b);

    *v = maxRGB * 100 / 255;
    delta = maxRGB - minRGB;

    if (delta == 0) {
        *h = 0;
        *s = 0;
    } else {
        *s = delta * 100 / maxRGB;

        if (r == maxRGB) {
            *h = (60 * (g - b) / delta + 360) % 360;
        } else if (g == maxRGB) {
            *h = (60 * (b - r) / delta + 120);
        } else {
            *h = (60 * (r - g) / delta + 240);
        }
    }
}

/**
 * @brief Simple helper function, converting HSV color space to RGB color space
 *
 * Wiki: https://en.wikipedia.org/wiki/HSL_and_HSV
 *
 */
static void _hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

int32_t _random_range(int32_t min, int32_t max)
{
    if (min >= max) {
        // 如果最小值不小于最大值，返回一个错误值或者可以处理这种情况
        ESP_LOGE(TAG, "Invalid range for %s", __func__);
        return -1; // 或者其他错误处理
    }

    // 计算范围的跨度
    int32_t range = max - min + 1;
    uint32_t rand_value = esp_random() / (0xffffffff / range);

    // 将生成的随机数转换为指定范围内的值
    return min + (rand_value);
}

static void get_fade_color(uint8_t *new_r, uint8_t *new_g, uint8_t *new_b)
{
    static uint8_t r;
    static uint8_t g;
    static uint8_t b;

    uint32_t h, s, v;
    _rgb2hsv(r, g, b, &h, &s, &v);

    uint32_t new_h = (h + (esp_random() >> 28)) % 360; // 确保色相相差较大

    _hsv2rgb(new_h, s, v, new_r, new_g, new_b);
}

esp_err_t ws2812_led_init(void)
{

    g_panel.led_strip_pixels = (uint8_t *) calloc(EXAMPLE_LED_NUMBERS * 3, sizeof(uint8_t));
    if (g_panel.led_strip_pixels == NULL) {
        ESP_LOGE(TAG, "%s calloc failed", __func__);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Create RMT TX channel");
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select source clock
        .gpio_num = RMT_LED_STRIP_GPIO_NUM,
        .mem_block_symbols = 64, // increase the block size can make the LED less flickering
        .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
        .trans_queue_depth = 4, // set the number of transactions that can be pending in the background
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &g_panel.led_chan));

    ESP_LOGI(TAG, "Install led strip encoder");
    led_strip_encoder_config_t encoder_config = {
        .resolution = RMT_LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config, &g_panel.led_encoder));

    ESP_LOGI(TAG, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(g_panel.led_chan));

    g_panel.brightness = 5;
    g_panel.hue = 180;
    g_panel.saturation = 100;
    g_panel.power = true;
    g_panel.mode = LIGHT_MODE_NORMAL;

    ws2812_set_power(g_panel.power);
    g_panel.paint = Paint_create(LED_WIDTH, LED_HEIGHT);
    if (g_panel.paint == NULL) {
        ESP_LOGE(TAG, "Paint_create failed");
    }

    esp_err_t ret = iot_param_load(EFFECT_TEXT_NAME_SPACE, EFFECT_TEXT_KEY, &g_panel.text);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "iot_param_load failed, set default effect text");
        strncpy(g_panel.text, "Hello World!", sizeof(g_panel.text));
        iot_param_save(EFFECT_TEXT_NAME_SPACE, EFFECT_TEXT_KEY, &g_panel.text, sizeof(g_panel.text));
    }

    g_panel.mode_Queue = xQueueCreate(4, sizeof(LightMode));
    xTaskCreate(effect_task, "effect_task", 4096, NULL, 5, NULL);

    void lv_port(int width, int height);
    lv_port(LED_WIDTH, LED_HEIGHT);

    return ESP_OK;
}

void ws2812_set_power(bool power)
{
    g_panel.power = power;
    if (power) {
        ws2812_led_set_hsv(g_panel.hue, g_panel.saturation, g_panel.brightness);
    } else {
        ws2812_led_clear();
    }
}

void ws2812_set_brightness(uint8_t brightness)
{
    g_panel.brightness = brightness;
    ws2812_led_set_hsv(g_panel.hue, g_panel.saturation, g_panel.brightness);
}

void ws2812_set_hue(uint16_t hue)
{
    g_panel.hue = hue;
    ws2812_led_set_hsv(g_panel.hue, g_panel.saturation, g_panel.brightness);
}

void ws2812_set_saturation(uint8_t saturation)
{
    g_panel.saturation = saturation;
    ws2812_led_set_hsv(g_panel.hue, g_panel.saturation, g_panel.brightness);
}

bool ws2812_get_power(void)
{
    return g_panel.power;
}

uint8_t ws2812_get_brightness(void)
{
    return g_panel.brightness;
}

uint8_t ws2812_get_hue(void)
{
    return g_panel.hue;
}

uint8_t ws2812_get_saturation(void)
{
    return g_panel.saturation;
}


void ws2812_write_pixel(uint16_t idx, color32_t color)
{
    if (idx >= EXAMPLE_LED_NUMBERS) {
        ESP_LOGE(TAG, "Error: idx(%d) out of range\n", idx);
    }

    g_panel.led_strip_pixels[idx * 3 + 0] = color.ch.green;
    g_panel.led_strip_pixels[idx * 3 + 1] = color.ch.red;
    g_panel.led_strip_pixels[idx * 3 + 2] = color.ch.blue;
}

void ws2812_write_pixel_xy(uint16_t _x, uint16_t _y, color32_t color)
{
    uint8_t idx = led_loc_map[_x][_y];
    if (idx > 0) {
        ws2812_write_pixel(idx - 1, color);
    }
}

esp_err_t ws2812_refresh(void)
{
    ESP_ERROR_CHECK(rmt_transmit(g_panel.led_chan, g_panel.led_encoder, g_panel.led_strip_pixels, EXAMPLE_LED_NUMBERS * 3, &g_panel.tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(g_panel.led_chan, portMAX_DELAY));
    return ESP_OK;
}

esp_err_t ws2812_led_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    g_panel.power = true;
    for (size_t i = 0; i < EXAMPLE_LED_NUMBERS; i++) {
        color32_t c = {.ch.red = red, .ch.green = green, .ch.blue = blue};
        ws2812_write_pixel(i, c);
    }
    ws2812_refresh();

    return ESP_OK;
}

esp_err_t ws2812_led_set_hsv(uint32_t hue, uint32_t saturation, uint32_t value)
{
    uint8_t red, green, blue;
    g_panel.power = true;
    _hsv2rgb(hue, saturation, value, &red, &green, &blue);
    ws2812_led_set_rgb(red, green, blue);
    return ESP_OK;
}

esp_err_t ws2812_led_clear(void)
{
    ws2812_led_set_rgb(0, 0, 0);
    return ESP_OK;
}

void ws2812_write_line(int16_t angle, int16_t r, color32_t color)
{
    if (r > 9 || r < 0) {
        return;
    }
    if (angle < 0) {
        angle += 16;
    }
    angle = angle % 16;
    ws2812_write_pixel(led_line_map[angle][r] - 1, color);
}

void ws2812_write_circle1(int16_t angle, color32_t color)
{
    if (angle < 0) {
        angle += 8;
    }
    angle = angle % 8;
    ws2812_write_pixel(led_circle1_map[angle] - 1, color);
}

void ws2812_write_circle2(int16_t angle, color32_t color)
{
    if (angle < 0) {
        angle += 32;
    }
    angle = angle % 32;
    ws2812_write_pixel(led_circle2_map[angle] - 1, color);
}

static void generate_gradient(color32_t color, uint8_t steps, color32_t *gradient)
{

    for (size_t i = 0; i < steps; i++) {
        // 计算当前步长的颜色值
        gradient[i].ch.red = (uint8_t)(color.ch.red * (steps - i) / steps);
        gradient[i].ch.green = (uint8_t)(color.ch.green * (steps - i) / steps);
        gradient[i].ch.blue = (uint8_t)(color.ch.blue * (steps - i) / steps);
    }
}

static int effect_delay(uint32_t ms)
{
    // 从队列接收数据
    if (xQueueReceive(g_panel.mode_Queue, &g_panel.mode, pdMS_TO_TICKS(ms)) == pdPASS) {
        // 处理接收到的数据
        // printf("Received light mode: %d\n", g_panel.mode);
        return g_panel.mode;
    } else {
        return -1;
    }
}

static int _effect_spiral(int (*delay)(uint32_t ms))
{
    printf("%s\n", __func__);
    // # 生成渐变色数组
    color32_t color_gradients[9];
    color32_t color = { .full = 0 };
    color32_t dark_color = { .full = 0 };
    _hsv2rgb(0, 0, 40, &color.ch.red, &color.ch.green, &color.ch.blue);
    generate_gradient(color, 9, color_gradients);
    uint32_t a = 0;
    float line_pos[16] = {0.0};
    uint16_t line_bits[16] = {0};
    ws2812_led_clear();
    while (1) {
        ws2812_write_circle2(a - 9, dark_color);
        for (int i = 0; i < 9; i++) {
            ws2812_write_circle2(a - i, color_gradients[i]);
        }

        if (0 == a % 2) {
            uint8_t _ang = a / 2;
            line_bits[_ang] |= 1 << (9 + 2);
            for (int i = 0; i < 16; i++) {
                line_pos[i] += 0.5;
                if (line_pos[i] > 1.0) {
                    line_bits[i] >>= 1;
                    line_pos[i] -= 1.0;
                    for (int _b = 0; _b < 12; _b++) {
                        if (1 << _b & line_bits[i]) {
                            ws2812_write_line(i, _b, dark_color);
                            ws2812_write_line(i, _b - 1, color_gradients[7]);
                            ws2812_write_line(i, _b - 2, color_gradients[0]);
                        }
                    }
                }
            }
        }
        ws2812_refresh();
        int mode = delay(60);
        if (mode >= 0) {
            return mode;
        }

        a += 1;
        if (a >= 32) {
            a = 0;
        }
    }
}

static int _effect_breath(int (*delay)(uint32_t ms))
{
    printf("%s\n", __func__);
    int8_t dir = 1;
    ws2812_led_clear();
    uint32_t h = _random_range(0, 360), s = 90, v = 25;
    while (1) {

        ws2812_led_set_hsv(h, s, v);

        v += dir;
        if (v > 30 || v <= 1) {
            dir *= -1;
            if (v <= 3) {
                h = (h + _random_range(25, 200)) % 360; // 确保色相相差较大
            }
        }

        ws2812_refresh();
        int mode = delay(30);
        if (mode >= 0) {
            return mode;
        }
    }
}

static int _effect_rainbow(int (*delay)(uint32_t ms))
{
    printf("%s\n", __func__);
    uint32_t cnt = 0;
    ws2812_led_clear();
    uint32_t h = 0, s = 90, v = 25;
    while (1) {

        for (size_t i = 0; i < 32; i++) {
            h = (cnt + i) * 360 / 32;
            h = h % 360;
            color32_t color;
            _hsv2rgb(h, s, v, &color.ch.red, &color.ch.green, &color.ch.blue);
            ws2812_write_circle2(i, color);
        }

        cnt ++;
        if (cnt >= 32) {
            cnt = 0;
        }

        ws2812_refresh();
        int mode = delay(40);
        if (mode >= 0) {
            return mode;
        }
    }
}

static int _effect_wipe(int (*delay)(uint32_t ms))
{
    printf("%s\n", __func__);
    epd_paint_t *paint = g_panel.paint;
    ws2812_led_clear();
    uint32_t h = _random_range(0, 360), s = 100, v = 15;
    while (1) {
        int cx = LED_WIDTH / 2, cy = LED_HEIGHT / 2;
        int x = _random_range(cx - 20, cx + 20);
        int y = _random_range(cy - 20, cy + 20);
        int max_r = (LED_HEIGHT / 2);
        for (int i = 0; i < max_r; i++) {
            _hsv2rgb(h, s, v, &paint->color.ch.red, &paint->color.ch.green, &paint->color.ch.blue);
            Paint_DrawFilledCircle(paint, x, y, i);

            ws2812_refresh();
            int mode = delay(15);
            if (mode >= 0) {
                return mode;
            }
        }

        h = (h + _random_range(25, 100)) % 360; // 确保色相相差较大

    }
}

void lvgl_page_deinit(void);
void lvgl_scroll_text_init(const char *text);
void lvgl_jump_effect_init(void);
void lvgl_task_exec(void);


static int _effect_jump(int (*delay)(uint32_t ms))
{
    printf("%s\n", __func__);
    ws2812_led_clear();
    lvgl_jump_effect_init();

    while (1) {

        lvgl_task_exec();

        int mode = delay(30);
        if (mode >= 0) {
            lvgl_page_deinit();
            return mode;
        }
    }
}

static int _effect_shift(int (*delay)(uint32_t ms))
{
    printf("%s\n", __func__);
    ws2812_led_clear();
    lvgl_scroll_text_init(g_panel.text);
    while (1) {

        lvgl_task_exec();

        int mode = delay(30);
        if (mode >= 0) {
            lvgl_page_deinit();
            return mode;
        }

    }
}

static void effect_task(void *args)
{
    while (1) {
        switch (g_panel.mode) {
        case LIGHT_MODE_NORMAL:
            ws2812_set_power(g_panel.power);
            effect_delay(portMAX_DELAY);
            break;

        case LIGHT_MODE_RAINBOW:
            _effect_rainbow(effect_delay);
            break;

        case LIGHT_MODE_BREATH:
            _effect_breath(effect_delay);
            break;

        case LIGHT_MODE_SPIRAL:
            _effect_spiral(effect_delay);
            break;

        case LIGHT_MODE_WIPE:
            _effect_wipe(effect_delay);
            break;

        case LIGHT_MODE_JUMP:
            _effect_jump(effect_delay);
            break;

        case LIGHT_MODE_SHIFT:
            _effect_shift(effect_delay);
            break;

        default:
            effect_delay(portMAX_DELAY);
            break;
        }
    }
}


void ws2812_effect_set_mode(LightMode mode)
{
    if (mode == g_panel.mode) {
        return;
    }

    // 发送数据到队列
    if (xQueueSend(g_panel.mode_Queue, &mode, portMAX_DELAY) != pdPASS) {
        printf("Error sending item to queue.\n");
    }
    g_panel.mode = mode;
}

static const char *mode_name[] = {
    // 顺序必须与 LightMode 枚举值一致
    "normal",
    "rainbow",
    "breath",
    "spiral",
    "wipe",
    "jump",
    "shift",
    "play",
};

const char **ws2812_effect_get_mode_name_array(uint8_t *length)
{
    *length = sizeof(mode_name) / sizeof(mode_name[0]);
    return mode_name;
}

void ws2812_effect_set_mode_name(const char *mode)
{
    // printf("set mode name %s\n", mode);
    if (strcmp(mode, "play") == 0) {
        ws2812_play_mode_enable(1, 0, 0);
        return;
    } else {
        ws2812_play_mode_enable(0, 0, 0);
    }

    uint16_t num = sizeof(mode_name) / sizeof(char *);
    size_t i;
    for (i = 0; i < num; i++) {
        if (strcmp(mode, mode_name[i]) == 0) {
            ws2812_effect_set_mode(i);
            break;
        }
    }

    if (i == num) {
        ESP_LOGE(TAG, "Invalid mode: %s", mode);
    }
}

const char *ws2812_effect_get_mode_name()
{
    return mode_name[g_panel.mode];
}


LightMode ws2812_effect_get_mode(void)
{
    return g_panel.mode;
}

void ws2812_effect_set_text(const char *text)
{
    strncpy(g_panel.text, text, sizeof(g_panel.text));
    iot_param_save(EFFECT_TEXT_NAME_SPACE, EFFECT_TEXT_KEY, &g_panel.text, sizeof(g_panel.text));
}

const char *ws2812_effect_get_text(void)
{
    return g_panel.text;
}

void ws2812_play_mode_enable(bool enable, uint32_t start_delay, bool exit_to_normal_mode)
{
    eTaskState task_state;
    if (NULL == play_task_handle) {
        task_state = eInvalid;
    } else {
        task_state = eTaskGetState(play_task_handle);
    }

    printf("play task state: %d, enable=%d\n", task_state, enable);
    if (enable) {
        if (eDeleted <= task_state) {
            ESP_LOGI(TAG, "play task create");
            xTaskCreate(ws2812_play_run_task, "offline_task", 4096, (void *)start_delay, 1, &play_task_handle);
        }
    } else {
        if (eDeleted > task_state) { // created
            ESP_LOGI(TAG, "Notify the play task");
            xTaskNotify(play_task_handle, 1 + (uint32_t)exit_to_normal_mode, eSetValueWithOverwrite);
            vTaskDelay(pdMS_TO_TICKS(10));
            ESP_LOGI(TAG, "Notify the play task ok");
        }
    }
}

void ws2812_play_run_task(void *args)
{
    uint32_t start_delay = (uint32_t)args;
    ESP_LOGI(TAG, "Play task start, delay=%u", start_delay);
    LightMode m = LIGHT_MODE_NORMAL + 1;
    vTaskDelay(pdMS_TO_TICKS(start_delay));
    ws2812_effect_set_mode(m);
    uint32_t ulReceivedValue;
    while (1) {
        ulReceivedValue = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(30000));
        printf("offline task received %d\n", ulReceivedValue);
        if (ulReceivedValue > 0) {
            ESP_LOGI(TAG, "play task exit");
            break;
        }
        ESP_LOGI(TAG, "play run task switch mode %d", m);
        m++;
        if (m >= LIGHT_MODE_MAX) {
            m = LIGHT_MODE_NORMAL + 1;
        }
        ws2812_effect_set_mode(m);

    }
    if (ulReceivedValue == 2) {
        ws2812_effect_set_mode(LIGHT_MODE_NORMAL);
    }
    play_task_handle = NULL;
    vTaskDelete(NULL);
}


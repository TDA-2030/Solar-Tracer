
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ws2812_led.h"


#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2


static SemaphoreHandle_t lvgl_mux = NULL;
static lv_disp_drv_t disp_drv;      // contains callback functions

static const char *TAG = "lv_port";

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

bool example_lvgl_lock(int timeout_ms)
{
    return 1;
    // Convert timeout in milliseconds to FreeRTOS ticks
    // If `timeout_ms` is set to -1, the program will block until the condition is met
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(lvgl_mux, timeout_ticks) == pdTRUE;
}

void example_lvgl_unlock(void)
{
    // xSemaphoreGiveRecursive(lvgl_mux);
}

static void _flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    // printf("flush area %d,%d %d,%d\n", area->x1, area->y1, area->x2, area->y2);

    int32_t x;
    int32_t y;
    for (y = area->y1; y <= area->y2; y++) {
        for (x = area->x1; x <= area->x2; x++) {
            color32_t cc = {.full = lv_color_to32(*color_p)};
            ws2812_write_pixel_xy(x, y, cc);
            color_p++;
        }
    }
    if (lv_disp_flush_is_last(disp_drv)) {
        ws2812_refresh();
    }
    lv_disp_flush_ready(disp_drv);
}


// static void example_lvgl_port_task(void *arg)
// {
//     ESP_LOGI(TAG, "Starting LVGL task");
//     uint32_t task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
//     while (1) {
//         // Lock the mutex due to the LVGL APIs are not thread-safe
//         if (example_lvgl_lock(-1)) {
//             task_delay_ms = lv_timer_handler();
//             // Release the mutex
//             example_lvgl_unlock();
//         }
//         if (task_delay_ms > EXAMPLE_LVGL_TASK_MAX_DELAY_MS) {
//             task_delay_ms = EXAMPLE_LVGL_TASK_MAX_DELAY_MS;
//         } else if (task_delay_ms < EXAMPLE_LVGL_TASK_MIN_DELAY_MS) {
//             task_delay_ms = EXAMPLE_LVGL_TASK_MIN_DELAY_MS;
//         }
//         vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
//     }
// }

void lvgl_task_exec(void)
{
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1)) {
        lv_timer_handler();
        // Release the mutex
        example_lvgl_unlock();
    }
}

static lv_obj_t *circle[5];
static lv_obj_t *main_page;

static lv_obj_t *main_page_create(void)
{
    main_page = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_page, LV_PCT(100), LV_PCT(100));
    lv_obj_center(main_page);
    lv_obj_clear_flag(main_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(main_page, lv_color_make(2, 2, 2), LV_PART_MAIN);
    return main_page;
}

void lvgl_scroll_text_init(const char *text)
{
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1)) {
        main_page = main_page_create();
        LV_FONT_DECLARE(font_arial_70);

        static lv_style_t style;
        lv_style_init(&style);

        /*Set a background color and a radius*/
        lv_style_set_radius(&style, 0);
        lv_style_set_bg_opa(&style, LV_OPA_0);
        // lv_style_set_bg_color(&style, lv_color_darken(lv_palette_main(LV_PALETTE_GREEN), 220));
        lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);

        // lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_BLUE));
        lv_style_set_border_width(&style, 0);
        lv_style_set_text_color(&style, lv_color_lighten(lv_palette_main(LV_PALETTE_RED), 20));
        lv_style_set_text_font(&style, &font_arial_70);

        lv_obj_t *label = lv_label_create(main_page);
        lv_obj_set_size(label, 100, 100);
        lv_obj_add_style(label, &style, LV_STATE_DEFAULT);
        lv_label_set_text_fmt(label, " %s", text);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_style_anim_speed(label, 20, LV_STATE_DEFAULT);
        lv_obj_center(label);

        lv_obj_set_style_pad_top(label, 17, LV_PART_MAIN);
        // Release the mutex
        example_lvgl_unlock();
    }
}


static void _anim_exec_xcb(void *var, int32_t value)
{
    uint32_t r[5] = {10, 40, 70};
    for (size_t i = 0; i < 3; i++) {
        uint32_t rr = (r[i] + value) % (disp_drv.hor_res + 10);
        lv_obj_set_size(circle[i], rr, rr);

        if (value == 100) {
            lv_color_t cc = lv_color_hsv_to_rgb(_random_range(30, 360), 100, 30);
            lv_obj_set_style_arc_color(circle[i], cc, LV_PART_MAIN);
            lv_obj_set_style_arc_color(circle[i], cc, LV_PART_INDICATOR);
        }
    }
}

void lvgl_jump_effect_init(void)
{
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (example_lvgl_lock(-1)) {
        main_page = main_page_create();

        static lv_style_t style;
        lv_style_init(&style);
        lv_style_set_arc_width(&style, 5);
        lv_style_set_size(&style, 100);
        lv_style_set_arc_color(&style, lv_color_darken(lv_palette_main(LV_PALETTE_BLUE), 200));

        for (size_t i = 0; i < 3; i++) {
            circle[i] = lv_arc_create(main_page);
            lv_arc_set_angles(circle[i], 0, 360);
            lv_obj_add_style(circle[i], &style, LV_PART_MAIN);  // 将样式应用到圆弧背景
            lv_obj_add_style(circle[i], &style, LV_PART_INDICATOR);  // 将样式应用到圆弧前景
            lv_obj_remove_style(circle[i], NULL, LV_PART_KNOB);
            lv_obj_center(circle[i]);
        }

        /* 初始化动画结构体 */
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_values(&a, 0, 100); // 设置动画的开始和结束值，即圆环的最小和最大半径
        lv_anim_set_time(&a, 2000); // 设置动画的持续时间为2000毫秒
        lv_anim_set_path_cb(&a, lv_anim_path_linear);
        lv_anim_set_exec_cb(&a, _anim_exec_xcb); // 更新圆环的半径
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_start(&a);
// Release the mutex
        example_lvgl_unlock();
    }
}

#include "esp_heap_caps.h"
void lvgl_page_deinit(void)
{
    if (example_lvgl_lock(-1)) {
        lv_anim_del(NULL, NULL);
        lv_obj_del(main_page);
        main_page = NULL;
        lvgl_task_exec();
        example_lvgl_unlock();
    }
    printf("free=%d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
}


void lv_port(int width, int height)
{
    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    void *buf1 = NULL;
    void *buf2 = NULL;
    size_t buffer_size = width * 20;

    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers from PSRAM");
    buf1 = heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DEFAULT);
    assert(buf1);
    printf("lv_port buf=%p\n", buf1);

    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, buffer_size);

    ESP_LOGI(TAG, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = width;
    disp_drv.ver_res = height;
    disp_drv.flush_cb = _flush;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.full_refresh = 1;
    disp_drv.user_data = NULL;

    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    lvgl_mux = xSemaphoreCreateRecursiveMutex();
    assert(lvgl_mux);
    // ESP_LOGI(TAG, "Create LVGL task");
    // xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);


    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_make(2, 2, 2), LV_PART_MAIN);
    


}





/* LVGL Example project */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"

// Littlevgl 头文件
#include "lvgl/lvgl.h"          // LVGL头文件
#include "lvgl_helpers.h"       // 助手 硬件驱动相关
#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"

// 声明LVGL图片资源（在lv_examples/lv_examples/assets文件夹中）
LV_IMG_DECLARE(img_cogwheel_argb);

#define SLIDER_WIDTH 20		//Slider 滑块控件宽度

// Slider 滑块控件回调函数
static void slider_event_cb(lv_obj_t * slider, lv_event_t event);
// Sliderr 红、绿、蓝、对比度滑块控件声明，声明在外部回调中需要调用 
static lv_obj_t * red_slider, * green_slider, * blue_slider, * intense_slider;
// img 图像控件声明，声明在外部回调中需要调用 
static lv_obj_t * img1;





#define TAG " LittlevGL Demo"
#define LV_TICK_PERIOD_MS 10
SemaphoreHandle_t xGuiSemaphore;		// 创建一个GUI信号量

static void lv_tick_task(void *arg);	// LVGL 时钟任务
void guiTask(void *pvParameter);		// GUI任务

// LVGL 时钟任务
static void lv_tick_task(void *arg) {
	(void) arg;
	lv_tick_inc(LV_TICK_PERIOD_MS);
}
// Slider 滑块控件回调函数
static void slider_event_cb(lv_obj_t * slider, lv_event_t event)
{
	if(event == LV_EVENT_VALUE_CHANGED) {
		// 根据每个滑块的值创建一个颜色，重新给IMG图片着色
		lv_color_t color  = lv_color_make(lv_slider_get_value(red_slider), lv_slider_get_value(green_slider), lv_slider_get_value(blue_slider));
		lv_opa_t intense = lv_slider_get_value(intense_slider);// 单独获取对比度滑块值
		lv_obj_set_style_local_image_recolor_opa(img1, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, intense);// 设置图片透明度
		lv_obj_set_style_local_image_recolor(img1, LV_IMG_PART_MAIN, LV_STATE_DEFAULT, color);// 重新给图片上色
	}
}


void guiTask(void *pvParameter) {
	
	(void) pvParameter;
	xGuiSemaphore = xSemaphoreCreateMutex();	// 创建GUI信号量
	lv_init();									// 初始化LittlevGL
	lvgl_driver_init();							// 初始化液晶SPI驱动 触摸芯片SPI/IIC驱动

	// 初始化缓存
	static lv_color_t buf1[DISP_BUF_SIZE];
	static lv_color_t buf2[DISP_BUF_SIZE];
	static lv_disp_buf_t disp_buf;
	uint32_t size_in_px = DISP_BUF_SIZE;
	lv_disp_buf_init(&disp_buf, buf1, buf2, size_in_px);

	// 添加并注册触摸驱动
	lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.flush_cb = disp_driver_flush;
	disp_drv.buffer = &disp_buf;
	lv_disp_drv_register(&disp_drv);

	// 添加并注册触摸驱动
	ESP_LOGI(TAG,"Add Register Touch Drv");
	lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.read_cb = touch_driver_read;
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	lv_indev_drv_register(&indev_drv);

	// 定期处理GUI回调
	const esp_timer_create_args_t periodic_timer_args = {
		.callback = &lv_tick_task,
		.name = "periodic_gui"
	};
	esp_timer_handle_t periodic_timer;
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));




	///////////////////////////////////////////////////
	///////////////Gauge1 仪表指示控件//////////////////
	///////////////////////////////////////////////////
	// 创建红绿蓝三个滑块控件
	// 绿蓝对比度滑块都以继承红色方块参数创建
	red_slider = lv_slider_create(lv_scr_act(), NULL);		// 创建一个滑块
	lv_slider_set_range(red_slider, 0, 255);				// 滑块值范围0-255，设置为颜色的值范围
	lv_obj_set_size(red_slider, SLIDER_WIDTH, 200); 		// 20宽，200高，垂直滑块
	// 设置本地风格，滑块背景色为红色
	lv_obj_set_style_local_bg_color(red_slider, LV_SLIDER_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_RED);
	lv_obj_set_event_cb(red_slider, slider_event_cb);		// 设置回调函数
	
	//lv_slider_create使用copy方式创建，后面的滑块会继承红色滑块的设置参数
	green_slider = lv_slider_create(lv_scr_act(), red_slider);
	lv_obj_set_style_local_bg_color(green_slider, LV_SLIDER_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_LIME);

	blue_slider = lv_slider_create(lv_scr_act(), red_slider);
	lv_obj_set_style_local_bg_color(blue_slider, LV_SLIDER_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_BLUE);

	intense_slider = lv_slider_create(lv_scr_act(), red_slider);
	lv_obj_set_style_local_bg_color(intense_slider, LV_SLIDER_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_GRAY);
	lv_slider_set_value(intense_slider, 255, LV_ANIM_OFF);

	// 使用相对对齐方式对齐四个滑块，每个滑块差20
	lv_obj_align(red_slider, NULL, LV_ALIGN_IN_LEFT_MID, 20, 0);
	lv_obj_align(green_slider, red_slider, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
	lv_obj_align(blue_slider, green_slider, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
	lv_obj_align(intense_slider, blue_slider, LV_ALIGN_OUT_RIGHT_MID, 20, 0);

	// 创建一个图片控件
	img1 = lv_img_create(lv_scr_act(), NULL);
	lv_img_set_src(img1, &img_cogwheel_argb);					// 加载图像数据
	lv_obj_align(img1, NULL, LV_ALIGN_IN_RIGHT_MID, -20, 0);	// 对齐

	lv_obj_t * img2 = lv_img_create(lv_scr_act(), NULL);		// 再创建一个图片控件用来演示FontAwesome字体图标
	lv_img_set_src(img2, LV_SYMBOL_OK "Accept");				// FontAwesome 字体图标"对勾"
	lv_obj_align(img2, img1, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);	// 对齐


//	lv_demo_widgets();
	
	while (1) {
		vTaskDelay(1);
		// 尝试锁定信号量，如果成功，调用处理LVGL任务
		if (xSemaphoreTake(xGuiSemaphore, (TickType_t)10) == pdTRUE) {
			lv_task_handler();					// 处理LVGL任务
			xSemaphoreGive(xGuiSemaphore);		// 释放信号量
		}
	}
	vTaskDelete(NULL);							// 删除任务
}

// 主函数
void app_main() {
	ESP_LOGI(TAG, "\r\nAPP is start!~\r\n");
	
	// 如果要使用任务创建图形，则需要创建固定任务,否则可能会出现诸如内存损坏等问题
	xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);
}





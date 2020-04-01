#include "talking_clock.h"
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_wifi.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "fatfs_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "esp_peripherals.h"
#include "periph_sdcard.h"
#include "periph_touch.h"
#include "periph_button.h"
#include "periph_wifi.h"
#include "wifi.h"
#include "smbus.h"
#include "i2c-lcd1602.h"
#include "sntp_sync.h"
#include <stdbool.h>
#include <sys/time.h>
#include "input_key_service.h"

//OLED screen
#include "ssd1306.h"
#include "ssd1306_draw.h"
#include "ssd1306_font.h"
#include "ssd1306_default_if.h"

#include "leds.h"

//Draw methods for SSD1306
void SetupDemo(struct SSD1306_Device *DisplayHandle, const struct SSD1306_FontDef *Font);
void SayHello(struct SSD1306_Device *DisplayHandle, const char *HelloText);
//SSD1306
struct SSD1306_Device SSD1306Display;

static const char *TAG = "TALKING_CLOCK";

audio_pipeline_handle_t pipeline;
audio_element_handle_t i2s_stream_writer, mp3_decoder, fatfs_stream_reader;
TimerHandle_t timer_1_sec;
TaskHandle_t taskHandle;

//SSD1306 OLED
#define SSD1306DisplayAddress           0x3C
#define SSD1306DisplayWidth               128
#define SSD1306DisplayHeight              32
#define SSD1306ResetPin                      -1

//SSD1306 draw methods
bool DefaultBusInit(void)
{
    assert(SSD1306_I2CMasterInitDefault() == true);
    assert(SSD1306_I2CMasterAttachDisplayDefault(&SSD1306Display, SSD1306DisplayWidth, SSD1306DisplayHeight, SSD1306DisplayAddress, SSD1306ResetPin) == true);
    return true;
}

void SSD1306Setup(struct SSD1306_Device *DisplayHandle, const struct SSD1306_FontDef *Font)
{
    SSD1306_Clear(DisplayHandle, SSD_COLOR_BLACK);
    SSD1306_SetFont(DisplayHandle, Font);
}

void SSD1306Draw(struct SSD1306_Device *DisplayHandle, const char *HelloText)
{
    SSD1306_FontDrawAnchoredString(DisplayHandle, TextAnchor_Center, HelloText, SSD_COLOR_WHITE);
    SSD1306_Update(DisplayHandle);
}
//end SSD1306 draw methods


void stmp_timesync_event(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");

	time_t now;
    struct tm timeinfo;
    time(&now);

	char strftime_buf[64];
	localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Amsterdam is: %s", strftime_buf);
	
	talking_clock_fill_queue();
}

void timer_1_sec_callback( TimerHandle_t xTimer )
{ 
	// Print current time to the screen
	time_t now;
    struct tm timeinfo;
    time(&now);

	char strftime_buf[20];
	localtime_r(&now, &timeinfo);
	sprintf(&strftime_buf[0], "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	//  if (DefaultBusInit() == true)
    //  {
    //      SSD1306Setup(&SSD1306Display, &Font_droid_sans_fallback_24x28);
    //      SSD1306Draw(&SSD1306Display, strftime_buf);
    //  }

    ESP_LOGI(TAG, "The current date/time in Amsterdam is: %s", strftime_buf);
	// Say the time every hour
	if (timeinfo.tm_sec == 0 && timeinfo.tm_min == 0) 
	{
		talking_clock_fill_queue();
		audio_element_set_uri(fatfs_stream_reader, talking_clock_files[TALKING_CLOCK_ITSNOW_INDEX]); // Set first sample
		audio_pipeline_reset_ringbuffer(pipeline);
		audio_pipeline_reset_elements(pipeline);
		audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
		audio_pipeline_run(pipeline);
	}	
}

esp_err_t talking_clock_init() 
{
	// Initialize queue
	ESP_LOGI(TAG, "Creating FreeRTOS queue for talking clock");
	talking_clock_queue = xQueueCreate( 10, sizeof( int ) );

	if (talking_clock_queue == NULL) 
	{
		ESP_LOGE(TAG, "Error creating queue");
		return ESP_FAIL;
	}
	return ESP_OK;
}

esp_err_t talking_clock_fill_queue() 
{
	time_t now;
    struct tm timeinfo;
    time(&now);

	char strftime_buf[64];
	localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Amsterdam is: %s", strftime_buf);

	// Reset queue
	esp_err_t ret = xQueueReset(talking_clock_queue);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Cannot reset queue");
	}

	int data = TALKING_CLOCK_ITSNOW_INDEX;
	// Fill queue
	//xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

	// Convert hours to AM/PM unit
	int hour = timeinfo.tm_hour;
	hour = (hour == 0 ? 12 : hour);
	hour = (hour > 12 ? hour%12 : hour);
	data = TALKING_CLOCK_1_INDEX-1+hour;
	ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
	if (ret != ESP_OK) {
		//ESP_LOGE(TAG, "Cannot queue data");
		//return ret;
	}
	data = TALKING_CLOCK_HOUR_INDEX;
	ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
	if (ret != ESP_OK) {
		//ESP_LOGE(TAG, "Cannot queue data");
		//return ret;
	}

	int minutes = timeinfo.tm_min;

	if (minutes != 0) 
	{
		if (minutes <= 14)
		{
			data = TALKING_CLOCK_1_INDEX - 1 + minutes;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes > 14 && minutes <= 19)
		{
			data = TALKING_CLOCK_1_INDEX - 1 + (minutes % 10);
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_10_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes == 20)
		{
			data = TALKING_CLOCK_20_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes > 20 && minutes <= 29)
		{
			data = TALKING_CLOCK_1_INDEX - 1 + (minutes % 10);
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_AND_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		
			data = TALKING_CLOCK_20_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes == 30)
		{
			data = TALKING_CLOCK_30_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes > 30 && minutes <= 39)
		{
			data = TALKING_CLOCK_1_INDEX - 1 + (minutes % 10);
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_AND_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_30_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes == 40)
		{
			data = TALKING_CLOCK_40_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes > 40 && minutes <= 49)
		{
			data = TALKING_CLOCK_1_INDEX - 1 + (minutes % 10);
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_AND_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_40_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes == 50)
		{
			data = TALKING_CLOCK_50_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
		else if (minutes > 50 && minutes <= 59)
		{
			data = TALKING_CLOCK_1_INDEX - 1 + (minutes % 10);
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_AND_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);

			data = TALKING_CLOCK_50_INDEX;
			ret = xQueueSend(talking_clock_queue, &data, portMAX_DELAY);
		}
	}

	ESP_LOGI(TAG, "Queue filled with %d items", uxQueueMessagesWaiting(talking_clock_queue));

	return ESP_OK;
}

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    /* Handle touch pad events
           to start, pause, resume, finish current song and adjust volume
        */
    audio_board_handle_t board_handle = (audio_board_handle_t) ctx;
    int player_volume;
    audio_hal_get_volume(board_handle->audio_hal, &player_volume);

    if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE) 
	{
        ESP_LOGI(TAG, "[ * ] input key id is %d", (int)evt->data);
        switch ((int)evt->data) {
            case INPUT_KEY_USER_ID_PLAY:
			{
                // ESP_LOGI(TAG, "[ * ] [Play] input key event");
                // audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                // switch (el_state) 
				// {
                //     case AEL_STATE_INIT :
                //         ESP_LOGI(TAG, "[ * ] Starting audio pipeline");
				// 		talking_clock_fill_queue();
				// 		audio_element_set_uri(fatfs_stream_reader, talking_clock_files[TALKING_CLOCK_ITSNOW_INDEX]);
                //         audio_pipeline_run(pipeline);
                //         break;
                //     case AEL_STATE_RUNNING :
                //         ESP_LOGI(TAG, "[ * ] Pausing audio pipeline");
                //         audio_pipeline_pause(pipeline);
				// 		// Clear Queue
                //         break;
                //     case AEL_STATE_PAUSED :
                //         ESP_LOGI(TAG, "[ * ] Resuming audio pipeline");
				// 		// Create new queue
				// 		// Set first item in the queue
				// 		talking_clock_fill_queue();
				// 		audio_element_set_uri(fatfs_stream_reader, talking_clock_files[TALKING_CLOCK_ITSNOW_INDEX]); // Set first sample
				// 		audio_pipeline_reset_ringbuffer(pipeline);
				// 		audio_pipeline_reset_elements(pipeline);
				// 		audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
				// 		audio_pipeline_run(pipeline);	
                //         break;
                //     default :
                //         ESP_LOGI(TAG, "[ * ] Not supported state %d", el_state);
                // }
                // break;
			}
            case INPUT_KEY_USER_ID_SET:
			{
				leds_changeState(LOADING);
                talking_clock_fill_queue();
				audio_element_set_uri(fatfs_stream_reader, talking_clock_files[TALKING_CLOCK_ITSNOW_INDEX]); // Set first sample
				audio_pipeline_reset_ringbuffer(pipeline);
				audio_pipeline_reset_elements(pipeline);
				audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
				audio_pipeline_run(pipeline);

				vTaskDelay(pdMS_TO_TICKS(5000));
				leds_changeState(READY);
			}
            case INPUT_KEY_USER_ID_VOLUP:
			{
                ESP_LOGI(TAG, "[ * ] [Vol+] input key event");
                player_volume += 10;
                if (player_volume > 100) {
                    player_volume = 100;
                }
                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
                break;
			}
            case INPUT_KEY_USER_ID_VOLDOWN:
			{
                ESP_LOGI(TAG, "[ * ] [Vol-] input key event");
                player_volume -= 10;
                if (player_volume < 0) {
                    player_volume = 0;
                }
                audio_hal_set_volume(board_handle->audio_hal, player_volume);
                ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
                break;
			}
        }
    }
    return ESP_OK;
}

void talkingClockTask(void* parameter)
{
    ESP_LOGI(TAG, "[1.0] Initialize peripherals management");
    ESP_LOGI(TAG, "[1.1] Initialize and start peripherals");

	vTaskDelay(pdMS_TO_TICKS(1000));
    audio_board_key_init(set);
    audio_board_sdcard_init(set);

    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[ 3 ] Create and start input key service");
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    periph_service_handle_t input_ser = input_key_service_create(set);
    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, (void *)board_handle);

    ESP_LOGI(TAG, "[4.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[4.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[4.2] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGI(TAG, "[4.4] Create fatfs stream to read data from sdcard");
    char *url = NULL;
    fatfs_stream_cfg_t fatfs_cfg = FATFS_STREAM_CFG_DEFAULT();
    fatfs_cfg.type = AUDIO_STREAM_READER;
    fatfs_stream_reader = fatfs_stream_init(&fatfs_cfg);
    audio_element_set_uri(fatfs_stream_reader, url);

    ESP_LOGI(TAG, "[4.5] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, fatfs_stream_reader, "file");
    audio_pipeline_register(pipeline, mp3_decoder, "mp3");
    audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

    ESP_LOGI(TAG, "[4.6] Link it together [sdcard]-->fatfs_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
    audio_pipeline_link(pipeline, (const char *[]) {"file", "mp3", "i2s"}, 3);

    ESP_LOGI(TAG, "[5.0] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[5.1] Listen for all pipeline events");
    audio_pipeline_set_listener(pipeline, evt);

    ESP_LOGW(TAG, "[ 6 ] Press the keys to control talking clock");
    ESP_LOGW(TAG, "      [Play] to start reading time");
    ESP_LOGW(TAG, "      [Vol-] or [Vol+] to adjust volume.");

	// Initialize Talking clock method
	talking_clock_init();

	// Synchronize NTP time
	sntp_sync(stmp_timesync_event);

	// Setup first audio sample 'it's now'
	audio_element_set_uri(fatfs_stream_reader, talking_clock_files[TALKING_CLOCK_ITSNOW_INDEX]);

	// Initialize 1 second timer to display the time
	int id = 1;
	timer_1_sec = xTimerCreate("MyTimer", pdMS_TO_TICKS(1000), pdTRUE, ( void * )id, &timer_1_sec_callback);
	if( xTimerStart(timer_1_sec, 10 ) != pdPASS )
	{
		ESP_LOGE(TAG, "Cannot start 1 second timer");
    }

    while (1) 
	{
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) 
		{
            // Set music info for a new song to be played
            if (msg.source == (void *) mp3_decoder && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) 
			{
                audio_element_info_t music_info = {};
                audio_element_getinfo(mp3_decoder, &music_info);
                ESP_LOGI(TAG, "[ * ] Received music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                         music_info.sample_rates, music_info.bits, music_info.channels);
                audio_element_setinfo(i2s_stream_writer, &music_info);
                continue;
            }
            // Advance to the next song when previous finishes
            if (msg.source == (void *) i2s_stream_writer && msg.cmd == AEL_MSG_CMD_REPORT_STATUS) 
			{
                audio_element_state_t el_state = audio_element_get_state(i2s_stream_writer);
                if (el_state == AEL_STATE_FINISHED) 
				{
					int element = 0;
					if (uxQueueMessagesWaiting(talking_clock_queue) > 0 && 
						xQueueReceive(talking_clock_queue, &element, portMAX_DELAY)) 
						{
						ESP_LOGI(TAG, "Finish sample, towards next sample");
						url = talking_clock_files[element];
						ESP_LOGI(TAG, "URL: %s", url);
						audio_element_set_uri(fatfs_stream_reader, url);
						audio_pipeline_reset_ringbuffer(pipeline);
						audio_pipeline_reset_elements(pipeline);
						audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
						audio_pipeline_run(pipeline);
					}
					else 
					{
						// No more samples. Pause for now
						audio_pipeline_pause(pipeline);
					}
                }
                continue;
            }
        }
		vTaskDelay(10 / portTICK_RATE_MS);
    }

    ESP_LOGI(TAG, "[ 7 ] Stop audio_pipeline");
    audio_pipeline_terminate(pipeline);

    audio_pipeline_unregister(pipeline, mp3_decoder);
    audio_pipeline_unregister(pipeline, i2s_stream_writer);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    /* Stop all peripherals before removing the listener */
    //esp_periph_set_stop_all(set);
    audio_event_iface_remove_listener(esp_periph_set_get_event_iface(set), evt);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_writer);
    audio_element_deinit(mp3_decoder);
    //esp_periph_set_destroy(set);
    //periph_service_destroy(input_ser);
	vTaskDelete(NULL); //Null means delete this task
}

void startClock(void)
{
	xTaskCreate(&talkingClockTask, "clock", 8 * 1024, NULL, 5, taskHandle);
}
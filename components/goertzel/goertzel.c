#include "goertzel.h"
#include "esp_log.h"
#include "board.h"
#include "audio_common.h"
#include "raw_stream.h"
#include "filter_resample.h"
#include <math.h>
#include "radio.h"
#include <stdbool.h>


static const char *TAG = "GOERTZEL";


#define GOERTZEL_SAMPLE_RATE_HZ 8000	// Sample rate in [Hz]
#define GOERTZEL_FRAME_LENGTH_MS 100	// Block length in [ms]
#define GOERTZEL_BUFFER_LENGTH (GOERTZEL_FRAME_LENGTH_MS * GOERTZEL_SAMPLE_RATE_HZ / 1000)
#define DETECTION_LIMIT_DECIBEL 30.0f  
#define AUDIO_SAMPLE_RATE 48000			// Default sample rate in [Hz]
#define GOERTZEL_N_DETECTION 5
#define GOERTZEL_DETECTION_DELAY 10000 //2000 ms between possible commands

static bool canDetect  = true;
//average frequencies found while testing panflutes
static const int GOERTZEL_DETECT_FREQUENCIES[GOERTZEL_N_DETECTION] = {
    1083,
    1200,
    1287,
    1410,
    1581
};

//method appears to not actually effect boolean
void resetDetectTimeTask(void* pvParameters)
{
    ESP_LOGI(TAG, "Not waiting for next signal");

    vTaskDelay(GOERTZEL_DETECTION_DELAY / portTICK_RATE_MS);
    canDetect = true;
    ESP_LOGI(TAG, "Waiting for next signal");

    vTaskDelete(NULL);
}

// Allocate number of Goertzel filter
goertzel_data_t** goertzel_malloc(int numOfConfigurations) {

	goertzel_data_t** configs;
	configs = (goertzel_data_t**)malloc(numOfConfigurations*sizeof(goertzel_data_t*));
	
	if (configs == NULL) {
		ESP_LOGE(TAG, "Cannot malloc goertzel configuration");
		return ESP_FAIL;
	}
	
	for (int i = 0; i < numOfConfigurations; i++) {
		configs[i] = (goertzel_data_t*)malloc(sizeof(goertzel_data_t));
		if (configs[i] == NULL) {
			ESP_LOGE(TAG, "Cannot malloc goertzel configuration");
			return ESP_FAIL;
		}

	}
	return configs;
	
}

// Initialize goertzel configuration per configuration
esp_err_t goertzel_init_config(goertzel_data_t* config) {
	
    float floatnumSamples = (float) config->samples;
    int k = (int) (0.5f + ((floatnumSamples * config->target_frequency) / config->sample_rate));
    config->omega = (2.0f * M_PI * k) / floatnumSamples;
    float cosine = cosf(config->omega);
    config->coefficient = 2.0f * cosine;
	config->active = true;
	config->scaling_factor = floatnumSamples / 2.0f;
	
	// Reset initial filter q
	goertzel_reset(config);

	return ESP_OK;
}


// Initialize goertzel configuration for multiple configurations
esp_err_t goertzel_init_configs(goertzel_data_t** configs, int numOfConfigurations) {
	for (int i = 0; i < numOfConfigurations; i++) {
		esp_err_t ret = goertzel_init_config(configs[i]);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Cannot create Goertzel settings");
			return ret;
		}
	}
	return ESP_OK;
}

// Reset goertzel filters for a single configuration
esp_err_t goertzel_reset(goertzel_data_t* config) {
	
	config->q0 = 0.0f;
	config->q1 = 0.0f;
	config->q2 = 0.0f;
	config->sample_counter = 0;
	return ESP_OK;
}

// Reset goertzel filters for multiple configurations
esp_err_t goertzel_resets(goertzel_data_t** configs, int numOfConfigurations) {
	for (int i = 0; i < numOfConfigurations; i++) {
		goertzel_reset(configs[i]);
	}
	return ESP_OK;
}

// Process all samples for all goertzel filters
esp_err_t goertzel_proces(goertzel_data_t** configs, int numOfConfigurations, int16_t* samples, int numOfSamples) {
	
	for (int idx = 0; idx < numOfSamples; idx++) {
		float data = samples[idx];
		for (int filter = 0; filter < numOfConfigurations; filter++) {
			
			// Goertzel part 1
			goertzel_data_t* currFilter = configs[filter];
			currFilter->q0 = currFilter->coefficient * currFilter->q1 - currFilter->q2 + data;
			currFilter->q2 = currFilter->q1;
			currFilter->q1 = currFilter->q0;
			
			
			// Check if we have the amount of samples
			currFilter->sample_counter++;
			if (currFilter->sample_counter == currFilter->samples) {
				// Process part 2 
				
				float real = (currFilter->q1 - currFilter->q2 * cosf(currFilter->omega)) / currFilter->scaling_factor;
				float imag = (currFilter->q2 * sinf(currFilter->omega)) / currFilter->scaling_factor;
				float magnitude = sqrtf(real*real + imag*imag);
					
				// Provide callback support
				if (currFilter->goertzel_cb != NULL) {
					currFilter->goertzel_cb(currFilter, magnitude);
				}
				
				// Reset filter
				goertzel_reset(currFilter);
			}
			
		}
	}
	return ESP_OK;
}

// Free all goertzel configurations
esp_err_t goertzel_free(goertzel_data_t** configs) {
	if (configs != NULL && (*configs != NULL)) {
        ESP_LOGD(TAG, "free goertzel_data_t %p", *configs);
        free(*configs);
        *configs = NULL;
    } else {
        ESP_LOGE(TAG, "free goertzel_data_t failed");
		return ESP_FAIL;
    }
	
	return ESP_OK;
}

static void goertzel_callback(struct goertzel_data_t* filter, float result) {
    goertzel_data_t* filt = (goertzel_data_t*)filter;
    float logVal = 10.0f * log10f(result);

    // Detection filter. Only above 30 dB(A)
    if (logVal > DETECTION_LIMIT_DECIBEL && canDetect) {
        ESP_LOGI(TAG, "[Goertzel] Callback Freq: %d Hz amplitude: %.2f", filt->target_frequency, 10.0f * log10f(result));
        canDetect = false;
        xTaskCreate(resetDetectTimeTask, "Reset Frequency Timer Task", 2048, NULL, 5, NULL);

        switch (filt->target_frequency)
        {
            case 1083:
                ESP_LOGI(TAG, "case lowest");

                break;
            case 1200:
                ESP_LOGI(TAG, "case next lowest");

                break;
            case 1287:
                ESP_LOGI(TAG, "case middle");

                break;
            case 1410:
                ESP_LOGI(TAG, "case next highest");

                break;
            case 1581:
                ESP_LOGI(TAG, "case highest");
                radioConnect(station_3fm);
                break;
        }
    }
}


void goertzelTask(void* pvParameters)
{
    audio_pipeline_handle_t pipeline;
    audio_element_handle_t i2s_stream_reader, filter, raw_read;

    ESP_LOGI(TAG, "[ 1 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[ 2 ] Create audio pipeline for recording");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(pipeline);

    ESP_LOGI(TAG, "[2.1] Create i2s stream to read audio data from codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.i2s_config.sample_rate = AUDIO_SAMPLE_RATE;
    i2s_cfg.type = AUDIO_STREAM_READER;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);


    // Filter for reducing sample rate
    ESP_LOGI(TAG, "[2.2] Create filter to resample audio data");
    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = AUDIO_SAMPLE_RATE;
    rsp_cfg.src_ch = 2;
    rsp_cfg.dest_rate = GOERTZEL_SAMPLE_RATE_HZ;
    rsp_cfg.dest_ch = 1;
    filter = rsp_filter_init(&rsp_cfg);

    ESP_LOGI(TAG, "[2.3] Create raw to receive data");
    raw_stream_cfg_t raw_cfg = {
        .out_rb_size = 8 * 1024,
        .type = AUDIO_STREAM_READER,
    };
    raw_read = raw_stream_init(&raw_cfg);

    ESP_LOGI(TAG, "[ 3 ] Register all elements to audio pipeline");
    audio_pipeline_register(pipeline, i2s_stream_reader, "i2s");
    audio_pipeline_register(pipeline, filter, "filter");
    audio_pipeline_register(pipeline, raw_read, "raw");

    ESP_LOGI(TAG, "[ 4 ] Link elements together [codec_chip]-->i2s_stream-->filter-->raw-->[Goertzel]");
    audio_pipeline_link(pipeline, (const char* []) { "i2s", "filter", "raw" }, 3);


    // Config goertzel filters
    goertzel_data_t** goertzel_filt = goertzel_malloc(GOERTZEL_N_DETECTION); // Alloc mem
    // Apply configuration for all filters
    for (int i = 0; i < GOERTZEL_N_DETECTION; i++) {
        goertzel_data_t* currFilter = goertzel_filt[i];
        currFilter->samples = GOERTZEL_BUFFER_LENGTH;
        currFilter->sample_rate = GOERTZEL_SAMPLE_RATE_HZ;
        currFilter->target_frequency = GOERTZEL_DETECT_FREQUENCIES[i];
        currFilter->goertzel_cb = &goertzel_callback;
    }

    // Initialize goertzel filters
    goertzel_init_configs(goertzel_filt, GOERTZEL_N_DETECTION);



    ESP_LOGI(TAG, "[ 6 ] Start audio_pipeline");
    audio_pipeline_run(pipeline);



    bool noError = true;
    int16_t* raw_buff = (int16_t*)malloc(GOERTZEL_BUFFER_LENGTH * sizeof(short));
    if (raw_buff == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed!");
        noError = false;
    }

    while (noError) {
        raw_stream_read(raw_read, (char*)raw_buff, GOERTZEL_BUFFER_LENGTH * sizeof(short));

        // process Goertzel Samples
        goertzel_proces(goertzel_filt, GOERTZEL_N_DETECTION, raw_buff, GOERTZEL_BUFFER_LENGTH);

    }

    if (raw_buff != NULL) {
        free(raw_buff);
        raw_buff = NULL;
    }


    ESP_LOGI(TAG, "[ 7 ] Destroy goertzel");
    goertzel_free(goertzel_filt);

    ESP_LOGI(TAG, "[ 8 ] Stop audio_pipeline and release all resources");
    audio_pipeline_terminate(pipeline);

    /* Terminate the pipeline before removing the listener */
    audio_pipeline_remove_listener(pipeline);

    audio_pipeline_unregister(pipeline, i2s_stream_reader);
    audio_pipeline_unregister(pipeline, filter);
    audio_pipeline_unregister(pipeline, raw_read);

    /* Release all resources */
    audio_pipeline_deinit(pipeline);
    audio_element_deinit(i2s_stream_reader);
    audio_element_deinit(filter);
    audio_element_deinit(raw_read);
}

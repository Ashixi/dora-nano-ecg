#include <Arduino.h>
#include "dora_nano.h"
#include "esp_timer.h"

static dora_nano_ecg_cortex_t g_cortex;
static dora_nano_icd_state_t  g_icd;

// Protocol Constants
#define CMD_PING          0x00
#define CMD_CALIBRATE     0x01
#define CMD_PREDICT_BATCH 0x02

#define RSP_PONG          0x80
#define RSP_CALIBRATED    0x81
#define RSP_PREDICT_RES   0x82

#define MAX_CALIB_BEATS   250
#define BATCH_MAX_BEATS   32

// Calibration buffers in 8 MB PSRAM
static float* s_calib_w0 = nullptr;
static float* s_calib_w1 = nullptr;

struct BeatInput {
    float wave0[DORA_NANO_WAVEFORM_LEN];
    float wave1[DORA_NANO_WAVEFORM_LEN];
    float timing[4];
};

struct BeatResult {
    uint8_t  pred_class;
    uint8_t  icd_zone;
    uint16_t latency_us;
};

static BeatInput  s_batch_in[BATCH_MAX_BEATS];
static BeatResult s_batch_out[BATCH_MAX_BEATS];

static void read_exact(uint8_t* dst, size_t len) {
    size_t read_bytes = 0;
    while (read_bytes < len) {
        if (Serial.available()) {
            size_t n = Serial.readBytes((char*)(dst + read_bytes), len - read_bytes);
            read_bytes += n;
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial.setRxBufferSize(65536);
    Serial.setTxBufferSize(16384);

    dora_nano_ecg_init(&g_cortex);
    dora_nano_icd_init(&g_icd);

    // Allocate calibration buffers in PSRAM
    size_t calib_bytes = MAX_CALIB_BEATS * DORA_NANO_WAVEFORM_LEN * sizeof(float);
    if (psramFound()) {
        s_calib_w0 = (float*)ps_malloc(calib_bytes);
        s_calib_w1 = (float*)ps_malloc(calib_bytes);
    } else {
        s_calib_w0 = (float*)malloc(calib_bytes);
        s_calib_w1 = (float*)malloc(calib_bytes);
    }
}

void loop() {
    if (!Serial.available()) {
        return;
    }

    uint8_t cmd = Serial.read();

    if (cmd == CMD_PING) {
        uint8_t rsp[5];
        rsp[0] = RSP_PONG;
        uint32_t freq = ESP.getCpuFreqMHz();
        memcpy(&rsp[1], &freq, 4);
        Serial.write(rsp, 5);
        Serial.flush();
    }
    else if (cmd == CMD_CALIBRATE) {
        uint16_t num_beats = 0;
        read_exact((uint8_t*)&num_beats, 2);

        if (num_beats > MAX_CALIB_BEATS) {
            num_beats = MAX_CALIB_BEATS;
        }

        size_t wave_bytes = num_beats * DORA_NANO_WAVEFORM_LEN * sizeof(float);
        read_exact((uint8_t*)s_calib_w0, wave_bytes);
        read_exact((uint8_t*)s_calib_w1, wave_bytes);

        dora_nano_ecg_init(&g_cortex);
        dora_nano_icd_init(&g_icd);
        dora_nano_ecg_calibrate(&g_cortex, s_calib_w0, s_calib_w1, num_beats, 24);

        uint8_t rsp[4];
        rsp[0] = RSP_CALIBRATED;
        rsp[1] = (uint8_t)g_cortex.hippo0.count;
        rsp[2] = (uint8_t)g_cortex.hippo1.count;
        rsp[3] = 0; // Success
        Serial.write(rsp, 4);
        Serial.flush();
    }
    else if (cmd == CMD_PREDICT_BATCH) {
        uint16_t count = 0;
        read_exact((uint8_t*)&count, 2);

        if (count > BATCH_MAX_BEATS) {
            count = BATCH_MAX_BEATS;
        }

        read_exact((uint8_t*)s_batch_in, count * sizeof(BeatInput));

        for (uint16_t i = 0; i < count; ++i) {
            const BeatInput* in = &s_batch_in[i];

            int64_t t0 = esp_timer_get_time();
            int32_t pred = dora_nano_ecg_predict(&g_cortex, in->wave0, in->wave1, in->timing);
            int64_t t1 = esp_timer_get_time();

            int32_t zone = dora_nano_icd_update_multizone(&g_icd, pred, in->timing[0]);

            int64_t lat = t1 - t0;
            if (lat > 65535) lat = 65535;

            s_batch_out[i].pred_class = (uint8_t)pred;
            s_batch_out[i].icd_zone = (uint8_t)zone;
            s_batch_out[i].latency_us = (uint16_t)lat;
        }

        uint8_t header[3];
        header[0] = RSP_PREDICT_RES;
        memcpy(&header[1], &count, 2);
        Serial.write(header, 3);
        Serial.write((const uint8_t*)s_batch_out, count * sizeof(BeatResult));
        Serial.flush();
    }
}

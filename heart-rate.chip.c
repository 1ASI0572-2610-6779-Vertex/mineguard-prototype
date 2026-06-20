// Wokwi Custom Chip — MineGuard Heart-Rate (BPM) simulator
// Outputs a DC analog voltage that encodes the beats-per-minute set by the
// "Heart Rate (BPM)" slider. The firmware PulseSensor reads this pin with
// analogRead() and maps it back to BPM (40..180), so the slider acts as a
// live heart-rate dial.
//
// Encoding: 40 BPM -> 0.0 V, 180 BPM -> 3.3 V (linear).
//
// For docs: https://docs.wokwi.com/chips-api/getting-started
// SPDX-License-Identifier: MIT

#include "wokwi-api.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
  uint32_t pin_sig;
  uint32_t bpm_attr;
  timer_t  timer;
} chip_state_t;

static void update_outputs(void *user_data) {
  chip_state_t *chip = user_data;

  float bpm = attr_read(chip->bpm_attr);
  if (bpm < 40.0f)  bpm = 40.0f;
  if (bpm > 180.0f) bpm = 180.0f;

  // Map BPM (40..180) to an ESP32 ADC voltage (0..3.3 V)
  float voltage = (bpm - 40.0f) / 140.0f * 3.3f;
  pin_dac_write(chip->pin_sig, voltage);
}

void chip_init() {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));

  chip->pin_sig  = pin_init("SIG", ANALOG);
  chip->bpm_attr = attr_init_float("bpm", 75);

  const timer_config_t timer_cfg = {
    .callback  = update_outputs,
    .user_data = chip
  };
  chip->timer = timer_init(&timer_cfg);
  timer_start(chip->timer, 100000, true); // 100 ms (microseconds)

  printf("MineGuard Heart-Rate chip ready\n");
}

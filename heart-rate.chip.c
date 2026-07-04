// MineGuard Heart-Rate simulator for Wokwi.
// Generates an analog pulse waveform whose period follows the BPM slider.

#include "wokwi-api.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  uint32_t pin_sig;
  uint32_t bpm_attr;
  timer_t timer;
  uint32_t elapsed_ms;
} chip_state_t;

static void update_outputs(void *user_data) {
  chip_state_t *chip = user_data;

  float bpm = attr_read(chip->bpm_attr);
  if (bpm < 40.0f) bpm = 40.0f;
  if (bpm > 180.0f) bpm = 180.0f;

  uint32_t period_ms = (uint32_t)(60000.0f / bpm);
  uint32_t phase_ms = chip->elapsed_ms % period_ms;

  float voltage = 0.25f;
  if (phase_ms < 80) {
    voltage = 2.2f;
  } else if (phase_ms < 160) {
    voltage = 1.1f;
  }

  pin_dac_write(chip->pin_sig, voltage);
  chip->elapsed_ms += 20;
}

void chip_init() {
  chip_state_t *chip = calloc(1, sizeof(chip_state_t));

  chip->pin_sig = pin_init("SIG", ANALOG);
  chip->bpm_attr = attr_init_float("bpm", 75);
  chip->elapsed_ms = 0;

  const timer_config_t timer_cfg = {
    .callback = update_outputs,
    .user_data = chip
  };

  chip->timer = timer_init(&timer_cfg);
  timer_start(chip->timer, 20000, true);

  printf("MineGuard pulse waveform chip ready\n");
}

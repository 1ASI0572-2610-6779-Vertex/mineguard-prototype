// Wokwi Custom Chip — NEO-6M style GPS emitting NMEA over UART (9600 baud).
// Broadcasts coordinates around Lima, Peru. Emits GPGGA + GPRMC sentences at 1 Hz.
//
// For docs: https://docs.wokwi.com/chips-api/getting-started
// SPDX-License-Identifier: MIT

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uart_dev_t uart;
  uint32_t   timer_id;
  float      lat_ddmm; // latitude  in ddmm.mm  (degrees + minutes)
  float      lon_ddmm; // longitude in dddmm.mm (degrees + minutes)
} chip_state_t;

// NMEA checksum: XOR of every byte between '$' and '*'
static uint8_t nmea_checksum(const char *payload) {
  uint8_t crc = 0;
  for (int i = 0; payload[i] != 0; i++) crc ^= payload[i];
  return crc;
}

static void chip_timer_event(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;
  char buffer[128];
  char payload[100];

  // Small drift to simulate movement of the miner
  chip->lat_ddmm += 0.01f;
  chip->lon_ddmm += 0.01f;

  // GPGGA: fix data (quality 1, 8 satellites)
  snprintf(payload, sizeof(payload),
    "GPGGA,123519,%07.2f,S,%07.2f,W,1,08,0.9,150.0,M,46.9,M,,",
    chip->lat_ddmm, chip->lon_ddmm);
  uint8_t checksum = nmea_checksum(payload);
  snprintf(buffer, sizeof(buffer), "$%s*%02X\r\n", payload, checksum);
  uart_write(chip->uart, (uint8_t*)buffer, strlen(buffer));

  // GPRMC: recommended minimum (status A = active)
  snprintf(payload, sizeof(payload),
    "GPRMC,123519,A,%07.2f,S,%07.2f,W,000.5,084.4,230394,003.1,W",
    chip->lat_ddmm, chip->lon_ddmm);
  checksum = nmea_checksum(payload);
  snprintf(buffer, sizeof(buffer), "$%s*%02X\r\n", payload, checksum);
  uart_write(chip->uart, (uint8_t*)buffer, strlen(buffer));
}

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  // Lima, Peru ~ (-12.0464, -77.0428) expressed as ddmm.mm
  chip->lat_ddmm = 1202.78f; // 12 deg 02.78 min, South
  chip->lon_ddmm = 7702.57f; // 77 deg 02.57 min, West

  const uart_config_t uart_config = {
    .tx        = pin_init("TX", INPUT),
    .rx        = pin_init("RX", INPUT),
    .baud_rate = 9600,
    .rx_data   = NULL,
    .write_done = NULL,
    .user_data = chip,
  };
  chip->uart = uart_init(&uart_config);

  chip->timer_id = timer_init(&(timer_config_t){
    .callback  = chip_timer_event,
    .user_data = chip,
  });
  timer_start(chip->timer_id, 1000000, true); // 1 Hz (microseconds)

  printf("MineGuard GPS chip ready (Lima)\n");
}

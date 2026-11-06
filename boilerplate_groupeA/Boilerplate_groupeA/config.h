#pragma once

#include <stdint.h>

static const uint8_t kGatewayMacAddress[6] = {0x88,0x56,0xA6,0x7E,0x24,0x18}; // Adresse de la passerelle (écriture hexadécimale)
constexpr uint8_t kWifiChannel = 8; // Canal wifi de la passerelle

#define DEVICE_NAME "sensor_a" // modifiez cette valeur avec le nom de votre capteur (sensor_<groupe>), ex : "sensor_a"


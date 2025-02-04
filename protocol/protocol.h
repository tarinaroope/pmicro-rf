#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdlib.h>
#include <stdint.h>

#define PROTO_TEMPERATURE 0b001
#define PROTO_HUMIDITY 0b010
#define PROTO_CO2 0b100

//#define PROTO_DATA_LENGTH_BITS 27

//#define PROTO_MAX_ADDRESS_LENGTH_BITS 4

// Masks
#define PROTO_DEVICE_ADDRESS_MASK 0xF
#define PROTO_PROTOCOL_MASK 0b111
#define PROTO_TEMPERATURE_INT_MASK 0xFF
#define PROTO_TEMPERATURE_DECIMAL_MASK 0xF

/* DATA FORMAT
MSB                                                                                   LSB
|CO2 6 bits||Humidity 6 bits||Temperature 12 bits - 8 bits full, 4 bits decimal||Protocol 3 bits||Device address 4 bits|
Protocol:
- 001 Only temperature
- 010 Humidity
- 100 CO2
And possible combos
*/

inline void generate_empty_data(uint8_t device_address, uint8_t protocol, uint64_t *data)
{
    *data = (uint64_t) device_address & 0xFULL;
    *data |= (uint64_t) protocol << 4ULL;
}

void add_temperature(float temperature, uint64_t *data);

inline uint8_t get_device_address(uint64_t *data)
{
    return *data & PROTO_DEVICE_ADDRESS_MASK;
}

inline uint8_t get_protocol(uint64_t *data)
{
    return (*data >> 4) & PROTO_PROTOCOL_MASK;
}

float get_temperature(uint64_t *data);

#endif
#include "protocol.h"
#include <math.h>

void add_temperature(float temperature, uint64_t *data)
{
    // Nullify the previous temperature bits
    uint64_t mask = (~0ULL << 7) ^ (~0ULL << 19);
    *data &= ~mask;
    
    int8_t int_temp = (int8_t) temperature;
    float fract = fabs(temperature - (int)temperature); 
    uint8_t decimal = (uint8_t)(int)(fract * 10); 
    *data |= (uint64_t) decimal << 7ULL;
    *data |= (uint64_t) int_temp << 11ULL;
}

float get_temperature(uint64_t *data)
{
    int8_t decimal_part = (*data >> 7) & PROTO_TEMPERATURE_DECIMAL_MASK;
    int8_t int_part = (*data >> 11) & PROTO_TEMPERATURE_INT_MASK;
    return (int_part + (decimal_part / 10.0f));
}


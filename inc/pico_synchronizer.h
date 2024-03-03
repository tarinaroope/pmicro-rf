#ifndef RFSYNCHRONIZER_H
#define RFSYNCHRONIZER_H

#include "rf_device.h"

// Dynamic sync configuration values:
#define HIGH_ALLOWED_TX_RATE         1500      // us
#define LOW_ALLOWED_TX_RATE          600       // us
#define SYNC_SAMPLING_RATE           50        // us
#define SKEW_LOW_LIMIT               12        // Allow fastest transmission rate 600us / bit with 60us sampling rate
#define SKEW_HIGH_LIMIT              30        // Allow lowest transmission rate 1500us / bit with 60us sampling rate
#define STATE_TOLERANCE              3        // Number of wrong samples in every sync bit that can be tolerated
#define SYNC_LENGTH                  4        // Number of sync bits used for clock synchronization. Must be even number!

typedef enum
{
    PICO_SYNCHRONIZER_STATE_WAIT_SYNC = 0,            /**< Synchronization state */
    PICO_SYNCHRONIZER_STATE_START_SYNC,
    PICO_SYNCHRONIZER_STATE_SYNC,
    PICO_SYNCHRONIZER_STATE_DONE
} Pico_Synchronizer_State;

typedef struct Pico_Synchronizer Pico_Synchronizer;
struct Pico_Synchronizer
{
    RX_Synchronizer base;
    RX_Device* rx_device;

    uint8_t low_sample_count;   
    uint8_t high_sample_count;  
    uint8_t sync_sample_count;
    uint8_t processed_bit_count;
    
    uint8_t processing_high;
    uint8_t waiting_for_edge;
    uint64_t start_sync_timestamp;     

    Pico_Synchronizer_State state;
    repeating_timer_t timer;
    //uint8_t wrongcount;

    void (*state_function)(Pico_Synchronizer* /*self*/, uint8_t /*signal_state*/);
};

void pico_synchronizer_start(RX_Synchronizer* self, RX_Device* rx_device);
void pico_synchronizer_init(Pico_Synchronizer* self);

#endif
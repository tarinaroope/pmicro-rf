#ifndef RFSYNCHRONIZER_H
#define RFSYNCHRONIZER_H

#include <stdint.h>

// Dynamic sync configuration values:
#define HIGH_ALLOWED_TRAN_RATE       1500      // us
#define LOW_ALLOWED_TRAN_RATE        600       // us
#define SYNC_SAMPLING_RATE           50        // us
#define SKEW_LOW_LIMIT               12        // Allow fastest transmission rate 600us / bit with 60us sampling rate
#define SKEW_HIGH_LIMIT              30        // Allow lowest transmission rate 1500us / bit with 60us sampling rate
#define VARIATION_TOLERANCE           1        //
#define STATE_TOLERANCE               2        // Number of wrong samples in every sync bit that can be tolerated
#define SYNC_LENGTH                   16        // Number of sync bits used for clock synchronization. Must be even number!

typedef enum
{
    RX_SYNCHRONIZER_STATE_WAIT_SYNC = 0,            /**< Synchronization state */
    RX_SYNCHRONIZER_STATE_START_SYNC,
    RX_SYNCHRONIZER_STATE_SYNC,                    /**< Sync state */
    RX_SYNCHRONIZER_STATE_DONE
} RX_Synchronizer_State;

typedef struct RX_Synchronizer RX_Synchronizer;
struct RX_Synchronizer
{
    uint8_t low_sample_count;   /**< Number of samples with a value of 0. */
    uint8_t high_sample_count;  /**< Number of samples with a value of 1. */
    uint8_t sync_sample_count;
    uint8_t processed_bit_count;
    uint8_t processing_high;
    uint64_t start_sync_timestamp;     

    uint16_t detected_transmission_rate;
    RX_Synchronizer_State state;

    void (*state_function)(RX_Synchronizer* /*self*/, uint8_t /*signal_state*/); /**< Function pointer to the state processing function. */
    uint64_t (*get_timestamp)();
};

void rx_synchronizer_init(RX_Synchronizer* self, uint64_t (*get_timestamp)());
void rx_synchronizer_process(RX_Synchronizer* self, uint8_t signal_state);
void rx_synchronizer_set_state(RX_Synchronizer* self, RX_Synchronizer_State state);
void rx_synchronizer_state_wait_sync(RX_Synchronizer* self, uint8_t signal_state);
void rx_synchronizer_state_start_sync(RX_Synchronizer* self, uint8_t signal_state);
void rx_synchronizer_state_sync(RX_Synchronizer* self, uint8_t signal_state);
int8_t rx_sampler_sync_collect_high(RX_Synchronizer* self, uint8_t signal_state, uint8_t expected_count);
int8_t rx_sampler_sync_collect_low(RX_Synchronizer* self, uint8_t signal_state, uint8_t expected_count);

#endif
#ifndef RFSYNCHRONIZER_H
#define RFSYNCHRONIZER_H

#define SYNC_SAMPLING_RATE           60        // us
#define SKEW_LOW_LIMIT               10        // Allow fastest transmission rate 600us / bit with 60us sampling rate
#define SKEW_HIGH_LIMIT              25        // Allow lowest transmission rate 1500us / bit with 60us sampling rate
#define VARIATION_TOLERANCE           2        //
#define STATE_TOLERANCE               2        // Number of wrong samples in every sync bit that can be tolerated
#define SYNC_LENGTH                   8        // Number of sync bits used for clock synchronization. Must be even number!

typedef enum
{
    RX_SYNCHRONIZER_STATE_WAIT_SYNC = 0,            /**< Synchronization state */
    RX_SYNCHRONIZER_STATE_START_SYNC,
    RX_SYNCHRONIZER_STATE_SYNC,                    /**< Sync state */
    RX_SYNCHRONIZER_STATE_DONE
} RX_Synchronizer_State;

typedef struct
{
    uint8_t low_sample_count;   /**< Number of samples with a value of 0. */
    uint8_t high_sample_count;  /**< Number of samples with a value of 1. */
    uint8_t sync_sample_count;
    uint8_t processed_bit_count;
    uint8_t processing_high;
    uint64_t start_sync_timestamp;     

    uint8_t detected_transmission_rate;
    RX_Synchronizer_State state;

    void (*state_function)(void* /*self*/); /**< Function pointer to the state processing function. */
    uint64_t (*get_timestamp());
} RX_Synchronizer;

void rx_synchronizer_init(RX_Synchronizer* self, uint64_t (*get_timestamp));
void rx_synchronizer_process(RX_Synchronizer* self, uint8_t signal_state);
void rx_synchronizer_state_wait_sync(RX_Synchronizer* self, uint8_t signal_state);
void rx_synchronizer_state_start_sync(RX_Synchronizer* self, uint8_t signal_state);
void rx_synchronizer_state_sync(RX_Synchronizer* self, uint8_t signal_state);
void rx_sampler_sync_collect_high(RX_Synchronizer* self, uint8_t signal_state, uint8_t expected_count);
void rx_sampler_sync_collect_low(RX_Synchronizer* self, uint8_t signal_state, uint8_t expected_count);

#endif
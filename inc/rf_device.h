/**
 * @file rf_device.h
 * @brief This file contains the definitions of the RX_Device and TX_Device structures, as well as function prototypes for RF device operations.
 */

#ifndef RFDEVICE_H
#define RFDEVICE_H

#include <stdint.h>

#define MAX_PAYLOAD_LENGTH          64
#define PAYLOAD_LENGTH              7

#define START_SYMBOL                0xA39   
#define START_SYMBOL_LENGTH         12
#define START_SYMBOL_MASK           0xFFF

#define SYNC_SYMBOL                 0xAAAAAAAAAULL
#define SYNC_SYMBOL_LENGTH          36

#define TX_FREQUENCY                1000   // us / bit
#define SAMPLING_COUNT              10     // number of samples per bit (even). Speed = sampling_frequency / sampling_count
#define SAMPLING_TOLERANCE          1      // number of wrong samples that can be tolerated

/**
 * @brief Structure representing a received bit in a radio frequency device.
 */
typedef struct 
{
    uint8_t low_sample_count; /**< Number of samples with a value of 0. */
    uint8_t high_sample_count; /**< Number of samples with a value of 1. */
    uint8_t sync_index;     /**< Current sync index. */
    uint8_t latest_bit;     /**< Value of the latest received bit. */
} RX_BIT;

/**
 * @brief Structure representing a RF message.
 * 
 * This structure contains the fields necessary to represent a RF message.
 * It includes the message data, message length, and message CRC.
 */
typedef struct
{
    uint64_t    message;           /**< The message data. */
    uint8_t     message_length;    /**< The length of the message. */
    uint16_t    message_crc;       /**< The CRC of the message. */
} RF_Message;

/**
 * @brief Enumeration representing the different states of the RX (receive) process.
 */
typedef enum
{
    RX_SYNC = 0,            /**< Syncronization state */
    RX_WAIT_START,          /**< Waiting for start sequence state */
    RX_READ_LENGTH,         /**< Reading payload length state */
    RX_READ_PAYLOAD,        /**< Reading payload state */
    RX_READ_CRC             /**< Reading CRC (cyclic redundancy check) state */
} RX_State;

/**
 * @brief Enumeration representing the different states of the transmission process.
 */
typedef enum
{
    TX_WAKEUP = 0,          /**< Wakeup state */
    TX_SYNC,                /**< Sync state */
    TX_SEND_START,          /**< Send start state */
    TX_SEND_LENGTH,         /**< Send length state */
    TX_SEND_PAYLOAD,        /**< Send payload state */
    TX_SEND_CRC             /**< Send CRC state */
} TX_State;

typedef struct RX_Synchronizer RX_Synchronizer;

/**
 * @struct RX_Device
 * @brief Structure representing the receiver (RX) device.
 */
typedef struct RX_Device RX_Device;
struct RX_Device
{
    RX_State    state; /**< Current state of the receiver device. */
    RX_BIT      rx_bit; /**< Current bit being received. */
    RF_Message  message; /**< Message received. */

    uint8_t     signal_state; /**< Current state of the received signal. */
    uint64_t    buffer; /**< Buffer to store received bits. */
    uint8_t     buffer_current_bit_index; /**< Index pointing to next free bit slot in buffer*/

    uint64_t    sync_pattern;
    uint64_t    sync_pattern_mask; /**< Mask for detecting the synchronization pattern. */
    
    RX_Synchronizer* ext_synchronizer;

    void (*state_function)(RX_Device* /*self*/); /**< Function pointer to the state processing function. */
    void (*result_callback) (RF_Message /*message*/); /**< Function pointer to the result callback function. */
    void (*set_recurring_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); /**< Function pointer to set the recurring trigger time. */
    void (*cancel_trigger)(void* /*trigger_user_data*/); /**< Function pointer to cancel the trigger. */
    void* user_data; /**< User-defined data. */
};

struct RX_Synchronizer
{
    void (*wait_for_sync)(RX_Synchronizer* self, RX_Device* rx_device);
};

/**
 * @struct TX_Device
 * @brief Structure representing the transmitter (TX) device.
 */
typedef struct TX_Device TX_Device;
struct TX_Device
{
    TX_State    state; /**< Current state of the transmitter device. */
    RF_Message  message; /**< Message to be transmitted. */

    uint8_t     step_index; /**< Index of the current step in the transmission process. */

    void (*state_function)(TX_Device* /*self*/); /**< Function pointer to the state processing function. */
    void (*set_signal)(uint8_t /*is_high*/, void* /*user_data*/); /**< Function pointer to set the signal. */
    void (*set_onetime_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); /**< Function pointer to set the one-time trigger time. */
    void (*set_recurring_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); /**< Function pointer to set the recurring trigger time. */
    void (*cancel_trigger)(void* /*trigger_user_data*/); /**< Function pointer to cancel the trigger. */
    void* user_data; /**< User-defined data. */
};

void tx_init(   TX_Device* self, 
                void (*set_signal), 
                void (*set_onetime_trigger_time), 
                void (*set_recurring_trigger_time), 
                void (*cancel_trigger), 
                void* user_data);

void tx_callback(TX_Device* self);

void rx_init(   RX_Device* self,
                void* result_callback,
                void* set_recurring_trigger_time, 
                void* cancel_trigger,
                void* user_data);

void rx_signal_callback(RX_Device* self, uint8_t signal_status);
void rx_set_external_synchronizer(RX_Device* self, RX_Synchronizer* synchronizer);
void rx_set_detected_transmission_rate(RX_Device* self, float rate, uint8_t signal_status);
void rx_start_receiving(RX_Device* self);
void rx_stop_receiving(RX_Device* self);

#endif // RFDEVICE_H
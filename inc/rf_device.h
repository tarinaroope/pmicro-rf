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

#define SYNC_SYMBOL                 0xAAAAAAAAA
#define SYNC_SYMBOL_LENGTH          36

#define SYNC_PATTERN                0xFFC00FFC00
#define SYNC_PATTERN_MASK           0xFFFFFFFFFF

#define SAMPLING_FREQUENCY          100     // us
#define SAMPLING_COUNT              10      // number of samples per bit. Speed = sampling_frequency / sampling_count

#define SAMPLING_TOLERANCE          1       // number of wrong samples that can be tolerated
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
    RX_SYNC = 0,            /**< Synchronization state */
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

/**
 * @struct RX_Device
 * @brief Structure representing the receiver (RX) device.
 */
typedef struct
{
    RX_State    state; /**< Current state of the receiver device. */
    RX_BIT      rx_bit; /**< Current bit being received. */
    RF_Message  message; /**< Message received. */

    uint8_t     signal_state; /**< Current state of the received signal. */
    uint64_t    buffer; /**< Buffer to store received bits. */
    uint8_t     buffer_current_bit_index; /**< Index pointing to next free bit slot in buffer*/

    uint64_t    sync_buffer_mask; /**< Mask for detecting the synchronization pattern. */
    uint16_t    start_buffer_mask; /**< Mask for detecting the start pattern. */
    uint8_t     length_buffer_mask; /**< Mask for detecting the length pattern. */

    void (*state_function)(void* /*self*/); /**< Function pointer to the state processing function. */
    void (*result_callback) (RF_Message /*message*/); /**< Function pointer to the result callback function. */
    void (*set_recurring_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); /**< Function pointer to set the recurring trigger time. */
    void (*cancel_trigger)(void* /*trigger_user_data*/); /**< Function pointer to cancel the trigger. */
    void* user_data; /**< User-defined data. */

} RX_Device;

/**
 * @struct TX_Device
 * @brief Structure representing the transmitter (TX) device.
 */
typedef struct 
{
    TX_State    state; /**< Current state of the transmitter device. */
    RF_Message  message; /**< Message to be transmitted. */

    uint8_t     step_index; /**< Index of the current step in the transmission process. */

    void (*state_function)(void* /*self*/); /**< Function pointer to the state processing function. */
    void (*set_signal)(uint8_t /*is_high*/, void* /*user_data*/); /**< Function pointer to set the signal. */
    void (*set_onetime_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); /**< Function pointer to set the one-time trigger time. */
    void (*set_recurring_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); /**< Function pointer to set the recurring trigger time. */
    void (*cancel_trigger)(void* /*trigger_user_data*/); /**< Function pointer to cancel the trigger. */
    void* user_data; /**< User-defined data. */
} TX_Device;

void tx_init(   TX_Device* self, 
                void* set_signal, 
                void* set_onetime_trigger_time, 
                void *set_recurring_trigger_time, 
                void *cancel_trigger, 
                void* user_data);

/**
 * @brief Callback function for transmitting data.
 *
 * This function is called when the transmission of data is completed.
 *
 * @param self Pointer to the TX_Device object.
 */
void tx_callback(TX_Device* self);

/**
 * @brief Sends a bit using the transmitter device.
 * @param self Pointer to the TX_Device structure.
 * @param buffer Buffer containing the bit to be sent.
 * @param bit_index Index of the bit to be sent.
 */
void tx_send_bit(TX_Device* self, uint64_t buffer, uint8_t bit_index);

/**
 * @brief Sets the state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 * @param state New state to be set.
 */
void tx_set_state(TX_Device* self, TX_State state);

/**
 * @brief State processing function for the wakeup state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 */
void tx_state_process_wakeup(TX_Device* self);

/**
 * @brief State processing function for the sync state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 */
void tx_state_process_sync(TX_Device* self);

/**
 * @brief State processing function for the start state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 */
void tx_state_process_start(TX_Device* self);

/**
 * @brief State processing function for the send start state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 */
void tx_state_process_send_start(TX_Device* self);

/**
 * @brief State processing function for the send length state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 */
void tx_state_process_send_length(TX_Device* self);

/**
 * @brief State processing function for the send payload state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 */
void tx_state_process_send_payload(TX_Device* self);

/**
 * @brief State processing function for the send CRC state of the transmitter device.
 * @param self Pointer to the TX_Device structure.
 */
void tx_state_process_send_crc(TX_Device* self);

void rx_init(   RX_Device* self,
                void* result_callback,
                void* set_recurring_trigger_time, 
                void* cancel_trigger,
                void* user_data);
/**
 * @brief Callback function for the receiver device.
 * @param self Pointer to the RX_Device structure.
 * @param signal_status Status of the received signal.
 */
void rx_callback(RX_Device* self, uint8_t signal_status);

/**
 * @brief Performs sampling for the receiver device.
 * @param self Pointer to the RX_Device structure.
 * @param signal_status Status of the received signal.
 * @return true if sampling is complete and bit has been received, false otherwise.
 */
int rx_do_sampling(RX_Device* self);

/**
 * @brief Sets the state of the receiver device.
 * @param self Pointer to the RX_Device structure.
 * @param state New state to be set.
 */
void rx_set_state(RX_Device* self, RX_State state);

/**
 * @brief State processing function for the sync state of the receiver device.
 * @param self Pointer to the RX_Device structure.
 */
void rx_state_process_sync(RX_Device* self);

/**
 * @brief State processing function for the wait start state of the receiver device.
 * @param self Pointer to the RX_Device structure.
 */
void rx_state_process_wait_start(RX_Device* self);

/**
 * @brief State processing function for the read length state of the receiver device.
 * @param self Pointer to the RX_Device structure.
 */
void rx_state_process_read_length(RX_Device* self);

/**
 * @brief State processing function for the read payload state of the receiver device.
 * @param self Pointer to the RX_Device structure.
 */
void rx_state_process_read_payload(RX_Device* self);

/**
 * @brief State processing function for the read CRC state of the receiver device.
 * @param self Pointer to the RX_Device structure.
 * @param signal_status Status of the received signal.
 */
void rx_state_process_read_crc(RX_Device* self);

/**
 * @brief Starts the receiving process for the receiver device.
 * @param self Pointer to the RX_Device structure.
 */
void rx_start_receiving(RX_Device* self);

/**
 * @brief Stops the receiving process for the receiver device.
 * @param self Pointer to the RX_Device structure.
 */
void rx_stop_receiving(RX_Device* self);

#endif // RFDEVICE_H
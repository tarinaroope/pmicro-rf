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
#define SAMPLING_TOLERANCE          2      // number of wrong samples that can be tolerated

typedef struct RX_Synchronizer RX_Synchronizer;
typedef struct RX_Device RX_Device;
typedef struct TX_Device TX_Device;

typedef struct
{
    uint64_t    message;           
    uint8_t     message_length;    
    uint16_t    message_crc;       
} RF_Message;

typedef enum
{
    TX_INITIAL = 0,
    TX_WAKEUP,          
    TX_SYNC,                
    TX_SEND_START,          
    TX_SEND_LENGTH,         
    TX_SEND_PAYLOAD,        
    TX_SEND_CRC             
}TX_State;

typedef struct 
{
    uint8_t low_sample_count; 
    uint8_t high_sample_count; 
    uint8_t sync_index;     
    uint8_t latest_bit;     
} RX_Bit;

typedef enum 
{
    RX_SYNC = 0,            
    RX_WAIT_START,          
    RX_READ_LENGTH,         
    RX_READ_PAYLOAD,        
    RX_READ_CRC             
}RX_State;

struct RX_Device
{
    RX_State    state; 
    RX_Bit      rx_bit; 
    RF_Message  message; 

    uint8_t     signal_state; 
    uint64_t    buffer; 
    uint8_t     buffer_current_bit_index; 

    uint64_t    sync_pattern;
    uint64_t    sync_pattern_mask; 
    
    RX_Synchronizer* ext_synchronizer;

    void (*state_function)(RX_Device* /*self*/); 
    void (*result_callback) (RF_Message /*message*/); 
    void (*set_recurring_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); 
    void (*cancel_trigger)(void* /*trigger_user_data*/); 
    void* user_data; 
};

struct RX_Synchronizer
{
    void (*wait_for_sync)(RX_Synchronizer* self, RX_Device* rx_device);
};

struct TX_Device
{
    TX_State    state; 
    RF_Message  message; 
    uint8_t     step_index; 

    void (*state_function)(TX_Device* /*self*/); 
    void (*set_signal)(uint8_t /*is_high*/, void* /*user_data*/); 
    void (*set_onetime_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); 
    void (*set_recurring_trigger_time)(uint64_t /*time_to_trigger*/, void* /*trigger_user_data*/); 
    void (*cancel_trigger)(void* /*trigger_user_data*/); 
    void (*tx_ready)(void* /*trigger_user_data*/);
    void* user_data; 
};

// Transmitter functions

/**
 * @brief Initializes the TX device.
 *
 * @param self Pointer to the TX device structure.
 * @param set_signal Pointer to the function for setting the signal.
 * @param set_onetime_trigger_time Pointer to the function for setting a one-time trigger time.
 * @param set_recurring_trigger_time Pointer to the function for setting a recurring trigger time.
 * @param cancel_trigger Pointer to the function for canceling the trigger.
 * @param user_data User-defined data pointer.
 */
void tx_init(TX_Device* self, void (*set_signal), void (*set_onetime_trigger_time), 
                void (*set_recurring_trigger_time), void (*cancel_trigger), 
                void (*tx_ready_callback), void* user_data);

/**
 * @brief Sends a message using the TX device.
 *
 * @param self Pointer to the TX device structure.
 * @param message The message to be sent.
 * @return Returns 0 if the message is successfully sent, otherwise returns -1.
 */
int8_t tx_send_message(TX_Device* self, RF_Message message);

/**
 * @brief Callback function for the TX device.
 *
 * Called by timer ticks.
 *
 * @param self Pointer to the TX device structure.
 */
void tx_callback(TX_Device* self);

// Receiver functions

/**
 * @brief Initializes the RX device.
 *
 * @param self Pointer to the RX device structure.
 * @param result_callback Pointer to the callback function for receiving results.
 * @param set_recurring_trigger_time Pointer to the function for setting recurring trigger time.
 * @param cancel_trigger Pointer to the function for canceling trigger.
 * @param user_data User-defined data pointer.
 */
void rx_init( RX_Device* self, void* result_callback, void* set_recurring_trigger_time, 
                void* cancel_trigger, void* user_data);

/**
 * @brief Callback function for receiving RF signals.
 *
 * Function called be the timer ticks.
 * Performs signal sampling, and processes the received data.
 *
 * @param self Pointer to the RX device structure.
 * @param signal_status Status of the received signal (high or low).
 */
void rx_signal_callback(RX_Device* self, uint8_t signal_status);

/**
 * @brief Sets the external synchronizer for the RX device.
 *
 * This function sets the external synchronizer for the RX device. 
 *
 * @param self Pointer to the RX device structure.
 * @param synchronizer Pointer to the external synchronizer structure.
 */
void rx_set_external_synchronizer(RX_Device* self, RX_Synchronizer* synchronizer);

/**
 * @brief Sets the detected transmission rate and adjusts trigger time.
 *
 * Called by the external synchronizer after detecting the transmission rate from sync signal. 
 * 
 * @param self Pointer to the RX device structure.
 * @param rate Detected transmission rate of 1 bit.
 * @param signal_status Status of the last signal (high or low).
 */
void rx_set_detected_transmission_rate(RX_Device* self, float rate, uint8_t signal_status);

/**
 * @brief Starts the receiving process for the RX device.
 *
 * @param self Pointer to the RX device structure.
 */
void rx_start_receiving(RX_Device* self);

/**
 * @brief Stops the receiving process for the RX device.
 *
 * @param self Pointer to the RX device structure.
 */
void rx_stop_receiving(RX_Device* self);

#endif // RFDEVICE_H
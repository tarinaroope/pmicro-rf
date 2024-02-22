/**
 * @file rf_device.c
 * @brief This file contains the implementation of the RF device functions.
 *
 * The RF device functions include transmitting and receiving data using RF signals.
 * The functions in this file handle the state transitions and signal processing for RF communication.
 */

#include <stdlib.h>
#include <string.h>
#include "debug_logging.h"

#include "rf_device.h"

void tx_init(   TX_Device* self,
                void *set_signal,
                void *set_onetime_trigger_time,
                void *set_recurring_trigger_time,
                void *cancel_trigger,
                void* user_data)
{
    memset(self, 0, sizeof(TX_Device));
    self->set_signal = set_signal;
    self->set_onetime_trigger_time = set_onetime_trigger_time;
    self->set_recurring_trigger_time = set_recurring_trigger_time;
    self->cancel_trigger = cancel_trigger;
    self->user_data = user_data;
    tx_set_state(self, TX_WAKEUP);
}

void tx_callback(TX_Device* self)
{
    self->state_function(self);
}

void tx_send_bit(TX_Device* self, uint64_t buffer, uint8_t bit_index)
{
    uint64_t mask = 1ULL << bit_index;
    if (buffer & mask)
    {
       self->set_signal(1, self->user_data);
    }
    else
    {
        self->set_signal(0, self->user_data);
    }
}

void tx_set_state(TX_Device* self, TX_State state)
{
    TRACE("Setting state to %d", state);
    self->state = state;
    switch (state)
    {
        case TX_WAKEUP:
            self->state_function = tx_state_process_wakeup;
            self->step_index = 0;
            break;
        case TX_SYNC:
            self->state_function = tx_state_process_sync;
            self->step_index = SYNC_SYMBOL_LENGTH;   
            break;
        case TX_SEND_START:
            self->state_function = tx_state_process_send_start;
            self->step_index = START_SYMBOL_LENGTH;
            break;
        case TX_SEND_LENGTH:
            self->state_function = tx_state_process_send_length;
            self->step_index = PAYLOAD_LENGTH; // Based on max length
            break;
        case TX_SEND_PAYLOAD:
            self->state_function = tx_state_process_send_payload;
            self->step_index = self->message.message_length;
            break;
        case TX_SEND_CRC:
            self->state_function = tx_state_process_send_crc;
            self->step_index = 16;  // CRC is two bytes
            break;
        
        default:
            break;
    }
}

void tx_state_process_wakeup(TX_Device* self)
{
    if (self->step_index == 0)
    {
        self->set_signal(1, self->user_data);
        self->set_onetime_trigger_time(500, self->user_data);  // sleep 500 us
        self->step_index += 1;
    }
    else
    {
        self->set_signal(0, self->user_data);
        self->set_onetime_trigger_time(500, self->user_data);  // sleep 500 us
        tx_set_state(self, TX_SYNC);
    }
}

void tx_state_process_sync(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, SYNC_SYMBOL, self->step_index);
    if (self->step_index == (SYNC_SYMBOL_LENGTH - 1))
    {
        // First bit set, set the recurring trigger time
        self->set_recurring_trigger_time((STATIC_SAMPLING_FREQUENCY * SAMPLING_COUNT), self->user_data);  // 1ms for each bit
    }
    else if (!self->step_index)  // All sent, go to next state
    {
        tx_set_state(self, TX_SEND_START);
        return;
    }
}
 
void tx_state_process_send_start(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, START_SYMBOL, self->step_index);
    if (self->step_index == 0)
    {
        // Done sending start
        tx_set_state(self, TX_SEND_LENGTH);
    }
}

void tx_state_process_send_length(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, self->message.message_length, self->step_index);
    if (self->step_index == 0)
    {
        // Done sending start
        tx_set_state(self, TX_SEND_PAYLOAD);
    }
}

void tx_state_process_send_payload(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, self->message.message, self->step_index);
    if (self->step_index == 0)
    {
        tx_set_state(self, TX_SEND_CRC);
    }
}

void tx_state_process_send_crc(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, self->message.message_crc, self->step_index);
    if (self->step_index == 0)
    {
        tx_set_state(self, TX_WAKEUP);
        self->cancel_trigger(self->user_data);
    }
}

void rx_init(   RX_Device* self,
                void* result_callback,
                void* set_recurring_trigger_time, 
                void* cancel_trigger,
                void* user_data)
{
    memset(self, 0, sizeof(RX_Device));
    self->result_callback = result_callback;
    self->set_recurring_trigger_time = set_recurring_trigger_time;
    self->cancel_trigger = cancel_trigger;
    self->user_data = user_data;
    rx_set_state(self, RX_SYNC);
}

void rx_set_sync_mode(RX_Device* self, uint8_t mode, uint64_t (*timestamp_callback)())
{
    if (mode == SYNC_MODE_DYNAMIC && timestamp_callback != NULL)
    {
        self->synchronizer = malloc(sizeof(RX_Synchronizer));
        rx_synchronizer_init(self->synchronizer, timestamp_callback);
    }
    else
    {
        free(self->synchronizer);
        self->synchronizer = NULL;
    }
}

void rx_set_state(RX_Device* self, RX_State state)
{
    TRACE("Setting state to %d", state);

    self->state = state;
    switch (state)
    {
        case RX_SYNC:
            self->state_function = rx_state_process_sync;
            
            if (self->synchronizer)
            {
                rx_synchronizer_set_state(self->synchronizer, RX_SYNCHRONIZER_STATE_WAIT_SYNC);
                
                // Set back the default sampling rate
                self->cancel_trigger(self->user_data);
                self->set_recurring_trigger_time(SYNC_SAMPLING_RATE, self->user_data);
            }
            break;
        case RX_WAIT_START:
            self->state_function = rx_state_process_wait_start;

            break;
        case RX_READ_LENGTH:
            self->state_function = rx_state_process_read_length;
            
            break;
        case RX_READ_PAYLOAD:
            self->state_function = rx_state_process_read_payload;
            
            break;
        case RX_READ_CRC:
            self->state_function = rx_state_process_read_crc;
            
            break;    
        default:
            break;
    }

    self->buffer = 0;
    self->buffer_current_bit_index = 0;
}

int rx_do_sampling(RX_Device* self)
{
    if (self->rx_bit.sync_index > 0 && self->rx_bit.sync_index < (SAMPLING_COUNT - 1)) // Skip the first and last slot
    {       
        // Get a sample
        if (self->signal_state == 0)
        {
            self->rx_bit.low_sample_count += 1;
        }
        else
        {
            self->rx_bit.high_sample_count += 1;
        }  
    }
    else if (self->rx_bit.sync_index == (SAMPLING_COUNT - 1)) 
    {
        self->rx_bit.sync_index = 0;
        // Determine how many same samples we need to identify the bit
        uint8_t neededCount = SAMPLING_COUNT - SAMPLING_TOLERANCE - 2;

        if (self->rx_bit.low_sample_count >= neededCount)  
        {
            self->rx_bit.latest_bit = 0;
            self->rx_bit.low_sample_count = 0;
            self->rx_bit.high_sample_count = 0;
            // We have a bit
            return 1;
        }
        else if (self->rx_bit.high_sample_count >= neededCount) 
        {
            self->rx_bit.latest_bit = 1;
            self->rx_bit.low_sample_count = 0;
            self->rx_bit.high_sample_count = 0;
            // We have a bit
            return 1;
        }
        else
        {
            // Not enough proper samples found -> error in data.
            self->rx_bit.sync_index = 0;
            self->rx_bit.low_sample_count = 0;
            self->rx_bit.high_sample_count = 0;
            // No bit
            return -1;
        }
    }
    // Continue sampling
    self->rx_bit.sync_index += 1;
    return 0;    
}

void rx_callback(RX_Device* self, uint8_t signal_status)
{
    self->signal_state = signal_status;
 
    if (self->state != RX_SYNC) // Sampling not done for SYNC state
    {
        int res = rx_do_sampling(self);
        if (!res)
        {
            // Continue
            return;
        }
        else if (res < 0)
        {
            // Error in data, go back to sync state
            rx_set_state(self, RX_SYNC);
            return;
        } 
    }

    self->state_function(self);
}

void rx_state_process_sync(RX_Device* self)
{
    if (self->synchronizer != NULL)
    {
        rx_synchronizer_process(self->synchronizer, self->signal_state);
        if (self->synchronizer->state == RX_SYNCHRONIZER_STATE_DONE)
        {
            rx_set_state(self, RX_WAIT_START);

            // Assuming we always start sync time detection from 1 bit in sync pattern and have the 
            // sync bit pattern detection count even, we can assume that we have already collected one high sample
            self->rx_bit.high_sample_count = 1;

            // Adjust the recurring trigger time based on the detected transmission rate
            self->cancel_trigger(self->user_data);
            self->set_recurring_trigger_time(
                                            self->synchronizer->detected_transmission_rate / SAMPLING_COUNT,
                                            self->user_data); 
        }
    }
    else
    {
        // Using static synchronization

        self->buffer |= self->signal_state;
        self->buffer &= SYNC_PATTERN_MASK;
        if (self->buffer == SYNC_PATTERN)
        {
            // Sync pattern found and our sampler is in sync. Start reading bits.
            rx_set_state(self, RX_WAIT_START);
        }
        else
        {
            // Continue
            self->buffer <<= 1ULL;
        }
    }
}

void rx_state_process_wait_start(RX_Device* self)
{
    self->buffer |= self->rx_bit.latest_bit;
    self->buffer &= START_SYMBOL_MASK ; 
    if (self->buffer == START_SYMBOL) 
    {
        // Start symbol found, start reading length
        rx_set_state(self, RX_READ_LENGTH);
    }  
    else if (self->buffer_current_bit_index > (SYNC_SYMBOL_LENGTH + START_SYMBOL_LENGTH)) 
    {
        // No start symbol found. Go back to sync state
        rx_set_state(self, RX_SYNC);
    }
    else
    {
        // Continue
        self->buffer <<= 1ULL;
        self->buffer_current_bit_index += 1;
    }
}

void rx_state_process_read_length(RX_Device* self)
{  
    self->buffer |= self->rx_bit.latest_bit;
    if (self->buffer_current_bit_index == (PAYLOAD_LENGTH - 1))
    {
        if (self->buffer <= MAX_PAYLOAD_LENGTH)
        {
            // Length found, start reading payload
            self->message.message_length = self->buffer;
            rx_set_state(self, RX_READ_PAYLOAD);
        }
        else
        {
            // Invalid length. Go back to sync state
            rx_set_state(self, RX_SYNC);
        }
    }
    else
    {
        // Continue
        self->buffer <<= 1ULL;
        self->buffer_current_bit_index += 1;
    }
} 

void rx_state_process_read_payload(RX_Device* self)
{
    self->buffer |= self->rx_bit.latest_bit;
    if (self->buffer_current_bit_index == (self->message.message_length - 1))
    {
        // Payload received
        self->message.message = self->buffer;
        rx_set_state(self, RX_READ_CRC);
    }
    else
    {
        // Continue
        self->buffer <<= 1ULL;
        self->buffer_current_bit_index += 1;
    }
}
void rx_state_process_read_crc(RX_Device* self)
{
    self->buffer |= self->rx_bit.latest_bit;
    if (self->buffer_current_bit_index == 15)  // 16 bits for CRC
    {
        // CRC received
        self->message.message_crc = self->buffer;
        self->result_callback(self->message);
        //Go back to sync state
        rx_set_state(self, RX_SYNC);
    }
    else
    {
        // Continue
        self->buffer <<= 1ULL;
        self->buffer_current_bit_index += 1;
    }
}

void rx_start_receiving(RX_Device* self)
{
    if (!self->synchronizer)
    {
        // Static synchornization so use static sampling frequency
        self->set_recurring_trigger_time(STATIC_SAMPLING_FREQUENCY, self->user_data);
    }
    else
    {
        // Dynamic synchronization, we need to start with the sampling frequency needed by synchronizer
        self->set_recurring_trigger_time(SYNC_SAMPLING_RATE, self->user_data);
    }
}

void rx_stop_receiving(RX_Device* self)
{
    self->cancel_trigger(self->user_data);
}

   

/**
 * @file rx_device.c
 * @brief This file contains the implementation of the RX device functions.
 *
 * The RX device functions include receiving data using RF signals.
 * The functions in this file handle the state transitions and signal processing for RF communication.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "debug_logging.h"
#include "rf_device.h"

#include <stdio.h>

static void rx_set_state(RX_Device* self, RX_State state);

void rx_init(   RX_Device* self,
                void* result_callback,
                void* set_recurring_trigger_time, 
                void* cancel_trigger,
                void* user_data)
{
    memset(self, 0, sizeof(RX_Device));

    // Set functions
    self->result_callback = result_callback;
    self->set_recurring_trigger_time = set_recurring_trigger_time;
    self->cancel_trigger = cancel_trigger;
    self->user_data = user_data;
    
    // Prepare sync data
    uint8_t start_sync_pattern = SYNC_SYMBOL >> (SYNC_SYMBOL_LENGTH - 4); // Get 4 highest bits
    uint8_t sync_pattern_length = SAMPLING_COUNT * 4;
    uint16_t bitmask = (start_sync_pattern >> 3) & 0x1; // Use 4th bit as the mask
    for (int i = 0; i < sync_pattern_length; i++)
    {
        self->sync_pattern |= bitmask;
        self->sync_pattern_mask |= 1;
        if (i < (sync_pattern_length - 1))
        {
            self->sync_pattern <<= 1;
            self->sync_pattern_mask <<= 1;
        }

        if (!((i + 1) % SAMPLING_COUNT))
        {
            start_sync_pattern <<= 1;
            bitmask = (start_sync_pattern >> 3) & 0x1;
        }
    }
}

void rx_set_external_synchronizer(RX_Device* self, RX_Synchronizer* synchronizer)
{
    self->ext_synchronizer = synchronizer;
}

void rx_set_detected_transmission_rate(RX_Device* self, float rate, uint8_t signal_status)
{
    // Adjust the recurring trigger time based on the detected transmission rate
   
    uint16_t sync_rate = round((float) rate / SAMPLING_COUNT);
    self->set_recurring_trigger_time(sync_rate, self->user_data);
    rx_set_state(self, RX_WAIT_START);
    
    if (signal_status)
    {
        self->rx_bit.high_sample_count = 1;
    }
    else
    {
        self->rx_bit.low_sample_count = 1;
    }
    self->rx_bit.sync_index = 1;
}

static inline void rx_return_to_sync(RX_Device* self)
{
    if (self->ext_synchronizer)
    {
        self->cancel_trigger(self->user_data);
    }
    rx_set_state(self, RX_SYNC);
}

static int rx_do_sampling(RX_Device* self)
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

void rx_signal_callback(RX_Device* self, uint8_t signal_status)
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
            rx_return_to_sync(self);
            return;
        } 
    }
    if (self->state_function)
    {
        self->state_function(self);
    }
}

static void rx_state_process_sync(RX_Device* self)
{
        // Using static synchronization

        self->buffer |= self->signal_state;
        self->buffer &= self->sync_pattern_mask;
        if (self->buffer == self->sync_pattern)
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

static void rx_state_process_wait_start(RX_Device* self)
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
        rx_return_to_sync(self);
    }
    else
    {
        // Continue
        self->buffer <<= 1ULL;
        self->buffer_current_bit_index += 1;
    }
}

static void rx_state_process_read_length(RX_Device* self)
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
            rx_return_to_sync(self);
        }
    }
    else
    {
        // Continue
        self->buffer <<= 1ULL;
        self->buffer_current_bit_index += 1;
    }
} 

static void rx_state_process_read_payload(RX_Device* self)
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

static void rx_state_process_read_crc(RX_Device* self)
{
    self->buffer |= self->rx_bit.latest_bit;
    if (self->buffer_current_bit_index == 15)  // 16 bits for CRC
    {
        // CRC received
        self->message.message_crc = self->buffer;
        self->result_callback(self->message);
        rx_return_to_sync(self);
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
    rx_set_state(self, RX_SYNC);
    if (!self->ext_synchronizer)
    {
        self->set_recurring_trigger_time((TX_FREQUENCY / SAMPLING_COUNT), self->user_data);
    }
}

void rx_stop_receiving(RX_Device* self)
{
    self->cancel_trigger(self->user_data);
    free(self->ext_synchronizer);
    self->ext_synchronizer = NULL;
}

static void rx_set_state(RX_Device* self, RX_State state)
{
    TRACE("Setting state to %d", state);
    self->state = state;
    switch (state)
    {
        case RX_SYNC:
            if (self->ext_synchronizer)
            {
                self->ext_synchronizer->wait_for_sync(self->ext_synchronizer, self);
                self->state_function = NULL;
            }
            else
            {
                self->state_function = rx_state_process_sync;
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

   

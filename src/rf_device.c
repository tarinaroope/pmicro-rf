/**
 * @file rf_device.c
 * @brief This file contains the implementation of the RF device functions.
 *
 * The RF device functions include transmitting and receiving data using RF signals.
 * The functions in this file handle the state transitions and signal processing for RF communication.
 */

#include "pico/stdlib.h"
#include <string.h>
#include "debug_logging.h"

#include "rf_device.h"

void tx_init(   TX_Device* this, 
                void *set_signal, 
                void *set_onetime_trigger_time, 
                void *set_recurring_trigger_time, 
                void *cancel_trigger, 
                void* user_data)
{
    memset(this, 0, sizeof(TX_Device));
    this->set_signal = set_signal;
    this->set_onetime_trigger_time = set_onetime_trigger_time;
    this->set_recurring_trigger_time = set_recurring_trigger_time;
    this->cancel_trigger = cancel_trigger;
    this->user_data = user_data;
    tx_set_state(this, TX_WAKEUP);
}

void tx_callback(TX_Device* this)
{
    this->state_function(this);
}

void tx_send_bit(TX_Device* this, uint64_t buffer, uint8_t bit_index)
{
    uint64_t mask = 1 << bit_index;
    if (buffer & mask)
    {
       this->set_signal(true, this->user_data);
    }
    else
    {
        this->set_signal(false, this->user_data);
    }
}

void tx_set_state(TX_Device* this, TX_State state)
{
    TRACE("Setting state to %d", state);
    this->state = state;
    switch (state)
    {
        case TX_WAKEUP:
            this->state_function = tx_state_process_wakeup;
            this->step_index = 0;
            break;
        case TX_SYNC:
            this->state_function = tx_state_process_sync;
            this->step_index = SYNC_SYMBOL_LENGTH;   
            break;
        case TX_SEND_START:
            this->state_function = tx_state_process_send_start;
            this->step_index = START_SYMBOL_LENGTH;
            break;
        case TX_SEND_LENGTH:
            this->state_function = tx_state_process_send_length;
            this->step_index = PAYLOAD_LENGTH; // Based on max length
            break;
        case TX_SEND_PAYLOAD:
            this->state_function = tx_state_process_send_payload;
            this->step_index = this->message.message_length;
            break;
        case TX_SEND_CRC:
            this->state_function = tx_state_process_send_crc;
            this->step_index = 16;  // CRC is two bytes
            break;
        
        default:
            break;
    }
}

void tx_state_process_wakeup(TX_Device* this)
{
    if (this->step_index == 0)
    {
        this->set_signal(true, this->user_data);
        this->set_onetime_trigger_time(500, this->user_data);  // sleep 500 us
        this->step_index += 1;
    }
    else
    {
        this->set_signal(false, this->user_data);
        this->set_onetime_trigger_time(500, this->user_data);  // sleep 500 us
        tx_set_state(this, TX_SYNC);
    }
}

void tx_state_process_sync(TX_Device* this)
{
    this->step_index -= 1;
    tx_send_bit(this, SYNC_SYMBOL, this->step_index);
    if (this->step_index == (SYNC_SYMBOL_LENGTH - 1))
    {
        // First bit set, set the recurring trigger time
        this->set_recurring_trigger_time((SAMPLING_FREQUENCY * SAMPLING_COUNT), this->user_data);  // 1ms for each bit
    }
    else if (!this->step_index)  // All sent, go to next state
    {
        tx_set_state(this, TX_SEND_START);
        return;
    }
}
 
void tx_state_process_send_start(TX_Device* this)
{
    this->step_index -= 1;
    tx_send_bit(this, START_SYMBOL, this->step_index);
    if (this->step_index == 0)
    {
        // Done sending start
        tx_set_state(this, TX_SEND_LENGTH);
    }
}

void tx_state_process_send_length(TX_Device* this)
{
    this->step_index -= 1;
    tx_send_bit(this, this->message.message_length, this->step_index);
    if (this->step_index == 0)
    {
        // Done sending start
        tx_set_state(this, TX_SEND_PAYLOAD);
    }
}

void tx_state_process_send_payload(TX_Device* this)
{
    this->step_index -= 1;
    tx_send_bit(this, this->message.message, this->step_index);
    if (this->step_index == 0)
    {
        tx_set_state(this, TX_SEND_CRC);
    }
}

void tx_state_process_send_crc(TX_Device* this)
{
    this->step_index -= 1;
    tx_send_bit(this, this->message.message_crc, this->step_index);
    if (this->step_index == 0)
    {
        tx_set_state(this, TX_WAKEUP);
        this->cancel_trigger(this->user_data);
    }
}

void rx_init(   RX_Device* this,
                void* result_callback,
                void* set_recurring_trigger_time, 
                void* cancel_trigger,
                void* user_data)
{
    memset(this, 0, sizeof(RX_Device));
    this->result_callback = result_callback;
    this->set_recurring_trigger_time = set_recurring_trigger_time;
    this->cancel_trigger = cancel_trigger;
    this->user_data = user_data;
    rx_set_state(this, RX_SYNC);
}

void rx_set_state(RX_Device* this, RX_State state)
{
    TRACE("Setting state to %d", state);

    this->state = state;
    switch (state)
    {
        case RX_SYNC:
            this->state_function = rx_state_process_sync;
            this->buffer = 0;
            this->buffer_current_bit_index = 0;
            break;
        case RX_WAIT_START:
            this->state_function = rx_state_process_wait_start;
            this->buffer = 0;
            this->buffer_current_bit_index = 0;
            break;
        case RX_READ_LENGTH:
            this->state_function = rx_state_process_read_length;
            this->buffer = 0;
            this->buffer_current_bit_index = 0;
            break;
        case RX_READ_PAYLOAD:
            this->state_function = rx_state_process_read_payload;
            this->buffer = 0;
            this->buffer_current_bit_index = 0;
            break;
        case RX_READ_CRC:
            this->state_function = rx_state_process_read_crc;
            this->buffer = 0;
            this->buffer_current_bit_index = 0;
            break;    
        default:
            break;
    }
}

int rx_do_sampling(RX_Device* this)
{
    if (this->rx_bit.sync_index > 0 && this->rx_bit.sync_index < (SAMPLING_COUNT - 1)) // Skip the first and last slot
    {       
        // Get a sample
        if (this->signal_state == 0)
        {
            this->rx_bit.low_sample_count += 1;
        }
        else
        {
            this->rx_bit.high_sample_count += 1;
        }  
    }
    else if (this->rx_bit.sync_index == (SAMPLING_COUNT - 1)) 
    {
        this->rx_bit.sync_index = 0;
        // Determine how many same samples we need to identify the bit
        uint8_t neededCount = SAMPLING_COUNT - SAMPLING_TOLERANCE - 2;

        if (this->rx_bit.low_sample_count >= neededCount)  
        {
            this->rx_bit.latest_bit = 0;
            this->rx_bit.low_sample_count = 0;
            this->rx_bit.high_sample_count = 0;
            // We have a bit
            return 1;
        }
        else if (this->rx_bit.high_sample_count >= neededCount) 
        {
            this->rx_bit.latest_bit = 1;
            this->rx_bit.low_sample_count = 0;
            this->rx_bit.high_sample_count = 0;
            // We have a bit
            return 1;
        }
        else
        {
            // Not enough proper samples found -> error in data.
            this->rx_bit.sync_index = 0;
            this->rx_bit.low_sample_count = 0;
            this->rx_bit.high_sample_count = 0;
            // No bit
            return -1;
        }
    }
    // Continue sampling
    this->rx_bit.sync_index += 1;
    return 0;    
}

void rx_callback(RX_Device* this, uint8_t signal_status)
{
    this->signal_state = signal_status;
 
    if (this->state != RX_SYNC) // Sampling not done for SYNC state
    {
        int res = rx_do_sampling(this);
        if (!res)
        {
            // Continue
            return;
        }
        else if (res < 0)
        {
            // Error in data, go back to sync state
            rx_set_state(this, RX_SYNC);
            return;
        } 
    }

    this->state_function(this);
}

void rx_state_process_sync(RX_Device* this)
{
    this->buffer |= this->signal_state;
    this->buffer &= SYNC_PATTERN_MASK;
    if (this->buffer == SYNC_PATTERN)
    {
        // Sync pattern found and our sampler is in sync. Start reading bits.
        rx_set_state(this, RX_WAIT_START);
    }
    else
    {
        // Continue
        this->buffer <<= 1;
    }
}

void rx_state_process_wait_start(RX_Device* this)
{
    this->buffer |= this->rx_bit.latest_bit;
    this->buffer &= START_SYMBOL_MASK ; 
    if (this->buffer == START_SYMBOL) 
    {
        // Start symbol found, start reading length
        rx_set_state(this, RX_READ_LENGTH);
    }  
    else if (this->buffer_current_bit_index > (SYNC_SYMBOL_LENGTH + START_SYMBOL_LENGTH)) 
    {
        // No start symbol found. Go back to sync state
        rx_set_state(this, RX_SYNC);
    }
    else
    {
        // Continue
        this->buffer <<= 1;
        this->buffer_current_bit_index += 1;
    }
}

void rx_state_process_read_length(RX_Device* this)
{  
    this->buffer |= this->rx_bit.latest_bit;
    if (this->buffer_current_bit_index == (PAYLOAD_LENGTH - 1))
    {
        if (this->buffer <= MAX_PAYLOAD_LENGTH)
        {
            // Length found, start reading payload
            this->message.message_length = this->buffer;
            rx_set_state(this, RX_READ_PAYLOAD);
        }
        else
        {
            // Invalid length. Go back to sync state
            rx_set_state(this, RX_SYNC);
        }
    }
    else
    {
        // Continue
        this->buffer <<= 1;
        this->buffer_current_bit_index += 1;
    }
} 

void rx_state_process_read_payload(RX_Device* this)
{
    this->buffer |= this->rx_bit.latest_bit;
    if (this->buffer_current_bit_index == (this->message.message_length - 1))
    {
        // Payload received
        this->message.message = this->buffer;
        rx_set_state(this, RX_READ_CRC);
    }
    else
    {
        // Continue
        this->buffer <<= 1;
        this->buffer_current_bit_index += 1;
    }
}
void rx_state_process_read_crc(RX_Device* this)
{
    this->buffer |= this->rx_bit.latest_bit;
    if (this->buffer_current_bit_index == 15)  // 16 bits for CRC
    {
        // CRC received
        this->message.message_crc = this->buffer;
        this->result_callback(this->message);
        //Go back to sync state
        rx_set_state(this, RX_SYNC);
    }
    else
    {
        // Continue
        this->buffer <<= 1;
        this->buffer_current_bit_index += 1;
    }
}

void rx_start_receiving(RX_Device* this)
{
    this->set_recurring_trigger_time(SAMPLING_FREQUENCY, this->user_data);
}

void rx_stop_receiving(RX_Device* this)
{
    this->cancel_trigger(this->user_data);
}

   
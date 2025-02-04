/**
 * @file tx_device.c
 * @brief This file contains the implementation of the TX device functions.
 *
 * The TX device functions include transmitting data using RF signals.
 * The functions in this file handle the state transitions and signal processing for RF communication.
 */

#include <stdlib.h>
#include <string.h>
#include "debug_logging.h"
#include "rf_device.h"

static void tx_set_state(TX_Device* self, TX_State state);

void tx_callback(TX_Device* self)
{
    if (self->state_function)
    {
        self->state_function(self);
    }
}

static void tx_send_bit(TX_Device* self, uint64_t buffer, uint8_t bit_index)
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

void tx_init(TX_Device* self, void (*set_signal), void (*set_onetime_trigger_time), 
                void (*set_recurring_trigger_time), void (*cancel_trigger), 
                void (*tx_ready_callback), void* user_data)
{
    memset(self, 0, sizeof(TX_Device));


    // Set functions
    self->set_signal = set_signal;
    self->set_onetime_trigger_time = set_onetime_trigger_time;
    self->set_recurring_trigger_time = set_recurring_trigger_time;
    self->cancel_trigger = cancel_trigger;
    self->tx_ready = tx_ready_callback;
    self->user_data = user_data;

    // Start state
    tx_set_state(self, TX_INITIAL);
}

int8_t tx_send_message(TX_Device* self, RF_Message* message)
{
    if (self->state != TX_INITIAL)
    {
        return -1;
    }
    else
    {
        self->message = *message;
        tx_set_state(self, TX_WAKEUP);
        tx_callback(self);
        return 0;
    }
}

static void tx_state_process_wakeup(TX_Device* self)
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

static void tx_state_process_sync(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, SYNC_SYMBOL, self->step_index);
    if (self->step_index == (SYNC_SYMBOL_LENGTH - 1))
    {
        // First bit set, set the recurring trigger time
        self->set_recurring_trigger_time(TX_FREQUENCY, self->user_data);  // 1ms for each bit
    }
    else if (!self->step_index)  // All sent, go to next state
    {
        tx_set_state(self, TX_SEND_START);
        return;
    }
}
 
static void tx_state_process_send_start(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, START_SYMBOL, self->step_index);
    if (self->step_index == 0)
    {
        // Done sending start
        tx_set_state(self, TX_SEND_LENGTH);
    }
}

static void tx_state_process_send_length(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, self->message.message_length, self->step_index);
    if (self->step_index == 0)
    {
        // Done sending start
        tx_set_state(self, TX_SEND_PAYLOAD);
    }
}

static void tx_state_process_send_payload(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, self->message.message, self->step_index);
    if (self->step_index == 0)
    {
        tx_set_state(self, TX_SEND_CRC);
    }
}

static void tx_state_process_send_crc(TX_Device* self)
{
    self->step_index -= 1;
    tx_send_bit(self, self->message.message_crc, self->step_index);
    if (self->step_index == 0)
    {
        tx_set_state(self, TX_INITIAL);
        self->cancel_trigger(self->user_data);
        self->tx_ready(self->user_data);   
          
    }
}

static void tx_set_state(TX_Device* self, TX_State state)
{
    TRACE("Setting state to %d", state);
    self->state = state;
    switch (state)
    {
        case TX_INITIAL:
            self->state_function = NULL;
            break;
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

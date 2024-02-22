#include "rf_synchronizer.h"
#include <string.h>

void rx_synchronizer_init(RX_Synchronizer* self, uint64_t (*get_timestamp)())
{
    memset(self, 0, sizeof(RX_Synchronizer));
    self->state = RX_SYNCHRONIZER_STATE_WAIT_SYNC;
    self->state_function = rx_synchronizer_state_wait_sync;
    self->get_timestamp = get_timestamp;
}

void rx_synchronizer_process(RX_Synchronizer* self, uint8_t signal_state)
{
    if (self->state_function != NULL)
    {    
        self->state_function(self, signal_state);
    }
}

void rx_synchronizer_set_state(RX_Synchronizer* self, RX_Synchronizer_State state)
{
    self->state = state;
    switch (state)
    {
        case RX_SYNCHRONIZER_STATE_WAIT_SYNC:
            self->sync_sample_count = 0;
            self->state_function = rx_synchronizer_state_wait_sync;
            break;
        case RX_SYNCHRONIZER_STATE_START_SYNC:
            self->state_function = rx_synchronizer_state_start_sync;
            break;
        case RX_SYNCHRONIZER_STATE_SYNC:
            self->processing_high = 0;
            self->processed_bit_count = 0;
            self->state_function = rx_synchronizer_state_sync;
            break;
        case RX_SYNCHRONIZER_STATE_DONE:
            self->state_function = NULL;
            break;
    }

    self->high_sample_count = 0;
    self->low_sample_count = 0;
}

// Wait for enough highs and then for the first low
void rx_synchronizer_state_wait_sync(RX_Synchronizer* self, uint8_t signal_state)
{
    if (signal_state && self->high_sample_count < SKEW_LOW_LIMIT)   
    {
        self->high_sample_count += 1;
    }
    else if (!signal_state && self->high_sample_count == SKEW_LOW_LIMIT)  
    {
         // We have enough high samples, start waiting for the first low sample
         rx_synchronizer_set_state(self, RX_SYNCHRONIZER_STATE_START_SYNC);
    }
    else if (!signal_state)
    {
        // Got low sample so nullify the high sample count
        self->high_sample_count  = 0;
    }
}

// Collect low samples and wait for the first high sample.
void rx_synchronizer_state_start_sync(RX_Synchronizer* self, uint8_t signal_state)
{
    uint8_t res = rx_sampler_sync_collect_low(self, signal_state, 0); // 0 means no expected sample count at this point yet
    if (res < 0)
    {
        // Sampling error, go back to waiting for the high samples
        rx_synchronizer_set_state(self, RX_SYNCHRONIZER_STATE_WAIT_SYNC);
    }
    else if (res > 0)
    {
        // Got enough low samples and the first high sample
        self->sync_sample_count = res;
        self->start_sync_timestamp = self->get_timestamp();
        rx_synchronizer_set_state(self, RX_SYNCHRONIZER_STATE_SYNC);
    }
}

void rx_synchronizer_state_sync(RX_Synchronizer* self, uint8_t signal_state)
{
    uint8_t res = 0;
    if (self->processing_high)
    {
        res = rx_sampler_sync_collect_high(self, signal_state, self->sync_sample_count);
    }
    else
    {
        res = rx_sampler_sync_collect_high(self, signal_state, self->sync_sample_count);
    }
    if (res > 0)
    {
        self->processed_bit_count += 1;
        if (self->processed_bit_count == SYNC_LENGTH)
        {
            uint64_t sync_time = self->get_timestamp() - self->start_sync_timestamp;
            self->detected_transmission_rate = sync_time / SYNC_LENGTH;
            rx_synchronizer_set_state(self, RX_SYNCHRONIZER_STATE_DONE);
        }
        else
        {
            // Ensure we store the last taken sample also before flipping the processing state
            if (self->processing_high)
            {
                self->high_sample_count = 1;
                self->low_sample_count = 0;

            }
            else
            {
                self->low_sample_count = 1;
                self->high_sample_count = 0;
            }
            // Flip
            self->processing_high ^= 1;
        }
    }
    else if (res < 0)
    {
        // Sampling error, go back to waiting for the high samples
        rx_synchronizer_set_state(self, RX_SYNCHRONIZER_STATE_WAIT_SYNC);
    }
}

uint8_t rx_sampler_sync_collect_high(RX_Synchronizer* self, uint8_t signal_state, uint8_t expected_count)
{
    if (signal_state)
    {
        self->high_sample_count += 1;
    }
    else
    {
        if (!expected_count)
        {
            // No expected sample count, check against defined low and high limit
            if (self->high_sample_count >= SKEW_LOW_LIMIT && 
                self->high_sample_count <= SKEW_HIGH_LIMIT)
            {
                // We have enough high samples, return number of collected samples
                return self->high_sample_count;
            }
            else
            {
                // Too few high samples
                return (-1) * self->high_sample_count;
            }
        }
        else if (self->high_sample_count >= (expected_count - VARIATION_TOLERANCE))
        {
            // We have enough high samples, return number of collected samples
            return self->high_sample_count + self->low_sample_count; // total number of samples
        }

        self->low_sample_count += 1;

        // Check if we have too many low samples
        if (self->low_sample_count >= STATE_TOLERANCE)
        {
            return (-1) * self->low_sample_count;
        }
    }
    
    // Check if we have too many high samples

    if (!expected_count && self->high_sample_count > SKEW_HIGH_LIMIT)
    {
        return (-1) * self->high_sample_count;
    }
    else if (self->high_sample_count > (expected_count - VARIATION_TOLERANCE)) 
    {
        return (-1) * self->high_sample_count;
    }

    return 0;
}

uint8_t rx_sampler_sync_collect_low(RX_Synchronizer* self, uint8_t signal_state, uint8_t expected_count)
{
    if (!signal_state)
    {
        self->low_sample_count += 1;
    }
    else
    {
        if (!expected_count)
        {
            // No expected sample count, check against defined low and high limit
            if (self->low_sample_count >= SKEW_LOW_LIMIT && 
                self->low_sample_count <= SKEW_HIGH_LIMIT)
            {
                // We have enough low samples, return number of collected samples
                return self->low_sample_count;
            }
            else
            {
                // Too few low samples
                return (-1) * self->low_sample_count;
            }
        }
        else if (self->low_sample_count >= (expected_count - VARIATION_TOLERANCE))
        {
            // We have enough high samples, return number of collected samples
            return self->high_sample_count + self->low_sample_count; // total number of samples
        }

        self->high_sample_count += 1;

        // Check if we have too many high samples
        if (self->high_sample_count >= STATE_TOLERANCE)
        {
            return (-1) * self->high_sample_count;
        }
    }
    
    // Check if we have too many low samples

    if (!expected_count && self->low_sample_count > SKEW_HIGH_LIMIT)
    {
        return (-1) * self->low_sample_count;
    }
    else if (self->low_sample_count > (expected_count - VARIATION_TOLERANCE)) 
    {
        return (-1) * self->low_sample_count;
    }

    return 0;
}
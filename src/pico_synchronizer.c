#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"

#include "pico_synchronizer.h"
#include "rf_pico.h"
#include "debug_logging.h"

static void pico_synchronizer_process(Pico_Synchronizer* self, uint8_t signal_state);
static void pico_synchronizer_set_state(Pico_Synchronizer* self, Pico_Synchronizer_State state);

Pico_Synchronizer* global_instance; // needed due to the interrupt handlers

static void __not_in_flash_func(cancel_gpio_interrupt)()
{ 
    irq_set_enabled(IO_IRQ_BANK0, false);    
    gpio_set_irq_enabled(GPIO_PIN, 0, false);
}

static void __not_in_flash_func(gpio_int_handler)()
{
    if (global_instance->waiting_for_edge)
    {
        //global_instance->wrongcount = 0;
        // This is the starting edge
        uint64_t const current_timestamp = to_us_since_boot(get_absolute_time()); 
        if (!global_instance->start_sync_timestamp)
        {
            // Register the time for the first bit start.
            global_instance->waiting_for_edge = 0;
            global_instance->start_sync_timestamp = current_timestamp;
        }
        else
        {
            // This is the second / ending edge. Calc the total bit time
            global_instance->waiting_for_edge = 0;
            cancel_gpio_interrupt();  
            float const detected_transmission_rate = 
                (float) (current_timestamp - global_instance->start_sync_timestamp) / SYNC_LENGTH;
            if (detected_transmission_rate >= LOW_ALLOWED_TX_RATE &&
                    detected_transmission_rate <= HIGH_ALLOWED_TX_RATE )
            { 
                cancel_repeating_timer((&global_instance->timer));
                pico_synchronizer_set_state(global_instance, PICO_SYNCHRONIZER_STATE_DONE);
                rx_set_detected_transmission_rate(global_instance->rx_device, detected_transmission_rate, 0);  // we know the signal is low now. We could read this from gpio...
            }
            else
            {
                TRACE("Detected rate %.1f too high or low!", detected_transmission_rate);
                pico_synchronizer_set_state(global_instance, PICO_SYNCHRONIZER_STATE_WAIT_SYNC);
            }
        }
    }
    gpio_acknowledge_irq(GPIO_PIN, GPIO_IRQ_EDGE_FALL);
}

static void __not_in_flash_func(pico_synchronizer_register_gpio_int)()
{   
    if (!irq_is_enabled(IO_IRQ_BANK0))
    {
        gpio_set_irq_enabled(GPIO_PIN, GPIO_IRQ_EDGE_FALL, true);
        irq_set_exclusive_handler(IO_IRQ_BANK0, gpio_int_handler);
        irq_set_enabled(IO_IRQ_BANK0, true);
    }
    //global_instance->wrongcount = 0;
}

static bool pico_synchronizer_repeating_timer_callback(struct repeating_timer *t)
{
    pico_synchronizer_process((Pico_Synchronizer*) t->user_data, (uint8_t) gpio_get(GPIO_PIN));
    return true;
}

static int8_t rx_sampler_sync_collect_low(Pico_Synchronizer* self, uint8_t signal_state, uint8_t expected_count)
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
                // We won't allow any "extra" high samples
                return (-1) * self->low_sample_count;
            }
        }

        self->high_sample_count += 1;

        // Check if we have too many high samples
        if (self->high_sample_count > STATE_TOLERANCE)
        {
            return (-1) * self->high_sample_count;
        }
    }
    
    // Check if we have too many low samples
    if (!expected_count && self->low_sample_count > SKEW_HIGH_LIMIT)
    {
        return (-1) * self->low_sample_count;
    }

    if (self->low_sample_count == expected_count)
    {
        // We have enough high samples, return number of collected samples
        return self->high_sample_count + self->low_sample_count; // total number of samples
    }
    return 0;
}

static int8_t rx_sampler_sync_collect_high(Pico_Synchronizer* self, uint8_t signal_state, uint8_t expected_count)
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
                // We won't allow any "extra" high samples
                return (-1) * self->high_sample_count;
            }
        }
        self->low_sample_count += 1;

        // Check if we have too many low samples
        if (self->low_sample_count > STATE_TOLERANCE)
        {
            return (-1) * self->low_sample_count;
        }
    }

    if (self->high_sample_count == expected_count)
    {
            // We have enough high samples, return number of collected samples
            return self->high_sample_count + self->low_sample_count; // total number of samples
    }
    
    // Check if we have too many high samples
    if (!expected_count && self->high_sample_count > SKEW_HIGH_LIMIT)
    {
        return (-1) * self->high_sample_count;
    }
    return 0;
}

void pico_synchronizer_start(RX_Synchronizer* self, RX_Device* rx_device)
{
    Pico_Synchronizer * const sync = (Pico_Synchronizer*) self;
    sync->rx_device = rx_device;
    add_repeating_timer_us(SYNC_SAMPLING_RATE * -1, pico_synchronizer_repeating_timer_callback, sync, &(sync->timer));
    pico_synchronizer_set_state(sync, PICO_SYNCHRONIZER_STATE_WAIT_SYNC);
}

void pico_synchronizer_init(Pico_Synchronizer* self)
{
    memset(self, 0, sizeof(Pico_Synchronizer));
    global_instance = self;
    self->state = PICO_SYNCHRONIZER_STATE_WAIT_SYNC;
    self->state_function = NULL;
    self->base.wait_for_sync = pico_synchronizer_start;
}

void pico_synchronizer_process(Pico_Synchronizer* self, uint8_t signal_state)
{
    if (self->state_function)
    {    
        self->state_function(self, signal_state);
    }
}

// Wait for enough highs and then for the first low
static void pico_synchronizer_state_wait_sync(Pico_Synchronizer* self, uint8_t signal_state)
{
    if (signal_state && self->high_sample_count < SKEW_LOW_LIMIT)   
    {
        self->high_sample_count += 1;
    }
    else if (!signal_state && self->high_sample_count == SKEW_LOW_LIMIT)  
    {
        // We have enough high samples and we got first low, count the first low
        // and go to next state to check if we really have sync start
        pico_synchronizer_set_state(self, PICO_SYNCHRONIZER_STATE_START_SYNC);
        self->low_sample_count = 1;
    }
    else if (!signal_state)
    {
        // Got low sample so nullify the sample count  
        self->high_sample_count = 0;
        self->low_sample_count = 0;
    }
}

// Check if we have our first low and high bit pair
static void pico_synchronizer_state_start_sync(Pico_Synchronizer* self, uint8_t signal_state)
{
    int8_t res = 0;
    if (!self->processing_high)
    {
        res = rx_sampler_sync_collect_low(self, signal_state, 0); // 0 means no expected sample count at this point yet
    }
    else
    {
        res = rx_sampler_sync_collect_high(self, signal_state, self->sync_sample_count);
    }

    if (res < 0)
    {
        // Sampling error, go back to waiting for the high samples
        pico_synchronizer_set_state(self, PICO_SYNCHRONIZER_STATE_WAIT_SYNC);
    }
    else if (res > 0)
    {
        if (!self->processing_high)
        {
            // Good enough set for low samples. Store count and start getting the high samples
            self->sync_sample_count = res;
            self->high_sample_count = 1;    // We got already one high sample
            self->low_sample_count = 0;
            self->processing_high = 1;
        }
        else if (res == self->sync_sample_count)
        {
            // Low and high sample counts match, assume we have sync start. Get the time of the first low bit by
            // registering interrupt for the falling edge
            self->waiting_for_edge = 1;
            pico_synchronizer_set_state(self, PICO_SYNCHRONIZER_STATE_SYNC);
            self->processing_high = 0;
        }
        else
        {
            // No match. Go back to first state.
            pico_synchronizer_set_state(self, PICO_SYNCHRONIZER_STATE_WAIT_SYNC);
        }
    }
}

static void pico_synchronizer_state_sync(Pico_Synchronizer* self, uint8_t signal_state)
{
    if (self->waiting_for_edge)
    {
        /*
        if (!self->wrongcount)
        {
            self->wrongcount++;
           // return;
        }
        */
        // Timer triggered while waiting for edge... It's kind of a problem. Assume error in signal.
        self->waiting_for_edge = 0;
        TRACE("Timer triggered before signal edge!");
        pico_synchronizer_set_state(self, PICO_SYNCHRONIZER_STATE_WAIT_SYNC);
        return;
    }
    int8_t res = 0;

    if (self->processing_high)
    {
        res = rx_sampler_sync_collect_high(self, signal_state, self->sync_sample_count);
    }
    else
    {
        res = rx_sampler_sync_collect_low(self, signal_state, self->sync_sample_count);
    }
    if (res > 0)
    {
        self->processed_bit_count += 1;

        if (self->processed_bit_count == SYNC_LENGTH)
        {
            // Enough sync bits processed. Get the time delta.
            self->waiting_for_edge = 1;
        }
        self->high_sample_count = 0;    
        self->low_sample_count = 0;
        self->processing_high = self->processing_high ^ 1;    

    }
    else if (res < 0)
    {
        // Sampling error, go back to waiting for the high sample
        pico_synchronizer_set_state(self, PICO_SYNCHRONIZER_STATE_WAIT_SYNC);
    }
}

static void pico_synchronizer_set_state(Pico_Synchronizer* self, Pico_Synchronizer_State state)
{    
    TRACE("Setting state to %d", state);
    self->state = state;
    switch (state)
    {
        case PICO_SYNCHRONIZER_STATE_WAIT_SYNC:
            self->sync_sample_count = 0;
            self->start_sync_timestamp = 0;
            self->state_function = pico_synchronizer_state_wait_sync;
            break;
        case PICO_SYNCHRONIZER_STATE_START_SYNC:
            pico_synchronizer_register_gpio_int();
            self->processing_high = 0;
            self->waiting_for_edge = 0;
            self->state_function = pico_synchronizer_state_start_sync;
            break;
        case PICO_SYNCHRONIZER_STATE_SYNC:
            self->processing_high = 0;
            self->state_function = pico_synchronizer_state_sync;
            self->processed_bit_count = 0;
            break;
        case PICO_SYNCHRONIZER_STATE_DONE:
            self->state_function = NULL;
            break;
    }
    self->high_sample_count = 0;
    self->low_sample_count = 0;
}


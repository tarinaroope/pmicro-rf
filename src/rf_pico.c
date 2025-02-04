#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "rf_pico.h"
#include "pico_synchronizer.h"
#include "debug_logging.h"

// Callback functions

static void pico_tx_set_signal(uint8_t is_high, void* user_data)
{
    gpio_put(GPIO_PIN, (uint8_t) is_high);
}

static void pico_data_read_callback(void *user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    bool gpio_value = gpio_get(GPIO_PIN);
    rx_signal_callback(&(receiver->rx_device), (uint8_t) gpio_value);
}

static int64_t pico_tx_alarm_callback(alarm_id_t id, void *user_data) 
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    tx_callback(&(transmitter->tx_device));
    return 0;
}

static bool pico_rx_repeating_timer_callback(struct repeating_timer *t)
{
    pico_data_read_callback(t->user_data);
    return true;
}

static bool pico_tx_repeating_timer_callback(struct repeating_timer *t)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) t->user_data;
    tx_callback(&(transmitter->tx_device));
    return true;
}

static uint64_t pico_get_timestamp_us_callback()
{
    return to_us_since_boot(get_absolute_time());
}

static void pico_tx_set_onetime_trigger_time(uint64_t time_to_trigger, void* user_data)
{
    add_alarm_in_us(time_to_trigger, pico_tx_alarm_callback, user_data, true);
}

static void pico_tx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(transmitter->timer);
    add_repeating_timer_us(time_to_trigger * -1, pico_tx_repeating_timer_callback, user_data, timer);
}

static void pico_tx_cancel_trigger(void* user_data)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(transmitter->timer);
    cancel_repeating_timer(timer);
}   

static void pico_rx_cancel_trigger(void* user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(receiver->timer);
    cancel_repeating_timer(timer);
} 

static void pico_rx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(receiver->timer);
    add_repeating_timer_us(time_to_trigger * -1, pico_rx_repeating_timer_callback, user_data, timer);
}

static void pico_tx_ready_callback(void* user_data)
{
  // TODO
}

// Callback functions end

void pico_init_transmitter(rf_pico_transmitter* self)
{
    gpio_init(GPIO_PIN);
    gpio_set_dir(GPIO_PIN, GPIO_OUT);

    tx_init(&(self->tx_device), pico_tx_set_signal, pico_tx_set_onetime_trigger_time, 
            pico_tx_set_recurring_trigger_time, pico_tx_cancel_trigger, pico_tx_ready_callback, self);
}

void pico_tx_send_message(rf_pico_transmitter* transmitter, RF_Message* message)
{
    tx_send_message(&(transmitter->tx_device), message);
}

void pico_rx_start_receiving(rf_pico_receiver* self)
{
    rx_start_receiving(&(self->rx_device));
}

void pico_init_receiver(rf_pico_receiver* self, void* result_callback)
{
    gpio_init(GPIO_PIN);
    gpio_set_dir(GPIO_PIN, GPIO_IN);

    rx_init(&(self->rx_device),result_callback, pico_rx_set_recurring_trigger_time, 
            pico_rx_cancel_trigger, self);
    
    Pico_Synchronizer* synchronizer = (Pico_Synchronizer*) malloc(sizeof(Pico_Synchronizer));
    pico_synchronizer_init(synchronizer);
    rx_set_external_synchronizer(&(self->rx_device),&(synchronizer->base)); 
}

void pico_rx_stop_receiving(rf_pico_receiver* self)
{
    rx_stop_receiving(&(self->rx_device));    
}










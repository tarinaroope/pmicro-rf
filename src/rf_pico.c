#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "rf_pico.h"
#include "debug_logging.h"


void pico_tx_set_signal(uint8_t is_high, void* user_data)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    gpio_put(transmitter->gpio, (uint8_t) is_high);
}

void pico_data_read_callback(void *user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    bool gpio_value = gpio_get(receiver->gpio);
    rx_callback(&(receiver->rx_device), (uint8_t) gpio_value);
}

int64_t pico_tx_alarm_callback(alarm_id_t id, void *user_data) 
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    tx_callback(&(transmitter->tx_device));
    return 0;
}

bool pico_rx_repeating_timer_callback(struct repeating_timer *t)
{
    pico_data_read_callback(t->user_data);
    return true;
}

bool pico_tx_repeating_timer_callback(struct repeating_timer *t)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) t->user_data;
    tx_callback(&(transmitter->tx_device));
    return true;
}

void pico_tx_send_message(rf_pico_transmitter* transmitter, RF_Message message)
{
    // TODO: check initial status
    transmitter->tx_device.message = message;
    tx_callback(&(transmitter->tx_device));
}

void pico_rx_start_receiving(rf_pico_receiver* self)
{
    rx_start_receiving(&(self->rx_device));
}

void pico_rx_stop_receiving(rf_pico_receiver* self)
{
    rx_stop_receiving(&(self->rx_device));
}

uint64_t pico_get_timestamp_us_callback()
{
    return to_us_since_boot(get_absolute_time());
}

void pico_init_receiver(rf_pico_receiver* self, uint8_t gpio, void* result_callback)
{
    self->gpio = gpio;
    gpio_init(self->gpio);
    gpio_set_dir(self->gpio, GPIO_IN);

    rx_init(&(self->rx_device),result_callback, pico_rx_set_recurring_trigger_time, 
            pico_rx_cancel_trigger, self);
    rx_set_sync_mode(&(self->rx_device), SYNC_MODE_DYNAMIC, pico_get_timestamp_us_callback);
}

void pico_init_transmitter(rf_pico_transmitter* self, uint8_t gpio)
{
    self->gpio = gpio;
    gpio_init(self->gpio);
    gpio_set_dir(self->gpio, GPIO_OUT);

#ifdef USING_PICO_W
    cyw43_arch_init();
#else
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif

    tx_init(&(self->tx_device), pico_tx_set_signal, pico_tx_set_onetime_trigger_time, 
            pico_tx_set_recurring_trigger_time, pico_tx_cancel_trigger, self);
}

void pico_tx_set_onetime_trigger_time(uint64_t time_to_trigger, void* user_data)
{
    add_alarm_in_us(time_to_trigger, pico_tx_alarm_callback, user_data, true);
}

void pico_tx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(transmitter->timer);
    add_repeating_timer_us(time_to_trigger * -1, pico_tx_repeating_timer_callback, user_data, timer);
}

void pico_tx_cancel_trigger(void* user_data)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(transmitter->timer);
    cancel_repeating_timer(timer);
}   

void pico_rx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(receiver->timer);
   // TRACE("timer:%lld %ld", timer->delay_us,timer->alarm_id );

    add_repeating_timer_us(time_to_trigger * -1, pico_rx_repeating_timer_callback, user_data, timer);
}

void pico_rx_cancel_trigger(void* user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(receiver->timer);
   // TRACE("timer:%lld %ld", timer->delay_us,timer->alarm_id );

    cancel_repeating_timer(timer);
} 


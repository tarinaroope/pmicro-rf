
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "rf_pico.h"


void pico_tx_set_signal(bool is_high, void* user_data)
{
    rf_pico_transmitter* transmitter = (rf_pico_transmitter*) user_data;
    gpio_put(transmitter->gpio, is_high);
}

void pico_data_read_callback(void *user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    bool gpio_value = gpio_get(receiver->gpio);
    rx_callback(&(receiver->rx_device), gpio_value);
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

void pico_rx_start_receiving(rf_pico_receiver* this)
{
    rx_start_receiving(&(this->rx_device));
}

void pico_rx_stop_receiving(rf_pico_receiver* this)
{
    rx_stop_receiving(&(this->rx_device));
}

void pico_init_receiver(rf_pico_receiver* this, uint8_t gpio, void* result_callback)
{
    this->gpio = gpio;
    gpio_init(this->gpio);
    gpio_set_dir(this->gpio, GPIO_IN);

    rx_init(&(this->rx_device),result_callback, pico_rx_set_recurring_trigger_time, 
            pico_rx_cancel_trigger, this);
}

void pico_init_transmitter(rf_pico_transmitter* this, uint8_t gpio)
{
    this->gpio = gpio;
    gpio_init(this->gpio);
    gpio_set_dir(this->gpio, GPIO_OUT);

#ifdef USING_PICO_W
    cyw43_arch_init();
#else
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
#endif

    tx_init(&(this->tx_device), pico_tx_set_signal, pico_tx_set_onetime_trigger_time, 
            pico_tx_set_recurring_trigger_time, pico_tx_cancel_trigger, this);
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
    add_repeating_timer_us(time_to_trigger * -1, pico_rx_repeating_timer_callback, user_data, timer);
}

void pico_rx_cancel_trigger(void* user_data)
{
    rf_pico_receiver* receiver = (rf_pico_receiver*) user_data;
    repeating_timer_t* timer = (repeating_timer_t*) &(receiver->timer);
    cancel_repeating_timer(timer);
} 

void report_result(RF_Message message)
{
    printf("Received message: %llu, %x, %d\n", message.message, message.message_length, message.message_crc);
}

int main (void)
{
    stdio_init_all();
    sleep_ms(2000);
    
    /*
    rf_pico_transmitter trans;
    RF_Message message;
    message.message = 12345;
    message.message_length = 14;
    message.message_crc = 0x2222;
    pico_init_transmitter(&trans, GPIO_PIN);
    LED_PIN_PUT(LED_PIN, 1);

    sleep_ms(5000);

    LED_PIN_PUT(LED_PIN, 0);

    pico_tx_send_message(&trans, message);
    while (1)
    {
        sleep_ms(1000);
    }
    */
   
   printf("Receiver\n");
       sleep_ms(2000);

    rf_pico_receiver rec;
    pico_init_receiver(&rec, GPIO_PIN, report_result);
    sleep_ms(1000);
    pico_rx_start_receiving(&rec);
    while (1)
    {
        sleep_ms(1000);
    }    
    
}
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "rf_pico.h"

uint8_t times;
uint64_t prev;
bool got_message = false;

void report_result(RF_Message message)
{
    uint64_t const current_timestamp = to_us_since_boot(get_absolute_time());
    times = (current_timestamp - prev) / 1000000ULL;
            
    if (!got_message)
    {
        // First message from burst
        prev = current_timestamp;
        printf("Received message: %llu, %x, %d, stamp: %d\n", message.message, message.message_length, message.message_crc, times);
        got_message = true;
    }
    else if (got_message && times < 3)
    {
        // Second or third message from burst, just clear got_message
        //got_message = false;
    }
    else if (got_message && times > 3)
    {
        // First message from burst, but missed from previous burst
        prev = current_timestamp;
        printf("Received message: %llu, %x, %d, stamp: %d\n", message.message, message.message_length, message.message_crc, times);
        got_message = true;
    }

}

int main (void)
{
    timer_hw->dbgpause = 0; // hack!

    stdio_init_all();
    sleep_ms(2000);
        /*
    rf_pico_transmitter trans;
    RF_Message message;
    message.message = 12345;
    message.message_length = 14;
    message.message_crc = 0x2222;
    pico_init_transmitter(&trans);
   // LED_PIN_PUT(LED_PIN, 1);
    printf("Sending\n");

   // LED_PIN_PUT(LED_PIN, 0);

    while (1)
    {
        sleep_ms(5000);
        pico_tx_send_message(&trans, message);

    }
    */
    
    printf("Receiver\n");
    sleep_ms(1000);


    rf_pico_receiver rec;
    pico_init_receiver(&rec, report_result);
    sleep_ms(1000);
    pico_rx_start_receiving(&rec);
    
    while (1)
    {
        sleep_ms(2000);
    }    
    
}

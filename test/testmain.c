#include <stdio.h>
#include "pico/stdlib.h"
#include "rf_pico.h"

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
    pico_init_receiver(&rec, report_result);
    sleep_ms(1000);
    pico_rx_start_receiving(&rec);
    while (1)
    {
        sleep_ms(1000);
    }    
    
}

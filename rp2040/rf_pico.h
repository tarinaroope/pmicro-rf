#ifndef RF_PICO_H
#define RF_PICO_H

#include "pico/stdlib.h"
#include "rf_device.h"

#define GPIO_PIN 22

typedef struct 
{
    TX_Device tx_device;        
    repeating_timer_t timer;    

} rf_pico_transmitter;

typedef struct 
{
    RX_Device rx_device;
    repeating_timer_t timer;
} rf_pico_receiver;

/**
 * @brief Initializes the RF Pico transmitter.
 *
 * This function initializes the RF Pico transmitter by configuring GPIO pins
 * and setting up the transmitter device.
 *
 * @param self Pointer to the RF Pico transmitter structure.
 */
void pico_init_transmitter(rf_pico_transmitter* self);

/**
 * Sends a message using the RF Pico transmitter.
 *
 * @param transmitter The RF Pico transmitter.
 * @param message The message to be sent.
 */
void pico_tx_send_message(rf_pico_transmitter* transmitter, RF_Message* message);

/**
 * @brief Initializes the RF Pico receiver.
 *
 * This function initializes the RF Pico receiver by configuring GPIO pins,
 * setting up the receiver device, and initializing an external synchronizer.
 *
 * @param self Pointer to the RF Pico receiver structure.
 * @param result_callback Pointer to the callback function for receiving results.
 */
void pico_init_receiver(rf_pico_receiver* self, void* result_callback);

/**
 * @brief Starts receiving data using the RF Pico receiver.
 *
 * This function starts receiving data using the RF Pico receiver device.
 *
 * @param self Pointer to the RF Pico receiver structure.
 */
void pico_rx_start_receiving(rf_pico_receiver* self);

/**
 * @brief Stops receiving data using the RF Pico receiver.
 *
 * This function stops receiving data using the RF Pico receiver device.
 *
 * @param self Pointer to the RF Pico receiver structure.
 */
void pico_rx_stop_receiving(rf_pico_receiver* self);

#endif // RF_PICO_H

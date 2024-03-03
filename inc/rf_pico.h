#ifndef RF_PICO_H
#define RF_PICO_H

#include "pico/stdlib.h"
#include "rf_device.h"

#define GPIO_PIN 22

/**
 * @brief Structure representing a RF Pico transmitter.
 * 
 * This structure holds information about a RF Pico transmitter, including the GPIO pin, 
 * the TX device, and the repeating timer.
 */
typedef struct 
{
    TX_Device tx_device;        /**< TX device used for transmission. */
    repeating_timer_t timer;    /**< Repeating timer used for transmission. */

} rf_pico_transmitter;

/**
 * @brief Structure representing a Pico RF receiver.
 * 
 * This structure contains the necessary information for a Pico RF receiver,
 * including the GPIO pin, the RX device, and the repeating timer.
 */
typedef struct 
{
    RX_Device rx_device;
    repeating_timer_t timer;
} rf_pico_receiver;

/**
 * @brief Initializes the RF Pico transmitter.
 *
 * @param self The RF Pico transmitter.
 * @param gpio The GPIO pin to use for the transmitter.
 */
void pico_init_transmitter(rf_pico_transmitter* self);

/**
 * Sends a message using the RF Pico transmitter.
 *
 * @param transmitter The RF Pico transmitter.
 * @param message The message to be sent.
 */
void pico_tx_send_message(rf_pico_transmitter* transmitter, RF_Message message);

/**
 * @brief Callback function for the Pico RX alarm.
 *
 * This function is called when the Pico RX alarm is triggered.
 *
 * @param id The ID of the alarm.
 * @param user_data A pointer to user-defined data.
 * @return The return value is not used.
 */
void pico_init_receiver(rf_pico_receiver* self, void* result_callback);

/**
 * @brief Callback function for the Pico RX alarm.
 *
 * This function is called when the Pico RX alarm is triggered.
 *
 * @param id The ID of the alarm.
 * @param user_data A pointer to user-defined data.
 * @return The return value is not used.
 */
void pico_rx_start_receiving(rf_pico_receiver* self);

/**
 * @brief Stops the Pico RX from receiving.
 *
 * @param self The RF Pico receiver.
 */
void pico_rx_stop_receiving(rf_pico_receiver* self);

#endif // RF_PICO_H

#ifndef RF_PICO_H
#define RF_PICO_H

#include "pico/stdlib.h"
#include "rf_device.h"

// Different LEDS for different boards
#ifdef USING_PICO_W
#include "pico/cyw43_arch.h"
#define LED_PIN CYW43_WL_GPIO_LED_PIN
#define LED_PIN_PUT cyw43_arch_gpio_put
#else
#define LED_PIN 25
#define LED_PIN_PUT gpio_put
#endif

#define GPIO_PIN 22

/**
 * @brief Structure representing a RF Pico transmitter.
 * 
 * This structure holds information about a RF Pico transmitter, including the GPIO pin, 
 * the TX device, and the repeating timer.
 */
typedef struct 
{
    uint8_t gpio;               /**< GPIO pin used for transmission. */
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
    uint8_t gpio;

    RX_Device rx_device;

    repeating_timer_t timer;
} rf_pico_receiver;

/**
 * Sets the signal for pico_tx.
 *
 * @param is_high The signal value to set. True for high, false for low.
 * @param user_data A pointer to user-defined data.
 */
void pico_tx_set_signal(uint_8 is_high, void* user_data);
/**
 * @brief Callback function for reading data in the Pico device.
 *
 * This function is called when data needs to be read from the Pico device.
 * It takes a void pointer as an argument, which can be used to pass user-defined data.
 *
 * @param user_data A pointer to user-defined data.
 */
void pico_data_read_callback(void *user_data);
/**
 * @brief Callback function for the Pico TX alarm.
 *
 * This function is called when the Pico TX alarm is triggered.
 *
 * @param id The ID of the alarm.
 * @param user_data A pointer to user-defined data.
 * @return The return value is not used.
 */
int64_t pico_tx_alarm_callback(alarm_id_t id, void *user_data);
/**
 * @brief Callback function for the Pico RX repeating timer.
 *
 * This function is called when the Pico RX repeating timer expires.
 *
 * @param t Pointer to the repeating timer structure.
 * @return True if the callback was successful, false otherwise.
 */
bool pico_rx_repeating_timer_callback(struct repeating_timer *t);

/**
 * @brief Callback function for a repeating timer in the pico_tx module.
 *
 * This function is called when the repeating timer expires. It is responsible for handling the timer event
 * and performing any necessary actions.
 *
 * @param t A pointer to the repeating timer structure.
 * @return true if the timer event was handled successfully, false otherwise.
 */
bool pico_tx_repeating_timer_callback(struct repeating_timer *t);

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
void pico_rx_start_receiving(rf_pico_receiver* self);

/**
 * @brief Stops the Pico RX from receiving.
 *
 * @param self The RF Pico receiver.
 */
void pico_rx_stop_receiving(rf_pico_receiver* self);

/**
 * @brief Callback function for the Pico RX alarm.
 *
 * This function is called when the Pico RX alarm is triggered.
 *
 * @param id The ID of the alarm.
 * @param user_data A pointer to user-defined data.
 * @return The return value is not used.
 */
void pico_init_receiver(rf_pico_receiver* self, uint8_t gpio, void* result_callback);

/**
 * @brief Initializes the RF Pico transmitter.
 *
 * @param self The RF Pico transmitter.
 * @param gpio The GPIO pin to use for the transmitter.
 */
void pico_init_transmitter(rf_pico_transmitter* self, uint8_t gpio);

/**
 * @brief Initializes the RF Pico receiver.
 *
 * @param self The RF Pico receiver.
 * @param gpio The GPIO pin to use for the receiver.
 */
void pico_tx_set_onetime_trigger_time(uint64_t time_to_trigger, void* user_data);


/**
 * Sets the recurring trigger time for pico_tx.
 *
 * This function sets the time at which the pico_tx will trigger periodically.
 *
 * @param time_to_trigger The time at which the trigger should occur.
 * @param user_data      Optional user data to be passed to the trigger callback.
 */
void pico_tx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data);

/**
 * Cancels the recurring trigger for pico_tx.
 *
 * This function cancels the recurring trigger for pico_tx.
 *
 * @param user_data Optional user data to be passed to the trigger callback.
 */
void pico_tx_cancel_trigger(void* user_data);

/**
 * Sets the recurring trigger time for pico_rx.
 *
 * This function sets the time at which the pico_rx will trigger periodically.
 *
 * @param time_to_trigger The time at which the trigger should occur.
 * @param user_data      Optional user data to be passed to the trigger callback.
 */
void pico_rx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data);

/**
 * Cancels the recurring trigger for pico_rx.
 *
 * This function cancels the recurring trigger for pico_rx.
 *
 * @param user_data Optional user data to be passed to the trigger callback.
 */
void pico_rx_cancel_trigger(void* user_data);

#endif // RF_PICO_H
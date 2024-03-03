/**
 * @file rf_arduino.h
 * @brief Header file for Arduino transmitter functionality.
 * 
 * This file defines the Arduino transmitter structure and function prototypes for setting up the transmitter.
 * 
 */

#ifndef RF_ARDUINO_H
#define RF_ARDUINO_H

#define TX_PIN PB1
#define TIMER_INTERRUPT_COUNT 10

extern "C"
{
    #include "rf_device.h"
}

/**
 * @brief Structure representing an Arduino transmitter.
 * 
 * This structure holds information about the Arduino transmitter, including the TX device used for transmission,
 * the interrupt count, and a flag indicating whether the transmitter is operating in one-shot timer mode.
 */
typedef struct 
{  
    TX_Device tx_device;        /**< TX device used for transmission. */
    bool one_shot_timer;        /**< Flag indicating whether the transmitter is operating in one-shot timer mode. */
    uint8_t target_interrupt_count; /**< Target interrupt count */
    uint16_t interrupt_count;    /**< Number of interrupts triggered. */
    bool one_shot_timer_triggered; /**< Flag indicating whether the one-shot timer has been triggered. */
    bool timer_initialized;       /**< Flag indicating whether the timer has been initialized. */
} arduino_transmitter;

arduino_transmitter* get_global_transmitter();

/**
 * @brief Initializes the Arduino transmitter.
 * 
 * This function initializes the Arduino transmitter by setting up the TX pin.
 * 
 * @param self Pointer to the arduino_transmitter structure.
 * @param pin The pin number to be used for transmission.
 */
void arduino_tx_init(arduino_transmitter* self, uint8_t pin);

/**
 * @brief Sets the signal for the Arduino transmitter.
 * 
 * This function sets the signal (high or low) for the Arduino transmitter.
 * 
 * @param is_high Flag indicating whether the signal should be set to high (true) or low (false).
 * @param user_data Pointer to user-defined data.
 */

/**
 * @brief Sends a message using the Arduino transmitter.
 * 
 * This function sends a message using the Arduino transmitter.
 * 
 * @param self Pointer to the arduino_transmitter structure.
 * @param message The message to be sent.
 */
void arduino_tx_send_message(arduino_transmitter* self, RF_Message message);


#endif // RF_ARDUINO_H


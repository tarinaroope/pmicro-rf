/**
 * @file rf_arduino.h
 * @brief Header file for Arduino transmitter functionality.
 * 
 * This file defines the Arduino transmitter structure and function prototypes for setting up the transmitter,
 * setting the signal, and managing trigger times.
 */

#ifndef RF_ARDUINO_H
#define RF_ARDUINO_H

#define F_CPU 8000000UL
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
        uint8_t interrupt_count;    /**< Number of interrupts triggered. */
        bool one_shot_timer;        /**< Flag indicating whether the transmitter is operating in one-shot timer mode. */
} arduino_transmitter;

/**
 * @brief Sets up the timer for the Arduino transmitter.
 * 
 * This function initializes the timer for the Arduino transmitter and sets the time to trigger the interrupt.
 * 
 * @param self Pointer to the arduino_transmitter structure.
 * @param time_to_trigger The time in microseconds to trigger the interrupt.
 */
void setup_timer(arduino_transmitter* self, uint64_t time_to_trigger);

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
void arduino_tx_set_signal(uint8_t is_high, void* user_data);

/**
 * @brief Sets the one-time trigger time for the Arduino transmitter.
 * 
 * This function sets the one-time trigger time for the Arduino transmitter.
 * 
 * @param time_to_trigger The time in microseconds to trigger the interrupt.
 * @param user_data Pointer to user-defined data.
 */
void arduino_tx_set_onetime_trigger_time(uint64_t time_to_trigger, void* user_data);

/**
 * @brief Sets the recurring trigger time for the Arduino transmitter.
 * 
 * This function sets the recurring trigger time for the Arduino transmitter.
 * 
 * @param time_to_trigger The time in microseconds to trigger the interrupt.
 * @param user_data Pointer to user-defined data.
 */
void arduino_tx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data);

/**
 * @brief Cancels the trigger for the Arduino transmitter.
 * 
 * This function cancels the trigger for the Arduino transmitter.
 * 
 * @param user_data Pointer to user-defined data.
 */
void arduino_tx_cancel_trigger(void* user_data);

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


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

extern "C"
{
    #include "rf_device.h"
}

typedef struct 
{  
    TX_Device tx_device;        
    bool one_shot_timer;        
    uint8_t target_interrupt_count; 
    uint16_t interrupt_count;    
    bool one_shot_timer_triggered; 
    bool timer_initialized;       
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
 * @brief Sends a message using the Arduino transmitter.
 * 
 * This function sends a message using the Arduino transmitter.
 * 
 * @param self Pointer to the arduino_transmitter structure.
 * @param message The message to be sent.
 */
void arduino_tx_send_message(arduino_transmitter* self, RF_Message message);


#endif // RF_ARDUINO_H


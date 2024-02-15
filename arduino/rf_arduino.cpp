#include <avr/io.h>
#include <avr/interrupt.h>
#include "rf_arduino.h"

#define F_CPU 8000000UL
#define TX_PIN PB1
#define TIMER_INTERRUPT_COUNT 10

arduino_transmitter g_transmitter;

ISR (TIMER0_OVF_vect)      //Interrupt vector for Timer0
{
  if (g_transmitter.interrupt_count == TIMER_INTERRUPT_COUNT)
  {
    tx_callback(&(g_transmitter.tx_device));
    g_transmitter.interrupt_count;
    if (g_transmitter.one_shot_timer)
    {
      TCCR0B=0x00;
    }
  }
  else
  {
    g_transmitter.interrupt_count++; 
  }
}

void setup_timer(arduino_transmitter* self, uint64_t time_to_trigger)   
{
  self->interrupt_count = 0;
  
  TCCR0A = 0x00;
  TCCR0B = 0x00;
  TCCR0B |= (1<<CS01);   //prescaling with 8
  TCCR0A |= (1<<WGM01);//toggle mode and compare match  mode
  OCR0A = (time_to_trigger / TIMER_INTERRUPT_COUNT); // E.g. 1000us / 10 -> 100us * 10 times interrupt = 1000us
  TCNT0 = 0;
  //sei();   //enabling global interrupt
  TIMSK |= (1<<OCIE0A); 
}

void arduino_tx_init(arduino_transmitter* self, uint8_t pin)
{
  DDRB |= (1 << TX_PIN);			//replaces pinMode(TX_PIN, OUTPUT);

  tx_init(&(self->tx_device), arduino_tx_set_signal, arduino_tx_set_onetime_trigger_time, 
          arduino_tx_set_recurring_trigger_time, arduino_tx_cancel_trigger, self);
}

void arduino_tx_set_signal(uint8_t is_high, void* user_data)
{
  if (is_high)
  {
    PORTB |= (1 << TX_PIN);			//replaces digitalWrite(PB3, HIGH);
  }
  else
  {
    PORTB &= ~(1 << TX_PIN);			//replaces digitalWrite(PB3, LOW);
  }
}

void arduino_tx_set_onetime_trigger_time(uint64_t time_to_trigger, void* user_data)
{
  arduino_transmitter* tx = (arduino_transmitter*) user_data;
  tx->one_shot_timer = true;
  setup_timer(tx, time_to_trigger);
}

void arduino_tx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data)
{
  arduino_transmitter* tx = (arduino_transmitter*) user_data;
  tx->one_shot_timer = false;
  setup_timer(tx, time_to_trigger);
}

void arduino_tx_cancel_trigger(void* user_data)
{
  TCCR0B=0x00;
  arduino_transmitter* tx = (arduino_transmitter*) user_data;
  tx->interrupt_count = 0;
} 

void arduino_tx_send_message(arduino_transmitter* self, RF_Message message)
{
    // TODO: check initial status
    self->tx_device.message = message;
    tx_callback(&(self->tx_device));
}


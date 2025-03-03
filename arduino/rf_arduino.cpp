#include <arduino.h>
#include <avr/interrupt.h>
#include "rf_arduino.h"

arduino_transmitter g_transmitter;

void transmit_ready_callback(void* user_data)
{
  arduino_transmitter* tx = (arduino_transmitter*) user_data;
  tx->transmitting = false; 
}

ISR (TIMER1_COMPA_vect)      //Interrupt vector for Timer1
{ 
  // Increment count only in case of recurring timer or if one-shot timer has not been triggered
  if (!g_transmitter.one_shot_timer || !g_transmitter.one_shot_timer_triggered)
  {
    g_transmitter.interrupt_count++;
  }
  if (g_transmitter.interrupt_count == g_transmitter.target_interrupt_count)
  {
    if (g_transmitter.one_shot_timer)
    {
      // In case of one-shot timer, flag that timer has been triggered
      g_transmitter.one_shot_timer_triggered = true;
    }
    tx_callback(&(g_transmitter.tx_device));
    g_transmitter.interrupt_count = 0;
  }
}

static void setup_timer(arduino_transmitter* self, uint64_t time_to_trigger)   
{
  self->interrupt_count = 0;
  self->target_interrupt_count = time_to_trigger / 100; // 100us per interrupt

  if (!self->timer_initialized)
  {
    cli();
    TCCR1 = 0; // Stop timer
    TCNT1 = 0; // Zero timer
    GTCCR = _BV(PSR1); // Reset prescaler
    OCR1A = 204; // Target to trigger every 100us. Seems that this varies based on voltage and temperature...
    OCR1C = 204; // Set to same value to reset timer1 to 0 after a compare match
    TIMSK = _BV(OCIE1A); // Interrupt on compare match with OCR1A
  
    // Start timer in CTC mode; prescaler = 4; 
    //TCCR1 = _BV(CTC1) | _BV(CS11) | _BV(CS10); 
    TCCR1 = _BV(CTC1) | _BV(CS12) | _BV(CS10); 
    sei();
    self->timer_initialized = true;
  }
}

arduino_transmitter* get_global_transmitter()
{
  return &g_transmitter;
}

static void arduino_tx_set_signal(uint8_t is_high, void* user_data)
{
  if (is_high)
  {
    PORTB |= (1 << TX_PIN);			// HIGH
  }
  else
  {
    PORTB &= ~(1 << TX_PIN);			// LOW
  }
}

static void arduino_tx_set_onetime_trigger_time(uint64_t time_to_trigger, void* user_data)
{
  arduino_transmitter* tx = (arduino_transmitter*) user_data;
  tx->one_shot_timer = true;
  tx->one_shot_timer_triggered = false;
  setup_timer(tx, time_to_trigger);
}

static void arduino_tx_set_recurring_trigger_time(uint64_t time_to_trigger, void* user_data)
{
  arduino_transmitter* tx = (arduino_transmitter*) user_data;
  tx->one_shot_timer = false;
  setup_timer(tx, time_to_trigger);
}

static void arduino_tx_cancel_trigger(void* user_data)
{
  TCCR1 = 0;
  TIMSK = 0;
  TIFR = 0;
  arduino_transmitter* tx = (arduino_transmitter*) user_data;
  tx->interrupt_count = 0;
  tx->timer_initialized = false;
} 

void arduino_tx_send_message(arduino_transmitter* self, RF_Message* message)
{
  tx_send_message(&(self->tx_device), message);
  self->transmitting = true;
}

void arduino_tx_init(arduino_transmitter* self, uint8_t pin)
{
  DDRB |= (1 << TX_PIN);			//replaces pinMode(TX_PIN, OUTPUT);
  tx_init(&(self->tx_device), arduino_tx_set_signal, arduino_tx_set_onetime_trigger_time, 
          arduino_tx_set_recurring_trigger_time, arduino_tx_cancel_trigger, transmit_ready_callback, self);
}

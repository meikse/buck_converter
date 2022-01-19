#include <avr/io.h>
#include <avr/interrupt.h>

// define pin I/O
#define pwm PB0         
#define feedback PB2    
#define ref PB4

int init_FastPWM(void){
  TCCR0A = 0x00;
  TCCR0B = 0x00;  
  TCCR0A |= (1<<WGM00) | (1<<WGM01);   // Fast PWM
  TCCR0A |= (1<<COM0A1);               // Clear OC0A on Compare Match (PB0)
  TCCR0A |= (1<<COM0B1);               // Clear OC0B on Compare Match (PB1) (not neccessary)
  TCCR0B |= (1<<CS00);// | (1<<CS01);  // no Prescaler
}

int init_ADC(void){
  ADMUX = 0x00;                        // Vcc as analog reference, right adjust result
  ADCSRA = 0x00;            
  ADCSRA |= (1<<ADEN);                 // enable ADC
  ADCSRA |= (1<<ADPS0) | (1<<ADPS2);   // Prescaler 32
  ADCSRB |= 0x00;                      // Free running mode
}

int dummy_Read(void){
  int x;
  ADCSRA |= (1<<ADSC);                 // start conversion
  while (ADCSRA &(1<<ADSC));           // ADSC as long as conversion is in progress (see datasheet)
  x = ADC;                             // remove first entry because of init
  return 0;
}

int Read(void){
    ADCSRA |= (1<<ADSC);               // start conversion
    while (ADCSRA &(1<<ADSC));         // ADSC as long as conversion is in progress (see datasheet)
    return ADC;                        // return analog value
}

int clk_frequency(void){
  cli();                               // disable interrupts
  OSCCAL = 0x00;                       // normal Oscillator Frequency range
  CLKPR = (1<<CLKPCE);                 // enbale Prescaler
  CLKPR = (0<<CLKPS3) | (0<<CLKPS2) | (0<<CLKPS1) | (0<<CLKPS0); // no Prescaler
  sei();                               // enable interrupts 
}

int main(void){

  DDRB = ~(1<<ref) & ~(1<<feedback);   // PB2 & PB4 as inputs
  DDRB |= (1<<pwm);                    // pb0 as output 

  clk_frequency();                     // set right clk frequency
  init_FastPWM();                      // initialize Fast PWM for pb0 (for duty cycle)
  init_ADC();                          // initialize the ADC 
  dummy_Read();                        // read out the first value of the analog input and trash it

  while(1){
    ADMUX = (1<<MUX1);                 //ADC2 (PB4)
    int ref_value = Read();
    ADMUX = (1<<MUX0);                 //ADC1 (PB2)
    int feedback_value = Read();

    // simple P-controller
    if (ref_value < feedback_value && OCR0A < 255){
      OCR0A++;                         //inverted because of npn-BJT 
    }
    else if (ref_value > feedback_value && OCR0A > 0){
      OCR0A--;                         //inverted because of npn-BJT
    }
    else{
      OCR0A = OCR0A;                   // hold
    }      
  }  
}

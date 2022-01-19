# buck converter

buck converter utilized with an attiny13

---

the fact that i had laying around some transformators from old gadgets, gave me the thought to build my own power supply. As the input i have a transformator who transform 230V to 15V AC. the circuit already involves a full bridge rectifier.\
Because I want to power a LED strip with 12V. I need to step down the supply voltage.
All in all the resistances could be otimized, to draw less current. Because this project took to long I had no intentions to do so.

## circuit (hardware)
the circuit involves a constant voltage source (LM1085) an amplifier (LM358) the buck converter with an an P-channel MOSFET (IRF9Z24NPBF) and of course the controller (attiny13). 

![circuit](circuit_design.svg)

### voltage source
the voltage source, to generate constant 4.7V for the controller could also be used to transform the 15V into 12V. Here the LM1085 just converts the voltage drop into dissipation with heat. because the controller only draws a small current the dissipation isnt as high as using the LM1085 as the main device to achieve the voltage drop from 15V to 12V. For achieving the right output voltage with this device look at its datasheet.

### buck converter circuit
The circuit is build up like an usual buck converter topology. 
Additionally a BJT (npn) is neccessary to generate 15V or 0V at the base os the MOSFET, because the attiny13 only provides a max voltage of 4.7V. 
The inductance of the coil is unknown. I desoldered it from an old device. Shouldnt be to small. more important is the capacitor, it stores the energy. Shall be chosen high enough because the LED strip draws 0.5 - 1 A current. dont forget to place the diode correctly. A pull-up resistor is mandatory for the MOSFET. Important, the BC547 npn-Transistor (which I have plenty of) didnt work out at high frequencies, i tested a desoldered one and it worked. check if the BJT suits the circuit!

Remember: always prefer to use BJTs and MOSFETs at the end (n-channel, npn) or at the begin (p-channel, pnp) to achieve a max or min voltage between $U_GS$ or $U_BE$. I.e. a resistance between the collector and ground (no resistance anywhere else) would achieve the complete voltage drop along the resistance, therefor the BJT would not draw current at all. Additionally, p-channel MOSFET have negated values in there datasheet, because source and drain are switch. To let the MOSFET conduct always care about the $U_GS$. With a p-channel MOSFET the source is on top. Thus, the top pin and base pin need to have a potential difference to let the p-channel MOSFET conduct. ALWAYS DOUBLE CHECK CONNECTIONS AND PLACEMENT OF THE SEMICONDUCTOR. Wasted to much time with the wrong connections on the p-channel MOSFET.

### amplifier
The amplifier is not mandatory but used for safety reasons. First of all the voltage divider decrease the voltage very small and afterwards the amplifier increases the voltage again to the double. Hence, a small voltage gap can be amplified to get a larger range in measurement. the output of the opamp will be fedback to the controller

### controller 
the controller measures the reference value as the setpoint and the feedback value from the buck converter threw the amplifier with an ADC conversion. The Difference between both potentials can be used as an error to negligect. Depending on the error value. The duty cycle of the PWM signal, which drives the BJT and therefor the MOSFET, will be adapted.

## software

First of all. The Arduino can be used as an ISP to flash other µc. It is all provided in the Arduino IDE. Just search for it the internet. Unfortunatly, the file type is an `.ino` format, but actually is a c-code. as long as the `.hex` file is compiled just uploaded with avrdude. the `.hex` is hidden in the %appdata% folder (with arduino15).
To flash the attiny13 in the arduino IDE you need to follow some step which are also provided on the internet. (just install some libaries and burn bootloader, etc)

the software contains:
- Fast PWM 
- ADC conversion
- internal oscillator configuration
- p-controller

code:
```c
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
```

I have no idea if its neccessary to define the pins inside the program because the registers in the code already define exactly which pin does what the cycle. Anyway, just define them and declare them as input/output. The fast-PWM is only for the OC0A Pin on PB0 at the attiny13. No Prescaler is used to get high frequencies. The OCR0A byte is used later used for the controller. The `init_ADC()` function is just used to init the ADC conversion, the pin which shall be read out will be defined in the loop later on. You also need a dummy Readout because the first readout is mostly shit (refer literatur; Heimo Gaicher, AVR Programming). The default clock frequency is too slow, just 5 kHz. because the coil is whining we need a frequency above 20 kHz. Therefor the `clk_frequency()` function increases it. Attention: the BJT has to follow that frequency, too!
Here we get approx. 24 kHz which isnt hearbale anymore. After initialization in the `int main()` the while loop contains the ADC conversion for the pin PB4 and following for the pin PB2. the conditions in the if-else-statements test if feedback value is larger or smaller than the constant reference value.
Depending which statement. Inside the loop the if-else-statements increases or decreases OCR0A value which leads to an earlier or later overflow of interrupt count register. With that, the duty cycle of the Fast PWM changes depending on the OCR0A value. Remember, one is changing the OCR0A inside the loop the interrupt routine exists in parallel to the loop and hits whenever the OCR0A overflows.

## conlusion
p-controller is sufficient here, even though voltage jumps around +- 0.5 V at 12V. LED are still bright. 

#include <avr/io.h>
#include <avr/interrupt.h>
#define TWI_WAIT()     while (!(TWCR & (1<<TWINT)));
#define TWI_CHECK()    (TWCR & (1<<TWINT))

#define I2C_ADDRESS 0x71

#define PSCSR  7
#define PSCSRT 6
#define PSCSB  5
#define PSCSBT 4
#define PSFE   3
#define PSGC   2
#define PSSE   1
#define PSLE   0

#define PSSR   0x00
#define PSCR   0x01
#define PSCB   0x02
#define PSCW   0x03
#define PSCG   0x04
#define PSFT   0x05
#define PSFPR  0x06
#define PSFPB  0x07
#define PSFPC  0x08
#define PSFP   0x09
#define PSF2P  0x0A

//#define DEF_MER_STAT 0xFF
#define DEF_MER_STAT 0x40

unsigned char regs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
unsigned char answr=0;
unsigned char update=0;
unsigned char mereni=0;
unsigned char mereni_status=0;
unsigned char mereni_color=0;

inline unsigned char adc_read(unsigned char input_pin) {
  ADMUX = (1<<REFS0) | (1<<ADLAR) | (input_pin & 0x07);
  ADCSRA = (1<<ADEN) | (1<<ADSC);
  while (!(ADCSRA & (1<<ADIF)))
    ;
  ADCSRA |= (1<<ADIF);
  return ADCH;
}

ISR(INT0_vect) {
  if (mereni_status) {
    if (PORTD & _BV(PD2))
      TCNT0 = 0; // clear counter
    else {
//      mereni = TCNT0;
      mereni = adc_read(0);//debug ADC only
    }
    mereni_status--;
  }
  else if (mereni_color) {
    mereni_status = DEF_MER_STAT;
    regs[mereni_color+PSCR] = mereni;
//    regs[mereni_color+PSCR] = adc_read(0);//debug ADC only
    mereni_color--;
    PORTD &= 0xFC & (~mereni_color);
    PORTD |= 0x03 & mereni_color;
  }
  unsigned char debug = (OCR1B - 522) >> 1;
  debug--;
  OCR1B = 522 + (debug<<1);
}

inline void mer() {
  if (mereni_status) {
    if (PORTD & _BV(PD2))
      TCNT0 = 0; // clear counter
    else {
//      mereni = TCNT0;
      mereni = adc_read(0);//debug ADC only
    }
    mereni_status--;
  }
  else if (mereni_color) {
    mereni_status = DEF_MER_STAT;
    regs[mereni_color+PSCR] = mereni;
//    regs[mereni_color+PSCR] = adc_read(0);//debug ADC only
    mereni_color--;
    PORTD &= 0xFC & (~mereni_color);
    PORTD |= 0x03 & mereni_color;
  }
  unsigned char debug = (OCR1B - 522) >> 1;
  debug--;
  OCR1B = 522 + (debug<<1);
}

inline void update_status() {
  update=0;
  if (regs[PSSR] & (1<<PSCSR)) {
    //todo
  }
  if (regs[PSSR] & (1<<PSCSRT)) {
    //todo
  }
  if (regs[PSSR] & (1<<PSCSB)) {
    //todo
  }
  if (regs[PSSR] & (1<<PSCSBT)) {
    //todo
  }
  if (regs[PSSR] & (1<<PSFE)) { //servo enable
    ICR1 = 10347;
    TCCR1A = (1<<COM1A1) | (1<<COM1B1);
    TCCR1B = (1<<WGM13) | (1<<CS11);
    DDRB |= _BV(PB1) | _BV(PB2);
  }
  else {
    DDRB &= (~_BV(PB1)) & (~_BV(PB2));
    TCCR1A = 0;
    TCCR1B = 0;
  }
  if (regs[PSSR] & (1<<PSGC)) { //get color
    
    regs[PSSR] &= ~(1<<PSGC);
    TCCR0 = (1<<CS02) | (1<<CS00);//todo pouze pro ladeni 8MHz/1024 =8kHz
//    TCCR0 = (1<<CS01) | (1<<CS00);//8MHz/64 = 128kHz // bad
    //taos:
    //  S0 = H, S1 = L | 100 | 120 | kHz
//    TCCR0 = (1<<CS01);8MHz/8 = 1kHz //good
    //taos:
    //  S0 = H, S1 = H | 500 | 600 | kHz
    TCNT0 = 0; // clear counter

    cli();
    MCUCR |= (1<<ISC00);
    GICR |= (1<<INT0);
    mereni_status=DEF_MER_STAT;
    mereni_color=0x03;
    PORTD |= 0x03;
    sei();
  }
  if (regs[PSSR] & (1<<PSLE)) { //light on
    PORTB |= _BV(PB0);
    DDRB  |= _BV(PB0);
  }
  else {
    DDRB  &= ~_BV(PB0);
    PORTB &= ~_BV(PB0);
  }
}

int main(void) {
  OCR1A = 778;//default servo position
  OCR1B = 778;//default servo position

  DDRD |= 0x03; //enable sensor color select
  TWAR = I2C_ADDRESS << 1;
  TWCR = (1<<TWEA) | (1<<TWEN); //enable twi

  int counter = 100;
  long int cnt2=20000;
  while(1) {
    if (!counter--) {
      mer();
      counter=100;
    }
    if ((regs[PSSR] & (1<<PSSE)) && (!cnt2--)) {
      cnt2=20000;
      if ( regs[PSCG] < 0x38)
        OCR1A = 522 + (regs[PSFPC]<<1);
      else if(regs[PSCB] > regs[PSCW])
        OCR1A = 522 + (regs[PSFPB]<<1);
      else
        OCR1A = 522 + (regs[PSFPR]<<1);
      
      regs[PSSR] |= (1<<PSGC);
      update=1;
    }
    if ((!mereni_status) && (!mereni_color)) {
      cli();
      regs[mereni_color+PSCR] = mereni;
    }
    if (update)
      update_status();
    if (regs[PSSR] & (1<<PSSE)) {
      //check timer -> sort
    }
    if (TWI_CHECK()) {
      if ((TWSR & 0xF8) == 0x60) { //slave receive
        TWCR |= (1<<TWINT); //clear TWINT
        TWI_WAIT();

        unsigned char reg=0xFF;
        if ((TWSR & 0xF8) == 0x80) {
          reg = TWDR;
          TWCR |= (1<<TWINT); //clear TWINT
          TWI_WAIT();
        }

        if ((TWSR & 0xF8) == 0x80) {
          if (reg <= PSFPC)
            regs[reg] = TWDR;
          if (reg == PSFP)
            OCR1A = 522 + (TWDR<<1);
          if (reg == PSF2P)
            OCR1B = 522 + (TWDR<<1);
          if (reg == PSSR)
            update=1;
          TWCR &= ~(1<<TWEA); //disable ACK
          TWCR |= (1<<TWINT);
          TWI_WAIT();
        }
        else
          answr = reg;

        if ((TWSR & 0xF8) == 0x88)
          TWCR |= (1<<TWINT) | (1<<TWEA);

        if ((TWSR & 0xF8) == 0xA0)
          TWCR |= (1<<TWINT) | (1<<TWEA);
      }
      else if ((TWSR & 0xF8) == 0xA8) { //slave transmitt
        if (answr <= PSFPC)
          TWDR = regs[answr];
        if (answr == PSFP)
          TWDR = (OCR1A - 522) >> 1;
        if (answr == PSF2P)
          TWDR = (OCR1B - 522) >> 1;
        TWCR &= ~(1<<TWEA); //disable ACK
        TWCR |= (1<<TWINT); //clear TWINT
        TWI_WAIT();

        if ((TWSR & 0xF0) == 0xC0) //0xC0 or 0xC8
          TWCR |= (1<<TWINT) | (1<<TWEA);
      }
      if ((TWSR & 0xF8) == 0x00)
        TWCR |= (1<<TWINT) | (1<<TWSTO) | (1<<TWEA);
    }
  }
}

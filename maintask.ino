#include <inttypes.h>
#include<avr/io.h>
#include<util/delay.h>
#include <stdio.h>
#include <string.h>
#include<stdlib.h>
#include<avr/interrupt.h>


// commands
#define LCD_CLEARDISPLAY 0x01

#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET 0x20

#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode

#define LCD_ENTRYLEFT 0x02

#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04


#define LCD_CURSOROFF 0x00

#define LCD_BLINKOFF 0x00

// flags for display/cursor shift


uint8_t _numlines,_currline;

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00
class LiquidCrystal{
public:

  LiquidCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
    uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3);
  

  void init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
      uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
      uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7);
    
  
  void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);

  void clear();
 
void display();
 
  void setCursor(uint8_t, uint8_t); 
// virtual size_t write(uint8_t);
  void command(uint8_t);
  void disp(uint8_t value);
private:
  void send(uint8_t, uint8_t);
 void write4bits(uint8_t);
  
  void pulseEnable();
  
  
  uint8_t _rs_pin; // LOW: command.  HIGH: character.
  uint8_t _rw_pin; // LOW: write to LCD.  HIGH: read from LCD.
  uint8_t _enable_pin; // activated by a HIGH pulse.
  uint8_t _data_pins[8];
  uint8_t _displayfunction;
  uint8_t _displaycontrol;
  uint8_t _displaymode;

    
};
LiquidCrystal::LiquidCrystal(uint8_t rs, uint8_t rw, uint8_t enable,
           uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)
{
  init(1, rs, rw, enable, d0, d1, d2, d3, 0, 0, 0, 0);
}



void LiquidCrystal:: init(uint8_t fourbitmode, uint8_t rs, uint8_t rw, uint8_t enable,
      uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
       uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7)
{
  _rs_pin = rs;
  _rw_pin = rw;
  _enable_pin = enable;
  
  _data_pins[0] = d0;
  _data_pins[1] = d1;
  _data_pins[2] = d2;
  _data_pins[3] = d3; 
  _data_pins[4] = d4;
  _data_pins[5] = d5;
  _data_pins[6] = d6;
  _data_pins[7] = d7; 
    
  DDRB|=(1<<(_rs_pin-8))|(1<<(_enable_pin-8))|(1<<(_rw_pin-8));  // we can save 1 pin by not using RW. Indicate by passing 255 instead of pin#
 
  _displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;  
  begin(16, 1);
  
 
}

void LiquidCrystal::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
  if (lines > 1) {
    _displayfunction |= LCD_2LINE;
  }
  _numlines = lines;
  _currline = 0;

  // for some 1 line displays you can select a 10 pixel high font
  if ((dotsize != 0) && (lines == 1)) {
    _displayfunction |= LCD_5x10DOTS;
  }

  // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way befer 4.5V so we'll wait 50
  _delay_us(50000); 
  // Now we pull both RS and R/W low to begin commands
  
  PORTB&=~((1<<(_rs_pin-8))|(1<<(_enable_pin-8)));

  if (_rw_pin != 255) { 
   
    PORTB&=~(1<<(_rw_pin-8));
  }
  
  //put the LCD into 4 bit or 8 bit mode
  if (! (_displayfunction & LCD_8BITMODE)) {
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    write4bits(0x03);
    _delay_us(4500); // wait min 4.1ms

    // second try
    write4bits(0x03);
    _delay_us(4500); // wait min 4.1ms
    
    // third go!
    write4bits(0x03); 
    _delay_us(150);

    // finally, set to 4-bit interface
    write4bits(0x02); 
  } else {
    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    command(LCD_FUNCTIONSET | _displayfunction);
    _delay_us(4500);  // wait more than 4.1ms

    // second try
    command(LCD_FUNCTIONSET | _displayfunction);
    
 _delay_us(150);

    // third go
    command(LCD_FUNCTIONSET | _displayfunction);
  }

  // finally, set # lines, font size, etc.
  command(LCD_FUNCTIONSET | _displayfunction);  

  // turn the display on with no cursor or blinking default
  _displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
  display();

  // clear it off
  clear();

  // Initialize to default text direction (for romance languages)
  _displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  command(LCD_ENTRYMODESET | _displaymode);

}

void LiquidCrystal::clear()
{
  command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  _delay_us(2000);  // this command takes a long time!
}

void LiquidCrystal::setCursor(uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
  if ( row > _numlines ) {
    row = _numlines-1;    // we count rows starting w/0
  }
  
  command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}
void LiquidCrystal::display() {
  _displaycontrol |= LCD_DISPLAYON;
  command(LCD_DISPLAYCONTROL | _displaycontrol);
}
inline void LiquidCrystal::command(uint8_t value) {
  send(value, 0);
}
inline void LiquidCrystal::disp(uint8_t value) {
  send(value, 1);
}


// write either command or data, with automatic 4/8-bit selection
void LiquidCrystal::send(uint8_t value, uint8_t mode) {

    
    if(mode==1)
      PORTB|=(1<<(_rs_pin-8));
    else
      PORTB&=~(1<<(_rs_pin-8));

    // if there is a RW pin indicated, set it low to Write
    if (_rw_pin != 255) { 
      
      PORTB&=~(1<<(_rw_pin-8));
    }
    
    
      write4bits(value>>4);
      write4bits(value);
    
 

}

void LiquidCrystal::pulseEnable(void) {
 
    PORTB&=~(1<<(_enable_pin-8));
    _delay_us(1);    
   
    PORTB|=(1<<(_enable_pin-8));
    _delay_us(1);       // enable pulse must be >450ns
    //digitalWrite(_enable_pin, LOW);
    PORTB&=~(1<<(_enable_pin-8));
    _delay_us(100);       // commands need > 37us to settle
 
 
}

void LiquidCrystal::write4bits(uint8_t value) {
 
    for (int i = 0; i < 4; i++) {
     
     DDRD|=(1<<_data_pins[i]);
     if((value >> i) & 0x01)
      PORTD|=(1<<_data_pins[i]);
     else
      PORTD&=~(1<<_data_pins[i]);
    
  }

  pulseEnable();
}//////////////////////////////////lcd code ends/////////////////

LiquidCrystal lcd(12, 8, 9, 7, 6, 5, 4);

void initADC()
{
  ADMUX|=(1<<REFS0);
  ADMUX|=(1<<ADLAR);
  ADCSRA|=(1<<ADEN)|(1<<ADPS1)|(1<<ADPS2)|(1<<ADPS0);
  }

  uint16_t ReadADC(uint8_t ch)
  {
    ch=ch&0b00000111;
    ADMUX|=ch;
    
    ADCSRA|=(1<<ADSC);
    while(!(ADCSRA&(1<<ADIF)));
    ADCSRA|=(1<<ADIF);
    return (ADCH);
    }
    


int high=2,b0=0,b1=0;

int m3=40,hr3=11,flag3=0,alarmon=0,buzzeron=0;
int ms=0,s=50,m=59,hr=23,dd=28,mm=2,yy=2017,flag=0;
int ms1=0,s1=0,m1=0,hr1=0,flag1=0;
void buzzer()
{
  for(int i=100;(i<=255&&buzzeron==1);i++)
    { OCR2A=0;
    _delay_ms(100);
   
    OCR2A=i;
    _delay_ms(100);
    }
}
 
void prnt3()
{
   lcd.setCursor(0, 0);

    if(hr3<10)
   lcd.disp('0');
   disp1(hr3);
   usart_send(':');lcd.disp(':');

    if(m3<10)
   lcd.disp('0');
   disp1(m3);
   usart_send(' ');lcd.disp(' ');
   
   disp1(alarmon);
   usart_send(' ');
  
}

void usart_int(void)
{ UCSR0B=(1<< RXEN0)|(1<< TXEN0); //TRANSMIT AND RECEIVE ENABLE 
  UCSR0C=(1<<UCSZ01)|(1<<UCSZ00);//ASYNCHRONOUS, 8 BIT TRANSFER 
  UBRR0L= 0x67 ; //BAUD RATE 9600 
  UCSR0A= 0x00;
}

void usart_send(int ch ) 
{
 while(UCSR0A!=(UCSR0A|(1<<UDRE0)));//waiting for UDRE to become high
 UDR0= ch;
 UCSR0A |= 1<<UDRE0;
 
 }
void disp(int x)
{  int m,y=0,k;
   k=x;
   if (x==0)
   {
     usart_send('0');
     lcd.disp('0');
   }
   else
   { while(k)
     {
      m=k%10;
      y=y*10+m;
      k/=10;
     }
      while(y)
      {m=y%10;
       char c=m+48;
       usart_send(c);
       lcd.disp(c);
       y/=10;
      }
    if(x%10==0)
       {usart_send('0');
       lcd.disp('0');
       }
   }  
}
void disp1(int x)
{
  char b[5];
  itoa(x,b,10);
  int i=0;
  while(b[i])
   { usart_send(b[i]);
       lcd.disp(b[i]);
       i++;
   }
  
}

void prnt()
{
  lcd.setCursor(0, 0);
  if(dd<10)
   lcd.disp('0');
   disp1(dd);
   usart_send('/');   lcd.disp('/');
   
   if(mm<10)
   lcd.disp('0');
   disp1(mm);
   usart_send('/');  lcd.disp('/');
   
   disp1(yy);
   usart_send(' ');
   usart_send(' ');
  
   lcd.setCursor(0, 1);   
   if(hr<10)
   lcd.disp('0');
   disp1(hr);
   
   usart_send(':');  lcd.disp(':');
   
  if(m<10)
   lcd.disp('0');
   disp1(m);
   usart_send(':');  lcd.disp(':');
  if(s<10)
   lcd.disp('0');
   disp1(s);
   
   usart_send(' ');
   
}
void lcdstring(char* c)
{
  while(*c)
  {
    lcd.disp(*c);
    c++;
  }
}
void prnt1()
{
    
    lcd.setCursor(0, 0);
     if(hr1<10)
   lcd.disp('0');
   disp1(hr1);
   usart_send(':');lcd.disp(':');
   
    if(m1<10)
   lcd.disp('0');
   disp1(m1);
   usart_send(':');lcd.disp(':');
   
    if(s1<10)
   lcd.disp('0');
   disp1(s1);
   usart_send(':');lcd.disp(':');

    if(ms1<10)
   lcd.disp('0');
   disp1(ms1);
   usart_send(' ');
   usart_send(' ');
   usart_send('\n');

}
int main()
{ 
  
  DDRB=0xFF;
  
  PORTB|=0b00000000;
  DDRD&=~((1<<2)|(1<<3));
  int x;
  initADC();   
  usart_int();
 TCNT1=0X00;
 TCCR1A=0X00;
 TCCR2A|=(1<<COM2A1)|(1<<WGM21)|(1<<WGM20);
  TCCR2B|=(1<<CS20);
  
  TCCR1B=(1<<WGM12)|(1<<CS11)|(1<<CS10);
  OCR1A=250;
   sei();
  TIMSK1|=(1<<OCIE1A); 
  sei();
  TCNT0=0X00;
 TCCR0A=0X00;
 
 
  OCR0A=250;
   
  TIMSK0|=(1<<OCIE0A);
   lcd.begin(16, 2);
   EIMSK|=(1<<INT0);
   EIMSK|=(1<<INT1);//setting pin pd2 for accessing the interrupts
  EICRA|=(1<<ISC01)|(1<<ISC11);//set the interrupt mode (falling edge for both)*/
  while(1)
  {
    OCR2A=0;
      if(hr==hr3&&m==m3&&alarmon==1)
      { buzzeron=1;
        buzzer();
        while(alarmon)
        {
          OCR2A=255;
          _delay_ms(100);
          OCR2A=0;
           _delay_ms(100);
        }
      }
      
    x =ReadADC(0);//read analog value from channel 0
    
    if(x>=250)
      high=1;
    if(high==1&&x<=10)
      {b0++;
      b1=0;
      
    lcd.setCursor(0, 0);
    lcdstring("                ");
     lcd.setCursor(0, 1);
    lcdstring("                ");
       high=0;
       if(b0==4)
       b0=0;
      }
   switch(b0)
   {
    case 0:prnt();
            
           break;
    case 1:prnt1();
    
           break;
    case 2:prnt ();
           break;
    case 3: prnt3();  
           
    
   }
   
 
  
  }
 }
 ISR(TIMER1_COMPA_vect)
{
  ms++;
 if(ms==1000)
 {
  s++;
  ms=0;
  if(s==60)
  {
    m++;
    s=0;
    if(m==60)
    {
      hr++;
      m=0;
      if(hr==24)
      {
        dd++;
        hr=0;
      }
    }

  }
 }
if(dd==32&&mm==10)
 {mm++;
  dd=1;
 }
 switch(mm)
  {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:if(dd==32)
                   {
                    mm++;
                    dd=1;
                   }
                   break;
          case 4:
          case 6:
          case 9:
          case 11:if(dd==31)
                   {
                    mm++;
                    dd=1;
                   }
                   break;
          case 2:if(yy%100==0)
                {if(yy%400==0&&dd==30)
                  {
                    mm++;
                    dd=1;
                  }
                  }
                  else if(yy%4==0&&dd==30)
                  {
                    mm++;
                    dd=1;
                  }
                                
                  
                  else if(dd==29)
                  {
                    mm++;
                    dd=1;
                  }
        }     
     if(mm==13)
        {
          yy++;
          mm=1;
        }
      
    
  }
  
ISR(TIMER0_COMPA_vect)
{
  ms1++;
 if(ms1==1000)
 {
  s1++;
  ms1=0;
  if(s1==60)
  {
    m1++;
    s1=0;
    if(m1==60)
    {
      hr1++;
      m1=0;
    }
  }
 }  
}
ISR(INT0_vect)
{ 
  if(b0==1)
  {flag1++;
   if(flag1==1)
  {
   TCCR0B|=(1<<WGM02)|(1<<CS01)|(1<<CS00);
   
  }
  else
  {
   TCCR0B&=(~((1<<CS02)|(1<<CS01)|(1<<CS00)));
   flag1=0;
   
   
  }
  }
  if(b0==2)
  {
    
  _delay_ms(100);
    b1++;
  if (b1==7)
  {
    b1=1;
  }
  }
  if (b0==3)
  {
    b1++;
  if (b1==3)
  {
    b1=0;}
  
    
  }
}
void edt()
{
switch(b1)
  {
    case 1:s++;
           if(s==60)
           {
            s=0;
           }
           break;
    case 2:m++;
           if(m==60)
           {
            m=0;
           }
           break;
    case 3:hr++;
           if(hr==24)
           {
            hr=0;
           }
           break;
    case 4:dd++;
           if(dd==32)
           {
            dd=1;
           }
           break;
    case 5:mm++;
           if(mm==13)
           {
            mm=1;
           }
           break;
    case 6:yy++;
  
}
}
ISR(INT1_vect)
{ 

  if (buzzeron==1)
  {
    alarmon=0;
    buzzeron=0;
    PORTB&=~(1<<3);
    PORTB|=(1<<5);
    
  }
if (b0==1)
  {
  TCNT0=hr1=m1=s1=ms1=0;
 
  }
if(b0==2)
{  
  _delay_ms(100);
  edt();
  }
  if (b0==3)
  {
    switch(b1)
  {
    case 0:m3++;
           if(m3==60)
           {
            m3=0;
           }
           break;
    case 1:hr3++;
           if(hr3==24)
           {
            hr3=0;
           }
           break;
    case 2:if (alarmon==0)
             alarmon=1;
           else
             alarmon=0;
    
   }
    
  }
  }


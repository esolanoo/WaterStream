#define  F_CPU 4000000UL
#include <avr/io.h>				//Librería general del Microcontrolador
#include <stdint.h>				//Para poder declarar varialbes especiales
#include <util/delay.h>			//Para poder hacer delays
#include <avr/interrupt.h>	 //Para poder manejar interrupciones
#include "lcd.h"

//PROTOTIPADO DE FUNCIONES 
uint8_t cero_en_bit(volatile uint8_t *LUGAR, uint8_t BIT);
uint8_t uno_en_bit(volatile uint8_t *LUGAR, uint8_t BIT);
int Init_DS18B20(int pin);
void Write_DS18B20(unsigned char value, int pin);
unsigned char Read_DS18B20(int pin);
int Read_Temperature(int pin);
void pumps(void);						// función para bomba
void termo(void);
uint8_t tecladoF(volatile uint8_t *DDRT, volatile uint8_t *PORTT, volatile uint8_t *PINT);

/*
PORTC LCD
PORTA teclado matricial
PINB0 sensor frío
PINB1 sensor caliente
PINB3 bomba fría
PIND7 bomba caliente
*/

volatile uint8_t teclado[4][4] = {{1,2,3,10},
								{4,5,6,11},
								{7,8,9,12},
								{14,0,15,13}};
									
volatile int t_f, t_c, T;
volatile float Q, q_f, q_c;
void main(void){
	
	sei();
		
	DDRB |= (1<<3);
	PORTB &= (1<<3);
	TCNT0=0;
	OCR0=0;
	TCCR0=0b01101100;	// PWM0 en PB3
	
	DDRD |= (1<<7);
	PORTD &= (1<<7);
	TCNT2=0;
	OCR2=0;
	TCCR2=0b01101100;	// PWM2 en PD7
	
	lcd_init(LCD_DISP_ON);
	
	uint8_t aux;
    while (1){
		t_f = Read_Temperature(0);
		t_c = Read_Temperature(1);
		lcd_clrscr();
		lcd_puts("Temperatura (C): ");
		lcd_gotoxy(0,1);
		aux = tecladoF(&DDRA,&PORTA,&PINA);
		lcd_putc(aux+48);
		T = aux*10;
		aux = tecladoF(&DDRA,&PORTA,&PINA);
		lcd_putc(aux+48);
		T += aux;
		_delay_ms(1000);
		
		lcd_clrscr();
		lcd_puts("Caudal (L/min): ");
		lcd_gotoxy(0,1);
		lcd_puts("0.");
		aux = tecladoF(&DDRA,&PORTA,&PINA);
		lcd_putc(aux+48);
		Q = aux/10.0;
		aux = tecladoF(&DDRA,&PORTA,&PINA);
		lcd_putc(aux+48);
		Q += aux/100.0;
		_delay_ms(1000);
		
		lcd_clrscr();
		t_f = Read_Temperature(0); // lectura de termopar de agua fría en PB0
		_delay_us(20);
		lcd_putc(48+t_f/100);
		lcd_putc(48+(t_f/10)%10);
		lcd_putc('.');
		lcd_putc(48+t_f%10);
		lcd_puts(" C");
		lcd_gotoxy(0,1);
		
		t_c = Read_Temperature(1); // lectura de termopar de agua caliente en PB1
		_delay_us(20);
		lcd_putc(48+t_c/100);
		lcd_putc(48+(t_c/10)%10);
		lcd_putc('.');
		lcd_putc(48+t_c%10);
		lcd_puts(" C");
		_delay_ms(2000);	
		
		t_f /= 10;
		t_c /= 10;	
		
		lcd_clrscr();
		lcd_puts("Espere...");
			
		termo();
		pumps();
	
		lcd_clrscr();
		lcd_puts("Gracias :)");
		_delay_ms(1000);
    }
}

int Init_DS18B20(int pin){
	int temp;
	DDRB |= (1<<pin);;
	PORTB |= (1<<pin);
	_delay_us(10);
	PORTB &= ~(1<<pin);
	_delay_us(750);
	PORTB |= (1<<pin);
	DDRB &= ~(1<<pin);;
	_delay_us(150);
	temp = ((PINB&(1<<pin))>>pin);
	_delay_us(500);
	return temp;
}

void Write_DS18B20(unsigned char value, int pin){
	int i;
	DDRB |= (1<<pin);;
	for(i=0;i<8;i++){
		PORTB |= (1<<pin);
		_delay_us(1);
		PORTB &= ~(1<<pin);
		if((value&(1<<i))!=0){
			PORTB |= (1<<pin);
		}
		else{
			PORTB &= ~(1<<pin);
		}
		_delay_us(70);
		PORTB |= (1<<pin);
		_delay_us(10);
	}
	_delay_us(10);
}

unsigned char Read_DS18B20(int pin){
	unsigned char value = 0;
	int i;
	for(i=0;i<8;i++){
		DDRB |= (1<<pin);;
		PORTB |= (1<<pin);
		_delay_us(1);
		PORTB &= ~(1<<pin);
		_delay_us(1);
		PORTB |= (1<<pin);
		DDRB &= ~(1<<pin);;
		_delay_us(7);
		value |= (((PINB&(1<<pin))>>pin)<<i);
		_delay_us(70);
	}
	return value;
}

int Read_Temperature(int pin){
	char temp_l, temp_h;
	int temp;
	Init_DS18B20(pin);
	Write_DS18B20(0xCC, pin); //ROM
	Write_DS18B20(0x44, pin);
	_delay_ms(200);
	Init_DS18B20(pin);
	Write_DS18B20(0xCC, pin); //ROM
	Write_DS18B20(0xBE, pin); //RAM
	temp_l = Read_DS18B20(pin);
	temp_h = Read_DS18B20(pin);
	if((temp_h&0xf8)!=0x00){
		temp_h=~temp_h;
		temp_l=~temp_l;
		temp_l +=1;
		if(temp_l>255)
		temp_h++;
	}
	temp=temp_h;
	temp&=0x07;
	temp=((temp_h*256)+temp_l)*0.625+0.5;
	return temp;
}

void termo(void){
	float dc, df;
	dc = (T-t_c)*1.0;
	df = (T-t_f)*1.0;
	q_c = Q/(1-(dc/df));
	q_f = -dc*q_c/df;
}

void pumps(void){
	uint8_t var;
	var = (360.842*q_f*q_f)-(235.625*q_f)+138.708; // bomba fría
	OCR0 = var;
	_delay_us(60);
	var = (360.842*q_c*q_c)-(235.625*q_c)+138.708; // bomba caliente
	OCR2 = var;
	_delay_us(60);
	int i;
	for(i=0; i<=10; i++){ _delay_ms(1000); }	// deja las bombas funcionando por i segundos
	OCR0 = 0;
	_delay_us(60);
	OCR2 = 0;
	_delay_us(60);
}

uint8_t tecladoF(volatile uint8_t *DDRT, volatile uint8_t *PORTT, volatile uint8_t *PINT){
	//La funcion tecladoFuncion se quedará trabada hasta el momento en que se presione una tecla
	//Esta función regresa un valor del tipo uint8_t que tendrá que ser asignada a una variable
	//Ejemplo:

	//uint8_t tecla;
	//tecla = tecladoF(&DDRA,&PORTA,&PINA);

	*DDRT = (1<<7)|(1<<6)|(1<<5)|(1<<4);		// definimos puerto para teclado
	while(1){
		*PORTT = 0b11111111;
		for (uint8_t fila = 7; fila > 3; fila--){
			*PORTT = ~(1<<fila);
			_delay_us(2);
			for (uint8_t col = 0; col < 4; col++){
				if (cero_en_bit(PINT,col)){
					_delay_ms(100);
					while(cero_en_bit(PINT,col)) {}
					_delay_ms(100);
					return teclado[7-fila][col];
				}
			}
		}
	}
}

uint8_t cero_en_bit(volatile uint8_t *LUGAR, uint8_t BIT){
	return (!(*LUGAR&(1<<BIT)));
}

uint8_t uno_en_bit(volatile uint8_t *LUGAR, uint8_t BIT){
	return (*LUGAR&(1<<BIT));
}


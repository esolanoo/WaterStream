/*
 * PBL 2
 *
 * Created: 04/10/22
 * Author : Facheritos
 */ 

#define  F_CPU 8000000UL
#include <avr/io.h>				//Librería general del Microcontrolador
#include <stdint.h>				//Para poder declarar varialbes especiales
#include <util/delay.h>			//Para poder hacer delays
#include <avr/interrupt.h>	 //Para poder manejar interrupciones
#include "lcd.h"
//#include <time.h>
#include <stdlib.h>
#include "ultrasonic.h"

//PROTOTIPADO DE FUNCIONES PARA PODER UTILIZARLAS DESDE CUALQUIER "LUGAR"
//*************************************************************************
uint8_t cero_en_bit(volatile uint8_t *LUGAR, uint8_t BIT);
uint8_t uno_en_bit(volatile uint8_t *LUGAR, uint8_t BIT);
void pump(uint16_t tiempo);
void PaP(uint8_t motor, uint8_t reverse, uint8_t positions);
uint16_t pump_vol(uint16_t height_time, uint16_t radius);
void process(void);
void LCD_intgerToString(uint16_t data);
//*************************************************************************


//PUERTOS
// B0 - Boton inicio
// B7 - Bomba
// Sensor ultrasónico
//		PD6 - Echo
//		PB5 - Trigger
// PaPs
//		PORTC
//			Motor 1 (PC 0-3) - Carro
//			Motor 0 (PC 4-7) - Elevador

volatile uint8_t step1 = 0b10001000, step2 = 0b10001000;
volatile uint16_t aux_dist, distance, temp_dist;

void main(void){
	sei();
	DDRB &= ~(1<<0);
	PORTB |= (1<<0);		// Boton para iniciar todo el proceso
	DDRB |= (1<<7);			// Bomba
	DDRC |= 0xFF;
	PORTC = (step1&0xF0)|(step2&0x0F); // PaPs
	DDRA |= (1<<0)|(1<<7);
	
	lcd_init(LCD_DISP_ON);
	Ultrasonic_init();
	
    while (1){
		distance = Ultrasonic_readDistance();
		LCD_intgerToString(10 - distance);
		lcd_puts(" cm radio");
		_delay_ms(100);
		lcd_clrscr();
		if(cero_en_bit(&PINB, 0)){
			_delay_us(20);
			while(cero_en_bit(&PINB, 0)){}
			_delay_us(20);
			for(int iii=0; iii<4; iii++){ process(); }
			PaP(1, 0, 13);
		}
    }
}


void pump(uint16_t tiempo){	// tiempo en segundos
	PORTB |= (1<<7);
	LCD_intgerToString(tiempo);
	for(int ii=0; ii<tiempo; ii++){ _delay_ms(100); }
	PORTB &= ~(1<<7);
	_delay_us(200);
}

uint16_t pump_vol(uint16_t height_time, uint16_t radius){
	float height = (1.5*(height_time-3)+100)/10;		// convertir height_time a altura
	if(radius<1){ radius = 1; }
	uint16_t vol = 3.14159*radius*radius*height;
	if (radius<3) { vol = 3.14159*(radius+0.25)*(radius+0.25)*height; }
	LCD_intgerToString(vol);
	lcd_puts(" ml @ ");	
	if(vol>750){ return (uint16_t) vol*0.85; }
	else{ return (uint16_t) vol*.91; }
}

void PaP(uint8_t motor, uint8_t reverse, uint8_t positions){
	for(uint8_t i=0; i<positions; i++){
		for(uint8_t x=0; x<4; x++){			// Rotate PaP motor by 90 degree's in full step mode
			// x*4 = numero de vueltas que necesitemos para avanzar una posición
			for(uint8_t y=0; y<50; y++){
				if(motor == 0){
					if(reverse == 0){
						step1 = ((step1 & 0x01) ? 0x80 : 0x00) | (step1 >> 1); // ROR
					}
					else{
						step1 = ((step1 & 0x80) ? 0x01 : 0x00) | (step1 << 1);	// ROL
					}
				}
				else{
					if(reverse == 0){
						step2 = ((step2 & 0x01) ? 0x80 : 0x00) | (step2 >> 1); // ROR
					}
					else{
						step2 = ((step2 & 0x80) ? 0x01 : 0x00) | (step2 << 1);	// ROL
					}
				}
				PORTC = (step1&0xF0)|(step2&0x0F);
				_delay_us(3500);
			}
		}
	}
}

void process(void){
	PaP(1, 1, 3);
	
	_delay_ms(500);
	distance = Ultrasonic_readDistance();
	_delay_us(20);
	aux_dist = distance;
	_delay_ms(500);
	distance = Ultrasonic_readDistance();
	_delay_us(20);
	aux_dist = distance;
	
	lcd_clrscr();
	lcd_puts("hold r: ");
	LCD_intgerToString(10-distance);
	_delay_ms(250);
	temp_dist = 0;
	while ((aux_dist<distance+2)&&(aux_dist>distance-2)){		// deja al elevador moverse mientras lea la misma distancia
		aux_dist = Ultrasonic_readDistance();
		temp_dist++;
		PaP(0, 1, 1);			// A LO MEJOR CAMBIO DE DIRECCION
	}
	lcd_clrscr();
	if(10-distance<0){
		distance = 11;
	}
	lcd_puts("h: ");
	LCD_intgerToString(1.5*(temp_dist-3)+100);
	_delay_ms(2500);
	lcd_clrscr();
	
	uint16_t vol = pump_vol(temp_dist, 10-distance);
	pump(vol);

	lcd_clrscr();
	lcd_puts("...");
	for(uint16_t j=0; j<temp_dist; j++){ PaP(0, 0, 1); }
	lcd_clrscr();
	lcd_puts("next");
}


void LCD_intgerToString(uint16_t data){
	char buff[16]; // String to hold the ascii result
	itoa(data,buff,10); // Use itoa C function to convert the data to its corresponding ASCII value, 10 for decimal 
	lcd_puts(buff); // Display the string 
}

uint8_t cero_en_bit(volatile uint8_t *LUGAR, uint8_t BIT){
	return (!(*LUGAR&(1<<BIT)));
}

uint8_t uno_en_bit(volatile uint8_t *LUGAR, uint8_t BIT){
	return (*LUGAR&(1<<BIT));
}

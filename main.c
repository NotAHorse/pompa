/*
* histereza 2.0
*
* Created: 2020-05-28 20:40:21
* Author : a raccoon
*/

/*
reset			RESET	  U		pc5	adc - czujnik ciúnienia
LED0			pd0				pc4		
LED1			pd1				pc3		
LED2			pd2				pc2
LED3			pd3				pc1	
LED4			pd4				pc0
					VCC		  GND
					GND		  AREF
guzik sel		pb6			  AVCC
LED tryb2		pb7				pb5	SCK   \							guzik up
LED5			pd5				pb4	MISO   | - PROGRAMOWANIE		guzik down
LED6			pd6				pb3	MOSI  /							
LED7			pd7				pb2		LED tryb1
przekaünik		pb0            	pb1 	LED tryb0




LED tryb0 - LED tryb2 - wyswietla stan kontroli, binarnie od 1 do 6.


*/





#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>

// obsluga timerow:
void tim1_set(int t); // t w milisekundach
void tim1_reset();
int tim1_resNum = 0;

void tim0_set(int t); //t w milisekundach
void tim0_reset();
int tim0_resNum = 0;
unsigned char tim0_mem = 0;

char tim1 = 0;
char tim0 = 0;
//koniec timerow

#define max_val_u 120
#define max_val_l 2

#define min_val_u 118
#define min_val_l 0

int i, wartosc_czujnika_t[50], wartosc_czujnika_t_avg;

int main(void)
{
	unsigned char stan_histereza, stan_kontrola, sleep;
	unsigned char wys_cis, wys_stan_k;
	
	int maks_czujnika;
	
	unsigned char wartosc_czujnika;
	
	unsigned char przekaznik, up, down, sel;		//0 = off, 1 = on
	unsigned char s;
	int MAX, MIN;
	

	
	
	//tryby we/wy:
	DDRD = 0b11111111;
	PORTD = 0b00000000;
	DDRB = 0b10000111;
	PORTB =0b01111000;

	
	ADMUX =  0b01000101; //adc ref avcc+c, right adjust, channel 5
	ADCSRA = 0b11001110; //adc enable, interrupts, start conversion, prescaler 64
	//koniec trybow we/wy
	
	sei(); //wlacza przerwania
	//obsluga timerow:
	TCCR1A = 0;
	TCCR1B = 0;
	TIMSK |= 1<<OCIE1A;
	TIMSK |= 1<<TOIE1;
	TIMSK |= 1<<TOIE0;
	
	TCCR0 = 0;
	
	//koniec timerow
	
	
	up = 0;
	sel = 0;
	s = 0;
	down = 0;
		
	stan_histereza = 3;
	stan_kontrola = 61;
	wys_stan_k = 6;
	sleep = 0;
	
	MAX = 110;		//zmieÒmy jednak ograniczenia do 0 - 110. Dlaczego? Bo w sumie nie widzÍ czemu nie. Uøytkownik tego powinien byÊ wystarczajπco inteligentny
	MIN = 108;		//aby wiedzieÊ co robi, a czasem moøe potrzebowaÊ takich parametrÛw. Kiedy? - nie wiem.
	
	przekaznik = 0;
	wys_cis = 100;
	maks_czujnika = 512;
	wartosc_czujnika = 100;
	for(i = 0; i < 50; i++){wartosc_czujnika_t[i] = maks_czujnika;}
	wartosc_czujnika_t_avg = maks_czujnika;
	i = 0;
	
	while (1)
	{
		switch(stan_histereza){
			case 1:
			przekaznik = 0;
			if(wartosc_czujnika > MAX){
				stan_histereza = 2;
			}
			else if(sleep == 1)
			{
				stan_histereza = 3;
			}
			break;
			case 2:
			przekaznik = 1;
			if(wartosc_czujnika < MIN){
				stan_histereza = 1;
			}
			else if(sleep == 1)
			{
				stan_histereza = 3;
			}
			break;
			case 3:
			przekaznik = 0;
			
			if(!sleep){
				stan_histereza = 1;
			}
			break;
			default:
			stan_histereza = 1;
			break;
		}
		
		
		
		switch(stan_kontrola){
			case 1:
			wys_cis = wartosc_czujnika;
			wys_stan_k = 1;
			
			if(sel)
			{
				stan_kontrola = 2;
			}
			break;
			case 2:
			wys_cis = MAX;
			wys_stan_k = 2;
			
			if(sel)
			{
				stan_kontrola = 3;
			}
			else if(up^down)
			{
				
				tim1_set(5000);
				tim0_set(1000);
				stan_kontrola = 21;
			}
			break;
			case 21:
			wys_cis = MAX;
			wys_stan_k = 2;
			
			
			if(!(up^down))
			{
				tim0_reset();
				tim1_reset();
				stan_kontrola = 2;
			}
			else if(!tim0){
				tim0_set(1000);
				
				if(up){
					++MAX;
				}
				else if(down){
					--MAX;
				}
				
				
				if(MAX > max_val_u){MAX = max_val_u;}
				else if(MAX < max_val_l){MAX = max_val_l;}
				
				if(MIN+2 > MAX){MIN = MAX - 2;}
			}
			else if(!tim1)
			{
				tim1_reset();
				tim0_set(1000);
				
				stan_kontrola = 22;
			}
			break;
			case 22:
			wys_cis = MAX;
			wys_stan_k = 2;
			
			
			if(!(up^down))
			{
				stan_kontrola = 2;
				tim0_reset();
				tim1_reset();
			}
			else if(!tim0){
				tim0_set(1000);
				
				if(up){
					MAX+=5;
				}
				else if(down){
					MAX-=5;
				}
				
				
				if(MAX > max_val_u){MAX = max_val_u;}
				else if(MAX < min_val_l){MAX = min_val_l;}
				
				if(MIN+2 > MAX){MIN = MAX - 2;}
			}
			break;
			case 3:
			wys_cis = MIN;
			wys_stan_k = 3;
			
			if(sel)
			{
				stan_kontrola = 4;
			}
			else if(up^down)
			{
				tim1_set(5000);
				tim0_set(1000);
				stan_kontrola = 31;
			}
			break;
			case 31:
			wys_cis = MIN;
			wys_stan_k = 3;
			
			
			if(!(up^down))
			{
				stan_kontrola = 3;
			}
			else if(!tim0){
				tim0_set(1000);
				
				if(up){
					++MIN;
				}
				else if(down){
					--MIN;
				}
				
				
				if(MIN > min_val_u){MIN = min_val_u;}
				else if(MIN < min_val_l){MAX = min_val_l;}
				
				if(MIN+2 > MAX){MAX = MIN + 2;}
			}
			else if(!tim1)
			{
				tim1_reset();
				tim0_set(1000);
				
				stan_kontrola = 32;
			}
			break;
			case 32:
			wys_cis= MIN;
			wys_stan_k = 3;
			
			
			if(!(up^down))
			{
				stan_kontrola = 3;
			}
			else if(!tim0){
				tim0_set(1000);
				
				if(up){
					MIN+=5;
				}
				else if(down){
					MIN-=5;
				}
				
				
				if(MIN > min_val_u){MIN = min_val_u;}
				else if(MIN < min_val_l){MIN = min_val_l;}
				
				if(MIN+2 > MAX){MAX = MIN + 2;}
			}
			break;
			case 4:
			wys_cis = 0;
			wys_stan_k = 4;
			
			if(sel)	{
				stan_kontrola = 5;
				tim1_set(500);
			}
			else if(up){
				tim1_set(2500);
				stan_kontrola = 41;
			}
			break;
			case 41:
			wys_cis = 1;
			wys_stan_k = 4;
			
			if(sel)	{
				stan_kontrola = 5;
				tim1_set(500);
			}
			else if(up&&!tim1){
				tim1_set(2500);
				stan_kontrola = 42;
			}
			else if(!up){
				stan_kontrola = 4;
			}
			break;
			case 42:
			wys_cis = 3;
			wys_stan_k = 4;
			
			if(sel)	{
				stan_kontrola = 5;
				tim1_set(500);
			}
			else if(up&&!tim1){
				tim1_reset();
				stan_kontrola = 43;
			}
			else if(!up){
				stan_kontrola = 4;
			}
			break;
			case 43:
			wys_cis = 0;
			wys_stan_k = 4;
			
			maks_czujnika = wartosc_czujnika_t_avg;
			
			stan_kontrola = 1;
			
			break;
			case 5:
			wys_cis = 0b01010101;
			wys_stan_k = 5;
			
			if(sel){
				stan_kontrola = 6;
			}
			else if(!tim1){
				tim1_set(500);
				stan_kontrola = 51;
			}
			break;
			case 51:
			wys_cis = 0b10101010;
			wys_stan_k = 5;
			
			if(sel){
				stan_kontrola = 6;
			}
			else if(!tim1){
				tim1_set(500);
				stan_kontrola = 52;
			}
			break;
			case 52:
			wys_cis = 0b10101010;
			wys_stan_k = 0b101;
			
			if(sel){
				stan_kontrola = 6;
			}
			else if(!tim1){
				tim1_set(500);
				stan_kontrola = 53;
			}
			break;
			case 53:
			wys_cis = 0b01010101;
			wys_stan_k = 0b010;
			
			if(sel){
				stan_kontrola = 6;
			}
			else if(!tim1){
				tim1_set(500);
				stan_kontrola = 52;
			}
			break;
			case 6:
			wys_cis = wartosc_czujnika;
			wys_stan_k = 6;
			
			if(down){
				stan_kontrola = 61;
				tim1_set(5000);
			}
			else if(sel){
				stan_kontrola = 1;
			}
			break;
			case 61:
			wys_cis = 0;
			wys_stan_k = 6;
			sleep = 1;
			
			if(sel){
				sleep = 0;
				stan_kontrola = 1;
			}
			if(!tim1){
				tim1_set(1000);
				stan_kontrola = 62;
			}
			break;
			case 62:
			wys_cis = 0;
			wys_stan_k = 0;
			sleep = 1;
			
			if(sel){
				sleep = 0;
				stan_kontrola = 1;
			}
			if(!tim1){
				tim1_set(10000);
				stan_kontrola = 61;
			}
			break;
			default:
			stan_kontrola = 1;
			break;
		}
		
		//a teraz czas na obsluge we/wy
		PORTD = (wys_cis&128)|(wys_cis&64)|(wys_cis&32)|(wys_cis&16)|(wys_cis&8)|(wys_cis&4)|(wys_cis&2)|(wys_cis&1);
		
		
		(przekaznik)?(PORTB &= 0b11111110):(PORTB |= 0b00000001);
		
		(wys_stan_k&4)?(PORTB |= 0b10000000):(PORTB &= 0b01111111);
		(wys_stan_k&2)?(PORTB |= 0b00000100):(PORTB &= 0b11111011);
		(wys_stan_k&1)?(PORTB |= 0b00000010):(PORTB &= 0b11111101);
		
		/*
		if(bit_is_clear(PINB,PB1) && d ==0){down = 1; d = 1;}
		else if(bit_is_clear(PINB,PB1) && down == 1){down = 0;}
		if(bit_is_set(PINB, PB1)){d = 0; down = 0;}
		
		if(bit_is_clear(PINB,PB2) && u ==0){up = 1; u = 1;}
		else if(bit_is_clear(PINB,PB2) && up == 1){up = 0;}
		if(bit_is_set(PINB, PB2)){u = 0; up = 0;}
			*/
		if(bit_is_clear(PINB,PB6) && sel == 1){sel = 0;}
		if(bit_is_clear(PINB,PB6) && s ==0){sel = 1; s = 1;}
		if(bit_is_set(PINB, PB6)){s = 0; sel = 0;} 
			
		
		(bit_is_clear(PINB, PB4))?(down = 1):(down = 0);
		(bit_is_clear(PINB, PB5))?(up = 1):(up = 0);
		//(bit_is_clear(PINB, PC6))?(sel = 1):(sel = 0);

			
	if(!(ADCSRA&(1<<ADSC))){ADCSRA |= (1<<ADSC);}
		wartosc_czujnika = (unsigned char)(((float)(wartosc_czujnika_t_avg)/(float)maks_czujnika)*100);
		
		
		
		
	}
}




void tim1_set(int t){
	int res;
	tim1_reset();
	res = t/16776;
	tim1 = 1;
	tim1_resNum = res;
	t = t % 16776;
	
	cli();
	OCR1A = (t*3.90625);
	sei();
	
	TCCR1B |= (1<<CS12); //przy 1mhz daje to 3906.25 cykli na sekundÍ, czyli 3.90625 cykla na milisekundÍ

}

void tim1_reset(){
	TCCR1B = 0;
	cli();
	TCNT1H = 0;
	TCNT1L = 0;
	sei();
	tim1 = 0;
	tim1_resNum = 0;
};

ISR(TIMER1_COMPA_vect){
	if(tim1_resNum == 0){
		tim1 = 0;
		TCCR1B = 0;
	}
}

ISR(TIMER1_OVF_vect){
	if(tim1_resNum > 0){
		tim1_resNum--;
	}
}

void tim0_set(int t){
	int res;
	tim0_reset();
	res = t/261;
	tim0 = 1;
	tim0_resNum = res;
	t = t % 261;
	
	if(res == 0){
		TCNT0 = (unsigned char)((261-t)*0.9765625)%255;
	}
	else{
		tim0_mem = (unsigned char)((261-t)*0.9765625)%255;
	}
	
	TCCR0 |= (1<<CS02) | (1<<CS00); //przy 1mhz daje to 976.5625 cykli na sekundÍ, czyli 1ms = 0.9765625 cykla
}

void tim0_reset(){
	TCCR0 = 0;
	TCNT0 = 0;
	tim0 = 0;
	tim0_resNum = 0;
	tim0_mem = 0;
};


ISR(TIMER0_OVF_vect){
	if(tim0_resNum > 0){
		--tim0_resNum;
	}
	else if(tim0_mem!=0){
		TCNT0 = (unsigned char)tim0_mem;
		tim0_mem = 0;
	}
	else{
		tim0 = 0;
		TCCR0 = 0;
		TCNT0 = 0;
		tim0_resNum = 0;
		tim0_mem = 0;
	}
}



ISR(ADC_vect){
	++i;
	if(i >= 50){i =0;}
	wartosc_czujnika_t[i] = ADCW;
	wartosc_czujnika_t_avg = wartosc_czujnika_t_avg + (wartosc_czujnika_t[i])/50 - (((i==49)?(wartosc_czujnika_t[0]):(wartosc_czujnika_t[i+1])))/50;
}
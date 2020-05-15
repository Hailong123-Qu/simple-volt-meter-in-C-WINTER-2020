#include "avr.h"
#include "lcd.h"
#include <stdio.h>
#include <math.h>
#include <limits.h>

/*
when you want to test it with a power supply, please don't exceed  5.0V on power supply. Else you will burn the lcd
*/

typedef enum {true, false} bool;
bool differential ;
bool freeze;
bool reset;
int reset_count_down;

float minV, maxV, avgV, curV;
int cnt=0, sum=0;

int is_pressed(int r, int c);
int get_key(void);
float avg_cal();
unsigned short get_sample();
void lcd_display();
float unsigned_convert_A2D(unsigned short v);
float signed_convert_A2D();

void reset_fuc();
void setMinMax();
void differential_on_off();
/* testing values;
char buff[16];
lcd_pos(0,1);
sprintf(buff, "%02d", k);
lcd_puts2(buff);
*/
int main(void)
{
	lcd_init();
	reset = false;
	DDRB= 0xFF; //enable the all PORTBs
	lcd_clr();//clean up lcd trash just in case
	freeze = false; //for button

	differential = false;//for extra credit button
	int k; //for keypad

	avgV = 0.0; //global vars initialize.
	minV = 6;
	maxV = -6;
	bool first_attemp = true;
	unsigned short o; //read the raw data
	
	int tempSum = 0, tempCnt = 0; //for freeze button vars
	
	//press 1 for reset
	//press 16 for freeze
	//press 13 for diff. mode
	float kk;
    while (1) 
    {
		k = get_key();
		
		if(1 == k){
			//Button 1: Resets Max/Min/Avg to ----- (10%)
			reset = true;
			reset_count_down = 6;//depends on how long you want to perform
			reset_fuc();
		}else if(16 ==  k){
			//Button 1 (or another button): Initiates max/min hold and averaging  (10%) [Note: instantaneous value should always display when the power is on]
			//0227 office hour.
			//freeze others except instantaneous value. however, the cnt should keep counting as well as sum.
			//need to store the temp Sum and temp cnt somewhere when it freezes. 
			freeze = !freeze;
		}else if(13 == k){
			//p217 ADCH, ADCL
			//Supports differential (V1, and V2) inputs. A button press enters/leaves differential mode.  The readout should represent (V1 - V2).  (10%)
			differential = !differential;
		}
		
		o = get_sample();
		
		differential_on_off();
		
		if(differential == true){
			curV = signed_convert_A2D();  //read the raw data in negative/positive
		}else{
			curV = unsigned_convert_A2D(o);
		}
		avr_wait(500); //Sampling rate should be greater than 2 samples/second

		if(freeze == true){
			tempCnt++;
			tempSum += curV;
		}else{
			sum += tempSum;
			tempCnt += tempCnt;
			tempCnt = 0;
			tempSum = 0;
			setMinMax();
			sum+=curV;
			cnt++;
		}
	
		avgV = avg_cal(sum,cnt);
		lcd_display();
		
    }
}

void differential_on_off(){
	ADMUX = 0b01000000;
	if(differential == true){ //p215
		//last five bits 10000
		SET_BIT(ADMUX, 4);
		// no need to do the subtraction stuff and mulitply by gain. the avr is already done it for you.
		//gain = 1 by default in this mode
	}else{
		CLR_BIT(ADMUX, 4);
	}
}


void setMinMax(){
	if(curV > maxV){
		maxV = curV;
	}
	if(curV < minV){
		minV = curV;
	}
}

void reset_fuc(){
	minV = 6;
	maxV = -6;
	avgV = 0.0;
	cnt = 0.0;
	sum = 0.0;
}

//Displays 4 basic readouts: (1) instantaneous voltage, (2) max voltage, (3) min voltage, and (4) average voltage (60%, 15% each)
void lcd_display(){
	/*
	--------------------------------
	|current voltage | min. voltage|
	|max. voltage    | avg. voltage|
	--------------------------------
	*/
	char buff1[16], buff2[16];
	lcd_clr(); //this is necessary to clean up trash. 
	
	if(reset == true){
		lcd_pos(0,1);
		sprintf(buff1,"%.3f|-----", curV);
		lcd_puts2(buff1);
		lcd_pos(1,1);
		lcd_puts2("-----|-----");
		reset_fuc();
		reset_count_down--;
		if(reset_count_down == 0){
			reset = false;
		}
	}else{
		//Utilizing maximum precision (10%) 
		lcd_pos(0,1);
		if(differential == true && freeze == true){
			sprintf(buff1,"%.3f|%.3f X", curV, minV);	
		}else if(differential == true && freeze == false){
			sprintf(buff1,"%.3f|%.3f D", curV, minV);
		}else if(differential == false && freeze == true){
			sprintf(buff1,"%.3f|%.3f F", curV, minV);
		}else{
			sprintf(buff1,"%.3f|%.3f", curV, minV);
		}
	
		lcd_puts2(buff1);
	
		lcd_pos(1,1);
		sprintf(buff2,"%.3f|%.3f", maxV, avgV);
		lcd_puts2(buff2);
	}
}

float unsigned_convert_A2D(unsigned short v){
	return (float)v*5.0/1024.0;
}

float signed_convert_A2D(){
	//511*a + b = 5
	//-512*a + b = -5
	// y = C1*x + C2
	//solving for a and b to get the voltage range from -5 to 5
	 
	float a = (float) 5.0/1023.0;
	
	int myADC = ADC;
	//bit or operator doesn't work!!!! I wasted like 3 days solving this and fk my life
	//int myADC = (((int)ADCH)<<9) + (int8_t)ADCL;
	//if(myADC & 000000100000000000)
	//myADC |= 11111110000000000000000
	
	if(GET_BIT(myADC,9)){
		SET_BIT(myADC,15);
		SET_BIT(myADC,14);
		SET_BIT(myADC,13);
		SET_BIT(myADC,12);
		SET_BIT(myADC,11);
		SET_BIT(myADC,10);
	}
		
	return (float)myADC*2*a;
	
	//return (float)ADC;
	
	//binary to decimal conversion.	
	/* shou dong "convert" you wen ti
	int arr_bit[10];
	arr_bit[9] = GET_BIT(ADCH,1);
	arr_bit[8] = GET_BIT(ADCH,0);
	arr_bit[7] = GET_BIT(ADCL,7);
	arr_bit[6] = GET_BIT(ADCL,6);
	arr_bit[5] = GET_BIT(ADCL,5);
	arr_bit[4] = GET_BIT(ADCL,4);
	arr_bit[3] = GET_BIT(ADCL,3);
	arr_bit[2] = GET_BIT(ADCL,2);
	arr_bit[1] = GET_BIT(ADCL,1);
	arr_bit[0] = GET_BIT(ADCL,0);
		
	if(arr_bit[9] == 1){
		sum = -pow(2,9);
	}else{
		sum = pow(2,9)-1;
	}
	for(i = 0; i < 9; i++){
		if(arr_bit[i]==1){
			sum+=pow(2,i);
		}
	}
	*/
}

unsigned short get_sample(){//p214
	//dont try to use hexadecimal like ADMUX = 0x40. somehow it didn't work. i spent few hours solving that, but i got nothing :( 
	
	//for security issue, wait the bit changes for hardware
	//use setbit is safer
	SET_BIT(ADCSRA,7); 
	/*Writing this bit to one enables the ADC. By writing it to zero, the ADC is turned off. 
	Turning the ADC off while a conversion is in progress, will terminate this conversion.*/
	
	SET_BIT(ADCSRA,6);
	return ADC;
}

float avg_cal(int sum, int count){
	return (float)sum/count;
}

int is_pressed(int r, int c){
	//set all
	DDRC = 0;
	PORTC = 0;
	
	CLR_BIT(DDRC,r);
	SET_BIT(PORTC,r);
	
	SET_BIT(DDRC,c+4);
	CLR_BIT(PORTC,c+4);
	return (GET_BIT(PINC,r))?0:1; //if r bit is 0 return 1 else return 0
} 

int get_key(void){
	int r, c;
	for(r = 0; r < 4; r++){
		for(c = 0; c < 4; c++){
			if(is_pressed(r,c)){
				return (r*4)+c+1;
			}
		}
	}
	return 0;
}
#include <reg51.h>

// PWM
sbit WAVE = P3^1;  
unsigned int temp; //Used inside of Timer0 ISR  
unsigned int PWM;

// Dip-Switch
sbit SW1 = P1^0;
sbit SW2 = P1^1;
sbit SW3 = P1^2;

// Fan
sbit encoder = P3^3 ;
int edges;
int cycles;
long cycleBit;
int fullCycle;
int t1, t2, t3;
int c1, c2, c3;
double rpm;
int R = 2; // generates 2 pulse per revolution
//sbit PWM_Read = P2^4;		// for reading the PWM of blue wire (ACTIVE LOW) port 2


// LED Number Display
unsigned int hundreds = 0;
unsigned int tens = 0;
unsigned int ones = 0;
unsigned int currentDigit = 1;
sbit LED = P2; // 7-Segment LED port
sbit LED_Hundreds = P1^3;
sbit LED_Tens = P1^4;
sbit LED_Ones = P1^5;

// For Displaying Number Patterns on LED Pins - look up table
unsigned char digitPatterns[] = {
    0x03, 0x9F, 0x25, 0x0D, 0xD9, 0x49, 0x41, 0x1F, 0x01, 0x09
};


// P controller
#define Kp 1.0  // Proportional Gain
double setpoint = 300;  // Desired RPM (Setpoint)

// Function Prototype
void delay(unsigned int count);
void DIP_SW(SW1,SW2,SW3);
void timer();
void initTimer0(void);
void displayNumber(unsigned int number);
void external0_ISR();

void main () {
  P1=0xFF;     //make P1 an input
  P2=0xFF;     //make P2 an input
  P3=0x00;     //make P3 an output
	
	SW1 = 1;        // Making switch 0 pin as input
	SW2 = 1;        // Making switch 1 pin as input
	SW3 = 1;	      // Making switch 2 pin as input 
	encoder =1; 
	PWM = 0;
	
	// Initialize timer for PWM control
  initTimer0();

	external0_ISR();
	
	//enable exteral interupts for encoder INT0
	TCON = TCON | 0x01;
	
	//enable INT0 and global interupts
	IE = IE | 0x81;
	while(1){
			displayNumber(123);
		// Initialize DIP switches and set PWM value	
		  DIP_SW(SW1,SW2,SW3);
		
	}
}


/*
delay for when we use duty cycle - this fucntion was to for testing the PWM initially with the use of delay for duty cycle.
However, we do not need it anymore since we are usinng timer now
*/
void delay(unsigned int count) {
    int i;
    for (i=(count*1000); i>0; i--) {
    }
}

// setup the function for timer0
void initTimer0(void)
{
	TMOD &= 0xF0;    // Clear 4 bit field for timer0
	TMOD |= 0x01;    // Set timer0 in mode where 1 = 16 bit mode
	TH0 = 0;         // First time value
	TL0 = 0;         // Set arbitrarily zero
	ET0 = 1;         // Enable Timer0 interrupts
	EA  = 1;         // Global interupt enable
	TR0 = 1;         // Start Timer 0
}

void timer() {
  TR0 = 1;  //start timer
}

// executes when timer overflows
void timer0() interrupt 1 {
  TR0=0;	// stop timer 0 after overflow, so ISR execute fine
  WAVE=~WAVE; // toggle pin
  TL0 = 0x00;  
  TH0 = 0xFF; //we choose this for overflow
 }

 
void dip_SW(int SW1, int SW2, int SW3) {      
    {
        // If all switches are off (000), set PWM to maximum value (254).
        if (SW1 == 0 && SW2 == 0 && SW3 == 0) {
            PWM = 254;
        }

        // If switches are set to 001, set PWM to a slightly lower value (204).
        if (SW1 == 0 && SW2 == 0 && SW3 == 1) {
            PWM = 204;
        }

        // If switches are set to 010, set PWM to 179.
        if (SW1 == 0 && SW2 == 1 && SW3 == 0) {
            PWM = 179;
        }

        // If switches are set to 011, set PWM to 128.
        if (SW1 == 0 && SW2 == 1 && SW3 == 1) {
            PWM = 128;
        }

        // If switches are set to 100, set PWM to 102.
        if (SW1 == 1 && SW2 == 0 && SW3 == 0) {
            PWM = 102;
        }

        // If switches are set to 101, set PWM to 77.
        if (SW1 == 1 && SW2 == 0 && SW3 == 1) {
            PWM = 77;
        }

        // If switches are set to 110, set PWM to 26.
        if (SW1 == 1 && SW2 == 1 && SW3 == 0) {
            PWM = 26;
        }

        // If all switches are on (111), set PWM to the minimum value (0).
        if (SW1 == 1 && SW2 == 1 && SW3 == 1) {
            PWM = 0;
        }
    }    
}    


// timer0 interupt (ISR) for PWM
void Timer0_ISR (void) interrupt 1   
{
	dip_SW(SW1, SW2, SW3);
	TR0 = 0;    									// Stop Timer 0

	if(WAVE)											// if WAVE Pin is high
	{
		WAVE = 0;
		temp = (255-PWM);						// if PWM pin is high, subtract max PWM (255 for 8 bits) with current PWM
		TH0  = 0xFF - (temp)&0xFF;	// set the first time as the register subtracted by temporary register
		TL0  = 0xFF - temp&0xFF;		// set the arbitrarily zero as the register subtracted by temporary register
	}
	else	     										// if PWM_Pin is low
	{
		WAVE = 1;
		temp = PWM;									// if PWM pin is low, set the temporary register as the PWM signal
		TH0  = 0xFF - (temp)&0xFF;	// set the first time value as the register subtracted by temporary register
		TL0  = 0xFF - temp&0xFF;		// set the arbitrarily zero value as the register subtracted by temporary register
	}
	TF0 = 0;     									// Clearing the interupt flag
	TR0 = 1;     									// Start Timer 0
}


// speed calculation
double aveRPM (double t1, double t2, double t3, double r) {
	double aveT = (t1+t2+t3)/3;
	double freq = 1/aveT;
	return (60*freq/r);
}

// For Encoder
void external0_ISR() interrupt 0 {
	edges++;
	TR0 =1;												// start timer
	if (edges ==2) {
		edges =0;
		cycles++;
		encoder =~encoder;
		TR0=0;			
		cycleBit= (TH0<<8) | TL0; 	// shifts TH0 to left causing it to be in higher byte value
		fullCycle = cycleBit * 0.000001085;	// cycle time calculation. convert timer counts to time
		timer ();
		
		if (cycles==1){
			c1=fullCycle;
		} 
		if (cycles==2){
			c2=fullCycle;
		}
		if (cycles==3){
			c3=fullCycle;
			rpm = aveRPM (c1, c2, c3, R);
			cycles =0;								// purpose: to restart the counting process
			TH0=0;
			TL0=0;
			//p controller
			//int error = setpoint - rpm;   // Calculate error
      //PWM = (unsigned int)(PWM + Kp * error);  // Adjust PWM based on error
			
		}	
	}
}



	
// LED Display
void displayNumber(unsigned int number) {
    if (number < 0 || number > 999) {
        P2 = digitPatterns[0];
        return;
    }

		// Extract the digits
    hundreds = number / 100;
    tens = (number / 10) % 10;
    ones = number % 10;

    // Use timer interrupt for multiplexing
    switch (currentDigit) {
        case 1:				
					LED_Ones = 0;          // Turn off ones digit
					LED_Tens = 0;          // Turn off tens digit
					LED_Hundreds = 1;      // Activate hundreds digit
					P2 = digitPatterns[hundreds];  // Display the hundreds digit pattern
					delay(5);              // Small delay to stabilize the display
					currentDigit = 2;      // Move to the next digit (tens)
					break;

        case 2:
						LED_Ones = 0;
						LED_Hundreds = 0;
            LED_Tens = 1;
            P2 = digitPatterns[tens];
						delay(5);
            currentDigit = 3;
            break;

        case 3:
            LED_Ones = 1;
						LED_Tens = 0;
            LED_Hundreds = 0;
						delay(5);
            P2 = digitPatterns[ones];
            LED_Ones = 0;
						delay(5);
            currentDigit = 1;
			}
		}

/*		
float PID_Calc(float fSetSpeed)
{
	stPIDData.fSetSpeed = fSetSpeed;
 // error calculation, proportional control
	stPIDData.fErr = stPIDData.fSetSpeed - stPIDData.fActualSpeed;
 // PID formula
	stPIDData.fVoltage = stPIDData.fKp * stPIDData.fErr * (stPIDData.fErr - stPIDData.fErr_last);
	stPIDData.fErr_last = stPIDData.fErr;
 
 // actuall rotation speed
	stPIDData.fActualSpeed = stMotorData.uiRPM;
 
 // PID actual speed
	return stPIDData.fActualSpeed;
}


	void PID_ControlSample(void) 
{
	int iCount = 0;
	 float fSpeed = 0.0;
	 
	 
	PID_Init();
	 // PID control 1000 times
	while (iCount < 1000)
	{
	 fSpeed = PID_Calc(200.0);
	 iCount++;
	}
	stPIDData.fActualSpeed;
}
*/
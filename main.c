/*
 * 		Daša Noskova - xnosko05
 *  		IMP - 2020/2021
 *
 *********************************************/
#include "MK60D10.h"
#include "alfabet.h"
#include <stdio.h>
#include <string.h>

/* Macros for bit-level registers manipulation */
#define GPIO_PIN_MASK 0x1Fu
#define GPIO_PIN(x) (((1)<<(x & GPIO_PIN_MASK)))

/* Mapping of LEDs and buttons to specific port pins: */
// Note: only D9, SW3 and SW5 are used in this sample app
#define A0 0x100      // PTA8
#define A1 0x400      // PTA10
#define A2 0x40       // PTA6
#define A3 0x800      // PTA11

#define R0 0x4000000  //PTA26
#define R1 0x1000000  //PTA24
#define R2 0x200 	  //PTA9
#define R3 0x2000000  //PTA25
#define R4 0x10000000 //PTA28
#define R5 0x80 	  //PTA7
#define R6 0x8000000  //PTA27
#define R7 0x20000000 //PTA29

#define EN 0x10000000 //PTE28

/*
#define BTN_SW2 0x400     // Port E, bit 10
#define BTN_SW3 0x1000    // Port E, bit 12
#define BTN_SW4 0x8000000 // Port E, bit 27
#define BTN_SW5 0x4000000 // Port E, bit 26
#define BTN_SW6 0x800     // Port E, bit 11
*/


int pressed_up = 0, pressed_down = 0;
int enable = 0;
int count_cols = 0;
int offset = 0;
int time = 4799999; // 2s



/* Initialize the MCU - basic clock settings, turning the watchdog off */
void MCUInit(void)  {
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK;
}

void PortsInit(void)
{
    /* Turn on all port clocks */
    SIM->SCGC5 = SIM_SCGC5_PORTE_MASK | SIM_SCGC5_PORTA_MASK;

    /* Set corresponding PTA pins */

    PORTA->PCR[6] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[7] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[8] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[9] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[10] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[11] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[27] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[26] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[24] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[29] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[28] |= PORT_PCR_MUX(0x01);
    PORTA->PCR[25] |= PORT_PCR_MUX(0x01);

    //NVIC_ClearPendingIRQ(PORTA_IRQn);      /* Vynuluj priznak prerusenia od portu A  */
    //NVIC_EnableIRQ(PORTA_IRQn);        /* Povol prerusenie od portu A          */

    /* Set port E*/
    PORTE->PCR[28] = PORT_PCR_MUX(0x01);

    /*
    PORTE->PCR[10] = PORT_PCR_MUX(0x01); // SW2
    PORTE->PCR[12] = PORT_PCR_MUX(0x01); // SW3
    PORTE->PCR[27] = PORT_PCR_MUX(0x01); // SW4
    PORTE->PCR[26] = PORT_PCR_MUX(0x01); // SW5
    PORTE->PCR[11] = PORT_PCR_MUX(0x01); // SW6
	*/

    /* Change corresponding PTB port pins as outputs */
    PTA->PDDR = GPIO_PDDR_PDD(0x3F000FC0);     // LED ports as outputs
    PTA->PDOR |= GPIO_PDOR_PDO(0x3F000FC0);    // turn all LEDs OFF


}

/* Conversion of requested column number into the 4-to-16 decoder control.  */
void column_select(unsigned int col_num)
{
	unsigned i, result, col_sel[4];

	for (i =0; i<4; i++) {
		result = col_num / 2;	  // Whole-number division of the input number
		col_sel[i] = col_num % 2;
		col_num = result;

		switch(i) {

			// Selection signal A0
		    case 0:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(8))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(8)));
				break;

			// Selection signal A1
			case 1:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(10))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(10)));
				break;

			// Selection signal A2
			case 2:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(6))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(6)));
				break;

			// Selection signal A3
			case 3:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(11))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(11)));
				break;

			// Otherwise nothing to do...
			default:
				break;
		}
	}
}

void nul_rows(){
	PTA->PDOR &= ~R0;
	PTA->PDOR &= ~R1;
	PTA->PDOR &= ~R2;
	PTA->PDOR &= ~R3;
	PTA->PDOR &= ~R4;
	PTA->PDOR &= ~R5;
	PTA->PDOR &= ~R6;
	PTA->PDOR &= ~R7;
}

void nul_all(){
	for(int i = 0; i < 4; i++){
		nul_rows();
		column_select(i);
	}
}

void led_rows(int bit){
	nul_rows();

	if (0 != (bit & 128))
		PTA->PDOR |= R0;
	if (0 != (bit & 64))
		PTA->PDOR |= R1;
	if (0 != (bit & 32))
		PTA->PDOR |= R2;
	if (0 != (bit & 16))
		PTA->PDOR |= R3;
	if (0 != (bit & 8))
		PTA->PDOR |= R4;
	if (0 != (bit & 4))
		PTA->PDOR |= R5;
	if (0 != (bit & 2))
		PTA->PDOR |= R6;
	if (0 != (bit & 1))
		PTA->PDOR |= R7;

}


void printChar(char text, int i) {
	int* matrix;

	switch(text){
	case 'A':
	case 'a':
		matrix = A;
		break;
	case 'B':
	case 'b':
		matrix = B;
		break;
	case 'C':
	case 'c':
		matrix = C;
		break;
	case 'D':
	case 'd':
		matrix = D;
		break;
	case 'E':
	case 'e':
		matrix = E;
		break;
	case 'F':
	case 'f':
		matrix = F;
		break;
	case 'G':
	case 'g':
		matrix = G;
		break;
	case 'H':
	case 'h':
		matrix = H;
		break;
	case 'I':
	case 'i':
		matrix = I;
		break;
	case 'J':
	case 'j':
		matrix = J;
		break;
	case 'K':
	case 'k':
		matrix = K;
		break;
	case 'L':
	case 'l':
		matrix = L;
		break;
	case 'M':
	case 'm':
		matrix = M;
		break;
	case 'N':
	case 'n':
		matrix = N;
		break;
	case 'O':
	case 'o':
		matrix = O;
		break;
	case 'P':
	case 'p':
		matrix = P;
		break;
	case 'Q':
	case 'q':
		matrix = Q;
		break;
	case 'R':
	case 'r':
		matrix = R;
		break;
	case 'S':
	case 's':
		matrix = S;
		break;
	case 'T':
	case 't':
		matrix = T;
		break;
	case 'U':
	case 'u':
		matrix = U;
		break;
	case 'V':
	case 'v':
		matrix = V;
		break;
	case 'W':
	case 'w':
		matrix = W;
		break;
	case 'X':
	case 'x':
		matrix = X;
		break;
	case 'Y':
	case 'y':
		matrix = Y;
		break;
	case 'Z':
	case 'z':
		matrix = Z;
		break;
	}

	//int max_col = 6 - count_cols;

	//for (int i = 0; i < max_col; i++){
	nul_rows();
	column_select(i);
	int index = ((i+offset) % 6); //6 lebo matica pismena ma rozmer 6
	led_rows(matrix[index]);

}

int set_letter(int len){
	int sum = offset + (len-1)*6;
	int letter = 0;
	if (offset >= 0 && offset <= 5) {
		letter = 0;
	}
	else if (offset >= 6 && offset <= 11) {
		letter = 1;
	}
	else if (offset >= 12 && offset <= 17) {
		letter = 2;
	}

	return letter;
}

void print_text(char* text){
	int len = strlen(text);
	int letter;
	//17 = len*6-1
	if (offset > 18)
		offset = 0;

	int actual = offset;
	letter = set_letter(len);

	for (int i = 0; i < 16;i++){
		actual++;
		actual = actual % 7;
		if (actual == 6) {
			actual = 0;
			if (letter != len-1) // nie sme na konci slova
				letter++;
			else // koniec slova
				letter = 0;
		}
		printChar(text[letter],i);
	}
}


void PIT0_IRQHandler(void)
{
	PIT_TCTRL0 = 0;     // Disable timer
	offset++;
	PIT_TFLG0 = PIT_TFLG_TIF_MASK;     // Clear the timer interrupt flag
	PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;     // Enable timer

}

void PITInit(int time)
{
	 SIM_SCGC6 |= SIM_SCGC6_PIT_MASK; // Enable clock to PIT
	 PIT_LDVAL0 = time; // set starting value
	 //PIT_MCR &= ~PIT_MCR_MDIS_MASK;     // Enable clock for timer
	 PIT_MCR = 0x00;
	 //PIT_MCR |= PIT_MCR_FRZ_MASK; stop in debug
	 PIT_TFLG0 |= PIT_TFLG_TIF_MASK;     // Clear the timer interrupt flag

	 NVIC_EnableIRQ(PIT0_IRQn);         // enable interrupts from LPTMR0
	 PIT_TCTRL0 = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;     // Enable timer and timer interrupt

}

int main(void)
{
    MCUInit();
    PortsInit();
    PITInit(time);
    char *text = "FIT";

    while (1) {
        //nul_all();
    	print_text(text);
    }

    return 0;
}

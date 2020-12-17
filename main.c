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
unsigned int compare = 0x200;
int count_cols = 0;
int offset = 0;

/* Variable delay loop */
void delay(long long bound)
{
	int c, d;

	   for (c = 1; c <= bound; c++)
	       for (d = 1; d <= bound; d++)
	       {}
}


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

    NVIC_ClearPendingIRQ(PORTA_IRQn);      /* Vynuluj priznak prerusenia od portu A  */
    NVIC_EnableIRQ(PORTA_IRQn);        /* Povol prerusenie od portu A          */

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

void print_text(char* text){
	int actual = offset;
	int next = 0;
	int letter = 0;
	for (int i = 0; i < 16;i++){
		actual++;
		if (actual == 6) {
			actual = 0;
			letter++;
		}
		printChar(text[letter],i);
	}
}


void PIT_IRQHandler(void)
{
	//PIT_TCTRL0 = 0;     // Disable timer
	offset++;
	PIT_TFLG0 |= PIT_TFLG_TIF_MASK;     // Clear the timer interrupt flag
	NVIC_EnableIRQ(PIT0_IRQn);
	PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK;     // Enable timer
	/*LPTMR0_CMR = compare / 100;
	LPTMR0_CSR |=  LPTMR_CSR_TCF_MASK;
	offset++;
	print_text("XNO");
	if (count_cols == 6)
		count_cols = 0;*/

}

void PITInit(int count)
{
    SIM_SCGC6 |= SIM_SCGC6_PIT_MASK; // Enable clock to PIT
    PIT_LDVAL0 = 3297839; // set starting value
    PIT_MCR = PIT_MCR_FRZ_MASK;     // Enable clock for timer

    NVIC_EnableIRQ(PIT0_IRQn);         // enable interrupts from LPTMR0
    PIT_TCTRL0 = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;     // Enable timer and timer interrupt
    //NVIC_ICPR |= 1 << ((1 - 16) % 32); // enable PIT
    //NVIC_ISER |= 1 << ((1 - 16) % 32);  // enable PIT
    //LPTMR0_CSR &= ~LPTMR_CSR_TEN_MASK;   // Turn OFF LPTMR to perform setup
    //LPTMR0_PSR = ( LPTMR_PSR_PRESCALE(0) // 0000 is div 2
    //             | LPTMR_PSR_PBYP_MASK   // LPO feeds directly to LPT
    //             | LPTMR_PSR_PCS(1)) ;   // use the choice of clock
    //LPTMR0_CSR =(  LPTMR_CSR_TCF_MASK    // Clear any pending interrupt (now)
    //             | LPTMR_CSR_TIE_MASK    // LPT interrupt enabled
    //            );
    //NVIC_EnableIRQ(LPTMR0_IRQn);         // enable interrupts from LPTMR0
    //LPTMR0_CSR |= LPTMR_CSR_TEN_MASK;    // Turn ON LPTMR0 and start counting
}

int main(void)
{
    MCUInit();
    PortsInit();
    PITInit(compare);
    //TODO INIT PIT casovac

    while (1) {
        //nul_all();
    	print_text("PETE");
    }

    return 0;
}

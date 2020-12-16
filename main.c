//       An example for demonstrating basic principles of FITkit3 usage.
//
// It includes GPIO - inputs from button press/release, outputs for LED control,
// timer in output compare mode for generating periodic events (via interrupt 
// service routine) and speaker handling (via alternating log. 0/1 through
// GPIO output on a reasonable frequency). Using this as a basis for IMP projects
// as well as for testing basic FITkit3 operation is strongly recommended.
//
//            (c) 2019 Michal Bidlo, BUT FIT, bidlom@fit.vutbr.cz
////////////////////////////////////////////////////////////////////////////
/* Header file with all the essential definitions for a given type of MCU */
#include "MK60D10.h"
#include "alfabet.c"

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
int beep_flag = 0;
unsigned int compare = 0x200;

/* A delay function */
void delay(long long bound) {

  long long i;
  for(i=0;i<bound;i++);
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
    SIM->SCGC5 = SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK | SIM_SCGC5_PORTA_MASK;

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

    //NVIC_ClearPendingIRQ(PORTA_IRQn);      /* Vynuluj priznak prerusenia od portu A  */
    //NVIC_EnableIRQ(PORTA_IRQn);        /* Povol prerusenie od portu A          */

    //PTE->PDOR =
}

void setColumn(int index){
	switch(index) {
	case 0:
		PTA->PDOR &= ~A0;
		PTA->PDOR &= ~A1;
		PTA->PDOR &= ~A2;
		PTA->PDOR &= ~A3;
		break;
	case 1:
		PTA->PDOR |= A0;
		PTA->PDOR &= ~A1;
		PTA->PDOR &= ~A2;
		PTA->PDOR &= ~A3;
		break;
	case 2:
		PTA->PDOR &= ~A0;
		PTA->PDOR |= A1;
		PTA->PDOR &= ~A2;
		PTA->PDOR &= ~A3;
		break;
	}
}

void printText(int text) {
	switch(text){
	case 'x':
		//setColumn(0);
		//PTA->PDOR |= R2;
		//PTA->PDOR |= R1;
		//PTA->PDOR |= R3;
		setColumn(2);
		break;

	}
}

void LPTMR0_IRQHandler(void)
{
    // Set new compare value set by up/down buttons
    /*LPTMR0_CMR = compare;                // !! the CMR reg. may only be changed while TCF == 1
    LPTMR0_CSR |=  LPTMR_CSR_TCF_MASK;   // writing 1 to TCF tclear the flag
    GPIOA_PDOR ^= R0;                // invert D9 state
    //GPIOB_PDOR ^= A;               // invert D10 state
    //GPIOB_PDOR ^= LED_D11;               // invert D11 state
    //GPIOB_PDOR ^= LED_D12;               // invert D12 state
    beep_flag = !beep_flag;              // see beep_flag test in main()*/
}

void LPTMR0Init(int count)
{
    SIM_SCGC5 |= SIM_SCGC5_LPTIMER_MASK; // Enable clock to LPTMR
    LPTMR0_CSR &= ~LPTMR_CSR_TEN_MASK;   // Turn OFF LPTMR to perform setup
    LPTMR0_PSR = ( LPTMR_PSR_PRESCALE(0) // 0000 is div 2
                 | LPTMR_PSR_PBYP_MASK   // LPO feeds directly to LPT
                 | LPTMR_PSR_PCS(1)) ;   // use the choice of clock
    LPTMR0_CMR = count;                  // Set compare value
    LPTMR0_CSR =(  LPTMR_CSR_TCF_MASK    // Clear any pending interrupt (now)
                 | LPTMR_CSR_TIE_MASK    // LPT interrupt enabled
                );
    NVIC_EnableIRQ(LPTMR0_IRQn);         // enable interrupts from LPTMR0
    LPTMR0_CSR |= LPTMR_CSR_TEN_MASK;    // Turn ON LPTMR0 and start counting
}

int main(void)
{
    MCUInit();
    PortsInit();
    //LPTMR0Init(compare);

    while (1) {
    	//LPTMR0_IRQHandler;
        printText('x');
    }

    return 0;
}

/* startup.c */

#include <stdint.h>
#include <newlib.h>

/**** DANGER CRP3 WILL LOCK PART TO ALL READS and WRITES ****/
/*********** #define CRP3_MAGIC xxxx 0x43218765 *************/
__attribute__ ((used,section(".crp"))) const uint32_t CRP_WORD = 0xFFFFFFFF;

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler Function Prototype
 *----------------------------------------------------------------------------*/
typedef void( *pFunc )( void );

/* Addresses pulled in from the linker script */
extern  uint32_t __lpc17xx_checksum;  // Checksum for the lpc1788 nxp bootloader
extern  uint32_t  __stack; // stack top

/*----------------------------------------------------------------------------
  Default Handler for Exceptions / Interrupts
 *----------------------------------------------------------------------------*/
void __lpc1788_default_isr_handler(void) {
	while(1)
	{
		 __asm__ volatile("NOP\n"); // breakpoint can be place here
	}
}

/*----------------------------------------------------------------------------
  Exception / Interrupt Handler
 *----------------------------------------------------------------------------*/
/* Cortex-M3 Processor Exceptions */
void __lpc1788_isr_reset(void) __attribute__((section (".isr_reset"),naked,noreturn));
void __lpc1788_isr_nmi(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_hard_fault(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_mpu_fault(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_bus_fault(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_usage_fault(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_reserved_10(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_svcall(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_debug(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_reserved_13(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_pendsv(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_systick(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
/* LPC1788 Specific Interrupts */
void __lpc1788_isr_watchdog(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_timer0(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_timer1(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_timer2(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_timer3(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_uart0(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_uart1(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_uart2(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_uart3(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_pwm1(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_i2c0(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_i2c1(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_i2c2(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_reserved_29(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_ssp0(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_ssp1(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_pll0(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_rtc(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_external0(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_external1(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_external2(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_external3(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_adc(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_bod(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_usb(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_can(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_gpdma(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_i2s(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_ethernet(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_sdmmc(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_motor_control_pwm(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_quadrature_encoder(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_pll1(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_usb_activity(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_can_activity(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_uart4(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_ssp2(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_lcd(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_gpio(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_pwm0(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));
void __lpc1788_isr_eeprom(void) __attribute__ ((weak, alias("__lpc1788_default_isr_handler")));

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector table
 *----------------------------------------------------------------------------*/
const pFunc lpc1788_vector_table[] __attribute__ ((used, section(".isr_vector"))) = {
  /* Cortex-M3 Exceptions Handler */
  (pFunc)&__stack,                       /*      Initial Stack Pointer     */
  __lpc1788_isr_reset,                            /*      Reset Handler             */
  __lpc1788_isr_nmi,                              /*      NMI Handler               */
  __lpc1788_isr_hard_fault,                        /*      Hard Fault Handler        */
  __lpc1788_isr_mpu_fault,                        /*      MPU Fault Handler         */
  __lpc1788_isr_bus_fault,                       /*      Bus Fault Handler         */
  __lpc1788_isr_usage_fault,                   /*      Usage Fault Handler       */
  (pFunc)&__lpc17xx_checksum,                  /*   checksum for bootloader                  */
  __lpc1788_isr_reserved_10,                         /*      size of image for bootloader                  */
  __lpc1788_isr_reserved_10,                         /*      Reserved                  */
  __lpc1788_isr_reserved_10,
  	__lpc1788_isr_svcall,
  	__lpc1788_isr_debug,
  	__lpc1788_isr_reserved_13,
  	__lpc1788_isr_pendsv,
  	__lpc1788_isr_systick,

	/* LPC1788 Specific Interrupts */
	 __lpc1788_isr_watchdog,
	 __lpc1788_isr_timer0,
	 __lpc1788_isr_timer1,
	 __lpc1788_isr_timer2,
	 __lpc1788_isr_timer3,
	 __lpc1788_isr_uart0,
	 __lpc1788_isr_uart1,
	 __lpc1788_isr_uart2,
	 __lpc1788_isr_uart3,
	 __lpc1788_isr_pwm1,
	 __lpc1788_isr_i2c0,
	 __lpc1788_isr_i2c1,
	 __lpc1788_isr_i2c2,
	 __lpc1788_isr_reserved_29,
	 __lpc1788_isr_ssp0,
	 __lpc1788_isr_ssp1,
	 __lpc1788_isr_pll0,
	 __lpc1788_isr_rtc,
	 __lpc1788_isr_external0,
	 __lpc1788_isr_external1,
	 __lpc1788_isr_external2,
	 __lpc1788_isr_external3,
	 __lpc1788_isr_adc,
	 __lpc1788_isr_bod,
	 __lpc1788_isr_usb,
	 __lpc1788_isr_can,
	 __lpc1788_isr_gpdma,
	 __lpc1788_isr_i2s,
	 __lpc1788_isr_ethernet,
	 __lpc1788_isr_sdmmc,
	 __lpc1788_isr_motor_control_pwm,
	 __lpc1788_isr_quadrature_encoder,
	 __lpc1788_isr_pll1,
	 __lpc1788_isr_usb_activity,
	 __lpc1788_isr_can_activity,
	 __lpc1788_isr_uart4,
	 __lpc1788_isr_ssp2,
	 __lpc1788_isr_lcd,
	 __lpc1788_isr_gpio,
	 __lpc1788_isr_pwm0,
	 __lpc1788_isr_eeprom,
	 __lpc1788_isr_svcall,
	 __lpc1788_isr_svcall,
/*
 *  Others interrupts vectors are not used in bootloader and are treated as
 *  program area.
 */
};
//-----------------------------------------------------------------------
//
//
//  C init procedure
//
//-----------------------------------------------------------------------


/* Addresses pulled in from the linker script */
/* These magic symbols are provided by the linker.  */
extern uint32_t __etext;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

/* Application main() called in reset handler */
#if    !HAVE_INITFINI_ARRAY
extern int _start(void) __attribute__((noreturn));
#endif

#if HAVE_INITFINI_ARRAY
extern void __libc_init_array (void);
extern int main(void);
#endif

#include "chip.h"


/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/* System oscillator rate and RTC oscillator rate */
const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;

/* Setup system clocking */
static void Board_SetupClocking(void)
{
	/* Enable PBOOST for CPU clock over 100MHz */
	Chip_SYSCTL_EnableBoost();
	Chip_SetupXtalClocking();
	//Chip_SystemInit();
}

// The first executed routine.
void __lpc1788_isr_reset(void) {
    uint8_t *src, *dst;

    /* Copy with byte pointers to obviate unaligned access problems */

    /* Copy data section from Flash to RAM */
    src = (uint8_t *)&__etext;
    dst = (uint8_t *)&__data_start__;
    while (dst < (uint8_t *)&__data_end__)
        *dst++ = *src++;

    /* Clear the bss section */
    dst = (uint8_t *)&__bss_start__;
    while (dst < (uint8_t *)&__bss_end__)
        *dst++ = 0;

	Chip_SYSCTL_Map(REMAP_USER_FLASH_MODE);
	Board_SetupClocking();

	/* Generic Initialization */
	SystemCoreClockUpdate();

#if HAVE_INITFINI_ARRAY
    // Jump to the newlib init function
    __libc_init_array();
    // call main function
    (void)main();
    // something terrible went wrong
    for(;;)
    {
    	__asm volatile ("NOP\n");
    }
#endif

#if   !HAVE_INITFINI_ARRAY
    // Use the CRT0 functions.
    _start();
#endif

}

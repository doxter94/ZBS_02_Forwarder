OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
SEARCH_DIR(.)

/* Library configurations */
GROUP(libgcc.a libc.a libm.a libnosys.a)


MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 512K
  RAM (rwx)  : ORIGIN = 0x10000000, LENGTH = 64K
}


/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(__lpc1788_isr_reset)
/* Calculate the checksum for nxp bootloader. The "6" is because symbols are even and should be odd. The thumb mode.*/
PROVIDE(__lpc17xx_checksum = -(ABSOLUTE(__stack) + ABSOLUTE(__lpc1788_isr_reset) + ABSOLUTE(__lpc1788_isr_nmi) + ABSOLUTE(__lpc1788_isr_hard_fault) + ABSOLUTE(__lpc1788_isr_mpu_fault) + ABSOLUTE(__lpc1788_isr_bus_fault) + ABSOLUTE(__lpc1788_isr_usage_fault) + 6));
/* Set stack top to end of RAM, and stack limit move down by
 * size of stack_dummy section */
 /* The last 1KB is used for interrupts but for init process, use it*/
__StackTop = ORIGIN(RAM) + LENGTH(RAM);
/*__StackLimit = __StackTop - SIZEOF(.stack_dummy);*/
PROVIDE(__stack = __StackTop);

/* Used for bootlader buffer check */
_region_start_ram = ORIGIN(RAM);
_region_end_ram = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
	.text :
	{
		CREATE_OBJECT_SYMBOLS
		_ftext = .;	
		KEEP(*(.isr_vector))
		/* Make sure we pulled in an interrupt vector.  */
    	ASSERT (. != _ftext, "No interrupt vector");
	/* Code Read Protect data */	 
	. = 0x000002FC ;
	PROVIDE(__CRP_WORD_START__ = .) ;
	KEEP(*(.crp))
	PROVIDE(__CRP_WORD_END__ = .) ;
	ASSERT(!(__CRP_WORD_START__ == __CRP_WORD_END__), "Linker CRP Enabled, but no CRP_WORD provided within application");
	/* End of Code Read Protect */
	
	KEEP(*(.isr_reset))
	*(.text*)

		KEEP(*(.init))
		KEEP(*(.fini))

		/* .ctors */
		*crtbegin.o(.ctors)
		*crtbegin?.o(.ctors)
		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
		*(SORT(.ctors.*))
		*(.ctors)

		/* .dtors */
 		*crtbegin.o(.dtors)
 		*crtbegin?.o(.dtors)
 		*(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
 		*(SORT(.dtors.*))
 		*(.dtors)

		*(.rodata*)

		KEEP(*(.eh_frame*))
	} > FLASH

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*)
	} > FLASH

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	__exidx_end = .;

	/* To copy multiple ROM to RAM sections,
	 * uncomment .copy.table section and,
	 * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
	/*
	.copy.table :
	{
		. = ALIGN(4);
		__copy_table_start__ = .;
		LONG (__etext)
		LONG (__data_start__)
		LONG (__data_end__ - __data_start__)
		LONG (__etext2)
		LONG (__data2_start__)
		LONG (__data2_end__ - __data2_start__)
		__copy_table_end__ = .;
	} > FLASH
	*/

	/* To clear multiple BSS sections,
	 * uncomment .zero.table section and,
	 * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
	/*
	.zero.table :
	{
		. = ALIGN(4);
		__zero_table_start__ = .;
		LONG (__bss_start__)
		LONG (__bss_end__ - __bss_start__)
		LONG (__bss2_start__)
		LONG (__bss2_end__ - __bss2_start__)
		__zero_table_end__ = .;
	} > FLASH
	*/

	__etext = .;

	.data : AT (__etext)
	{
		__data_start__ = .;
		*(vtable)
		*(.data*)

		. = ALIGN(4);
		/* preinit data */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP(*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);

		. = ALIGN(4);
		/* init data */
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		PROVIDE_HIDDEN (__init_array_end = .);


		. = ALIGN(4);
		/* finit data */
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP(*(SORT(.fini_array.*)))
		KEEP(*(.fini_array))
		PROVIDE_HIDDEN (__fini_array_end = .);

		KEEP(*(.jcr*))
		. = ALIGN(4);
		/* All data end */
		__data_end__ = .;

	} > RAM

	.bss :
	{
		. = ALIGN(4);
		__bss_start__ = .;
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		__bss_end__ = .;
	} > RAM
	
	 /* Add no init section! */
	  /* non initialized variables */
	  .noinit (NOLOAD) : ALIGN(4)
	  {
	  	_noinit = .;
		*(.noinit)
		*(.noinit*) 
		. = ALIGN(4) ;
		_end_noinit = .;    
	  } >RAM 
	
	__end__ = .;
	end = __end__;
	/*
	.heap (COPY):
	{

		*(.heap*)
		__HeapLimit = .;
	} > RAM
*/
	/* .stack_dummy section doesn't contains any symbols. It is only
	 * used for linker to calculate size of stack sections, and assign
	 * values to stack symbols later 
	.stack_dummy (COPY):
	{
		*(.stack*)
	} > RAM
*/

	/* Check if data + heap + stack exceeds RAM limit */
	/*ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")*/
}
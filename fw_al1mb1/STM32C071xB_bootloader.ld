/*
 * STM32C071RB Application Linker Script for EngEmil STM32 Bootloader Integration
 * 
 * CRITICAL: ARM Cortex-M0+ requires vector table aligned to 256-byte boundary
 * (next power-of-2 >= vector table size of 192 bytes).
 * 
 * Memory layout:
 * 0x08004000: Application header (32 bytes)
 * 0x08004020-0x080040FF: Reserved/padding (224 bytes for alignment)
 * 0x08004100: Vector table (192 bytes, 256-byte aligned)
 * 0x080041C0: Code (.text, .rodata, etc.)
 * 
 * This script combines content from ChibiOS rules files:
 * - rules_code.ld (with 256-byte vector alignment)
 * - rules_stacks.ld
 * - rules_data.ld
 * - rules_memory.ld
 */

MEMORY
{
    flash0 (rx) : org = 0x08004100, len = 112k - 256
    flash1 (rx) : org = 0x00000000, len = 0
    flash2 (rx) : org = 0x00000000, len = 0
    flash3 (rx) : org = 0x00000000, len = 0
    flash4 (rx) : org = 0x00000000, len = 0
    flash5 (rx) : org = 0x00000000, len = 0
    flash6 (rx) : org = 0x00000000, len = 0
    flash7 (rx) : org = 0x00000000, len = 0
    ram0   (wx) : org = 0x20000000, len = 24k
    ram1   (wx) : org = 0x00000000, len = 0
    ram2   (wx) : org = 0x00000000, len = 0
    ram3   (wx) : org = 0x00000000, len = 0
    ram4   (wx) : org = 0x00000000, len = 0
    ram5   (wx) : org = 0x00000000, len = 0
    ram6   (wx) : org = 0x00000000, len = 0
    ram7   (wx) : org = 0x00000000, len = 0
}

/* Flash/RAM region aliases for ChibiOS */
REGION_ALIAS("VECTORS_FLASH", flash0);
REGION_ALIAS("VECTORS_FLASH_LMA", flash0);
REGION_ALIAS("XTORS_FLASH", flash0);
REGION_ALIAS("XTORS_FLASH_LMA", flash0);
REGION_ALIAS("TEXT_FLASH", flash0);
REGION_ALIAS("TEXT_FLASH_LMA", flash0);
REGION_ALIAS("RODATA_FLASH", flash0);
REGION_ALIAS("RODATA_FLASH_LMA", flash0);
REGION_ALIAS("VARIOUS_FLASH", flash0);
REGION_ALIAS("VARIOUS_FLASH_LMA", flash0);
REGION_ALIAS("RAM_INIT_FLASH_LMA", flash0);
REGION_ALIAS("MAIN_STACK_RAM", ram0);
REGION_ALIAS("PROCESS_STACK_RAM", ram0);
REGION_ALIAS("DATA_RAM", ram0);
REGION_ALIAS("DATA_RAM_LMA", flash0);
REGION_ALIAS("BSS_RAM", ram0);
REGION_ALIAS("HEAP_RAM", ram0);

/* Stack sizes */
PROVIDE(__process_stack_size__    = 0x400);
PROVIDE(__main_stack_size__        = 0x400);

/* Entry point */
ENTRY(Reset_Handler)

/* Memory region symbols (must be defined BEFORE SECTIONS) */
__ram0_base__           = ORIGIN(ram0);
__ram0_size__           = LENGTH(ram0);
__ram0_end__            = __ram0_base__ + __ram0_size__;
__ram1_base__           = ORIGIN(ram1);
__ram1_size__           = LENGTH(ram1);
__ram1_end__            = __ram1_base__ + __ram1_size__;
__ram2_base__           = ORIGIN(ram2);
__ram2_size__           = LENGTH(ram2);
__ram2_end__            = __ram2_base__ + __ram2_size__;
__ram3_base__           = ORIGIN(ram3);
__ram3_size__           = LENGTH(ram3);
__ram3_end__            = __ram3_base__ + __ram3_size__;
__ram4_base__           = ORIGIN(ram4);
__ram4_size__           = LENGTH(ram4);
__ram4_end__            = __ram4_base__ + __ram4_size__;
__ram5_base__           = ORIGIN(ram5);
__ram5_size__           = LENGTH(ram5);
__ram5_end__            = __ram5_base__ + __ram5_size__;
__ram6_base__           = ORIGIN(ram6);
__ram6_size__           = LENGTH(ram6);
__ram6_end__            = __ram6_base__ + __ram6_size__;
__ram7_base__           = ORIGIN(ram7);
__ram7_size__           = LENGTH(ram7);
__ram7_end__            = __ram7_base__ + __ram7_size__;

__flash0_base__         = ORIGIN(flash0);
__flash0_size__         = LENGTH(flash0);
__flash0_end__          = __flash0_base__ + __flash0_size__;
__flash1_base__         = ORIGIN(flash1);
__flash1_size__         = LENGTH(flash1);
__flash1_end__          = __flash1_base__ + __flash1_size__;
__flash2_base__         = ORIGIN(flash2);
__flash2_size__         = LENGTH(flash2);
__flash2_end__          = __flash2_base__ + __flash2_size__;
__flash3_base__         = ORIGIN(flash3);
__flash3_size__         = LENGTH(flash3);
__flash3_end__          = __flash3_base__ + __flash3_size__;
__flash4_base__         = ORIGIN(flash4);
__flash4_size__         = LENGTH(flash4);
__flash4_end__          = __flash4_base__ + __flash4_size__;
__flash5_base__         = ORIGIN(flash5);
__flash5_size__         = LENGTH(flash5);
__flash5_end__          = __flash5_base__ + __flash5_size__;
__flash6_base__         = ORIGIN(flash6);
__flash6_size__         = LENGTH(flash6);
__flash6_end__          = __flash6_base__ + __flash6_size__;
__flash7_base__         = ORIGIN(flash7);
__flash7_size__         = LENGTH(flash7);
__flash7_end__          = __flash7_base__ + __flash7_size__;

SECTIONS
{
    /* ========== Application header (bootloader requirement) ========== */
    .app_header 0x08004000 : AT(0x08004000)
    {
        KEEP(*(.app_header))
    }
    
    /* ========== Code sections (from rules_code.ld, modified) ========== */
    
    /* CRITICAL: ARM Cortex-M0+ requires 256-byte alignment for vector table */
    .vectors : ALIGN(256)
    {
        __textvectors_base__ = LOADADDR(.vectors);
        __vectors_base__ = .;
        KEEP(*(.vectors))
        __vectors_end__ = .;
    } > VECTORS_FLASH AT > VECTORS_FLASH_LMA

    .xtors : ALIGN(4)
    {
        __init_array_base__ = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end__ = .;
        __fini_array_base__ = .;
        KEEP(*(.fini_array))
        KEEP(*(SORT(.fini_array.*)))
        __fini_array_end__ = .;
    } > XTORS_FLASH AT > XTORS_FLASH_LMA

    .text : ALIGN_WITH_INPUT
    {
        __text_base__ = .;
        *(.text)
        *(.text.*)
        *(.glue_7t)
        *(.glue_7)
        *(.gcc*)
        __text_end__ = .;
    } > TEXT_FLASH AT > TEXT_FLASH_LMA

    .rodata : ALIGN(4)
    {
        __rodata_base__ = .;
        *(.rodata)
        *(.rodata.*)
        . = ALIGN(4);
        __rodata_end__ = .;
    } > RODATA_FLASH AT > RODATA_FLASH_LMA

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > VARIOUS_FLASH AT > VARIOUS_FLASH_LMA

    .ARM.exidx : {
        __exidx_base__ = .;
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end__ = .;
        __exidx_end = .;
     } > VARIOUS_FLASH AT > VARIOUS_FLASH_LMA

    .eh_frame_hdr :
    {
        *(.eh_frame_hdr)
    } > VARIOUS_FLASH AT > VARIOUS_FLASH_LMA

    .eh_frame : ONLY_IF_RO
    {
        *(.eh_frame)
    } > VARIOUS_FLASH AT > VARIOUS_FLASH_LMA

    /* ========== Stack sections (from rules_stacks.ld) ========== */
    
    .mstack (NOLOAD) :
    {
        . = ALIGN(8);
        __main_stack_base__ = .;
        . += __main_stack_size__;
        . = ALIGN(8);
        __main_stack_end__ = .;
    } > MAIN_STACK_RAM

    .pstack (NOLOAD) :
    {
        . = ALIGN(8);
        __process_stack_base__ = .;
        __main_thread_stack_base__ = .;
        . += __process_stack_size__;
        . = ALIGN(8);
        __process_stack_end__ = .;
        __main_thread_stack_end__ = .;
    } > PROCESS_STACK_RAM

    /* ========== Data sections (from rules_data.ld) ========== */
    
    .data : ALIGN(4)
    {
        PROVIDE(_textdata = LOADADDR(.data));
        PROVIDE(_data = .);
        __textdata_base__ = LOADADDR(.data);
        __data_base__ = .;
        *(.data)
        *(.data.*)
        *(.ramtext)
        . = ALIGN(4);
        PROVIDE(_edata = .);
        __data_end__ = .;
    } > DATA_RAM AT > DATA_RAM_LMA

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_base__ = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
        PROVIDE(end = .);
    } > BSS_RAM
    
    /* ========== Memory regions (from rules_memory.ld) ========== */
    
    /* RAM region 0 */
    .ram0_init : ALIGN(4)
    {
        __ram0_init_text__ = LOADADDR(.ram0_init);
        __ram0_init__ = .;
        KEEP(*(.ram0_init))
        KEEP(*(.ram0_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram0 (NOLOAD) : ALIGN(4)
    {
        . = ALIGN(4);
        __ram0_clear__ = .;
        *(.ram0_clear)
        *(.ram0_clear.*)
        . = ALIGN(4);
        __ram0_noinit__ = .;
        *(.ram0)
        *(.ram0.*)
        . = ALIGN(4);
        __ram0_free__ = .;
    } > DATA_RAM

    /* RAM regions 1-7: Not used, but sections REQUIRED by ChibiOS startup code
     * CRITICAL: These regions have LENGTH=0, so we place sections in DATA_RAM
     * to ensure ChibiOS __init_ram_areas() loop doesn't crash on null pointers */
    .ram1_init : ALIGN(4)
    {
        __ram1_init_text__ = LOADADDR(.ram1_init);
        __ram1_init__ = .;
        KEEP(*(.ram1_init))
        KEEP(*(.ram1_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram1 (NOLOAD) : ALIGN(4)
    {
        __ram1_clear__ = .;
        *(.ram1_clear)
        *(.ram1_clear.*)
        . = ALIGN(4);
        __ram1_noinit__ = .;
        *(.ram1)
        *(.ram1.*)
        . = ALIGN(4);
        __ram1_free__ = .;
    } > DATA_RAM
    
    .ram2_init : ALIGN(4)
    {
        __ram2_init_text__ = LOADADDR(.ram2_init);
        __ram2_init__ = .;
        KEEP(*(.ram2_init))
        KEEP(*(.ram2_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram2 (NOLOAD) : ALIGN(4)
    {
        __ram2_clear__ = .;
        *(.ram2_clear)
        *(.ram2_clear.*)
        . = ALIGN(4);
        __ram2_noinit__ = .;
        *(.ram2)
        *(.ram2.*)
        . = ALIGN(4);
        __ram2_free__ = .;
    } > DATA_RAM
    
    .ram3_init : ALIGN(4)
    {
        __ram3_init_text__ = LOADADDR(.ram3_init);
        __ram3_init__ = .;
        KEEP(*(.ram3_init))
        KEEP(*(.ram3_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram3 (NOLOAD) : ALIGN(4)
    {
        __ram3_clear__ = .;
        *(.ram3_clear)
        *(.ram3_clear.*)
        . = ALIGN(4);
        __ram3_noinit__ = .;
        *(.ram3)
        *(.ram3.*)
        . = ALIGN(4);
        __ram3_free__ = .;
    } > DATA_RAM
    
    .ram4_init : ALIGN(4)
    {
        __ram4_init_text__ = LOADADDR(.ram4_init);
        __ram4_init__ = .;
        KEEP(*(.ram4_init))
        KEEP(*(.ram4_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram4 (NOLOAD) : ALIGN(4)
    {
        __ram4_clear__ = .;
        *(.ram4_clear)
        *(.ram4_clear.*)
        . = ALIGN(4);
        __ram4_noinit__ = .;
        *(.ram4)
        *(.ram4.*)
        . = ALIGN(4);
        __ram4_free__ = .;
    } > DATA_RAM
    
    .ram5_init : ALIGN(4)
    {
        __ram5_init_text__ = LOADADDR(.ram5_init);
        __ram5_init__ = .;
        KEEP(*(.ram5_init))
        KEEP(*(.ram5_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram5 (NOLOAD) : ALIGN(4)
    {
        __ram5_clear__ = .;
        *(.ram5_clear)
        *(.ram5_clear.*)
        . = ALIGN(4);
        __ram5_noinit__ = .;
        *(.ram5)
        *(.ram5.*)
        . = ALIGN(4);
        __ram5_free__ = .;
    } > DATA_RAM
    
    .ram6_init : ALIGN(4)
    {
        __ram6_init_text__ = LOADADDR(.ram6_init);
        __ram6_init__ = .;
        KEEP(*(.ram6_init))
        KEEP(*(.ram6_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram6 (NOLOAD) : ALIGN(4)
    {
        __ram6_clear__ = .;
        *(.ram6_clear)
        *(.ram6_clear.*)
        . = ALIGN(4);
        __ram6_noinit__ = .;
        *(.ram6)
        *(.ram6.*)
        . = ALIGN(4);
        __ram6_free__ = .;
    } > DATA_RAM
    
    .ram7_init : ALIGN(4)
    {
        __ram7_init_text__ = LOADADDR(.ram7_init);
        __ram7_init__ = .;
        KEEP(*(.ram7_init))
        KEEP(*(.ram7_init.*))
        . = ALIGN(4);
    } > DATA_RAM AT > RAM_INIT_FLASH_LMA

    .ram7 (NOLOAD) : ALIGN(4)
    {
        __ram7_clear__ = .;
        *(.ram7_clear)
        *(.ram7_clear.*)
        . = ALIGN(4);
        __ram7_noinit__ = .;
        *(.ram7)
        *(.ram7.*)
        . = ALIGN(4);
        __ram7_free__ = .;
    } > DATA_RAM

    /* Heap */
    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        __heap_base__ = .;
        . = ORIGIN(HEAP_RAM) + LENGTH(HEAP_RAM);
        __heap_end__ = .;
    } > HEAP_RAM
}

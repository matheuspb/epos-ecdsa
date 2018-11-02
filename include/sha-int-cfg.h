#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C"
{
#endif

#include <hw_types.h>
#include <cc-ctrl-defines.h>

//*****************************************************************************
//
// Macro to generate an interrupt priority mask based on the number of bits
// of priority supported by the hardware.
//
//*****************************************************************************

#define INT_PRIORITY_MASK       ((0xFF << (8 - NUM_PRIORITY_BITS)) & 0xFF)


//*****************************************************************************
//
// The following are defines for the NVIC register addresses.
//
//*****************************************************************************

#define NVIC_DIS0               0xE000E180  // Interrupt 0-31 Clear Enable
#define NVIC_DIS1               0xE000E184  // Interrupt 32-54 Clear Enable
#define NVIC_DIS2               0xE000E188  // Interrupt 64-95 Clear Enable
#define NVIC_DIS3               0xE000E18C  // Interrupt 96-127 Clear Enable
#define NVIC_DIS4               0xE000E190  // Interrupt 128-131 Clear Enable

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************
//extern bool IntMasterEnable(void);
//extern bool IntMasterDisable(void);
//extern void IntRegister(uint32_t ui32Interrupt, void (*pfnHandler)(void));
//extern void IntUnregister(uint32_t ui32Interrupt);
//extern void IntPriorityGroupingSet(uint32_t ui32Bits);
//extern uint32_t IntPriorityGroupingGet(void);
//extern void IntPrioritySet(uint32_t ui32Interrupt, uint8_t ui8Priority);
//extern int32_t IntPriorityGet(uint32_t ui32Interrupt);
//extern void IntEnable(uint32_t ui32Interrupt);
//extern void IntDisable(uint32_t ui32Interrupt);
//extern void IntPendSet(uint32_t ui32Interrupt);
//extern void IntPendClear(uint32_t ui32Interrupt);
//extern void IntPriorityMaskSet(uint32_t ui32PriorityMask);
//extern uint32_t IntPriorityMaskGet(void);
//extern void IntAltMapDisable(void);
//extern bool IntAltMapIsEnabled(void);

extern void IntAltMapEnable(void)
{
    HWREG(SYS_CTRL_I_MAP) |= SYS_CTRL_I_MAP_ALTMAP;
}

//*****************************************************************************
// This is a mapping between interrupt number (for the peripheral interrupts
// only) and the register that contains the interrupt disable for that
// interrupt.
//
//*****************************************************************************

static const uint32_t g_pui32DisRegs[] =
{
    NVIC_DIS0, NVIC_DIS1, NVIC_DIS2, NVIC_DIS3, NVIC_DIS4
};

//*****************************************************************************
//
// The following are defines for the interrupt assignments.
//
//*****************************************************************************

#define INT_GPIOA               16          // GPIO Port A
#define INT_GPIOB               17          // GPIO Port B
#define INT_GPIOC               18          // GPIO Port C
#define INT_GPIOD               19          // GPIO Port D
#define INT_UART0               21          // UART0 Rx and Tx
#define INT_UART1               22          // UART1 Rx and Tx
#define INT_SSI0                23          // SSI0 Rx and Tx
#define INT_I2C0                24          // I2C0 Master and Slave
#define INT_ADC0                30          // ADC0 Sequence 0
#define INT_WATCHDOG            34          // Watchdog timer
#define INT_WATCHDOG0           34          // Watchdog Timer0
#define INT_TIMER0A             35          // Timer 0 subtimer A
#define INT_TIMER0B             36          // Timer 0 subtimer B
#define INT_TIMER1A             37          // Timer 1 subtimer A
#define INT_TIMER1B             38          // Timer 1 subtimer B
#define INT_TIMER2A             39          // Timer 2 subtimer A
#define INT_TIMER2B             40          // Timer 2 subtimer B
#define INT_COMP0               41          // Analog Comparator 0

#ifdef CC2538_USE_ALTERNATE_INTERRUPT_MAP
#define INT_RFCORERTX           42           // RFCORE RX/TX
#define INT_RFCOREERR           43           // RFCORE Error
#define INT_ICEPICK             44           // Icepick
#endif // CC2538_USE_ALTERNATE_INTERRUPT_MAP

#define INT_FLASH               45          // FLASH Control

#ifdef CC2538_USE_ALTERNATE_INTERRUPT_MAP
#define INT_AES                 46           // AES
#define INT_PKA                 47           // PKA
#define INT_SMTIM               48           // SMTimer
#define INT_MACTIMR             49           // MACTimer
#endif // CC2538_USE_ALTERNATE_INTERRUPT_MAP

#define INT_SSI1                50          // SSI1 Rx and Tx
#define INT_TIMER3A             51          // Timer 3 subtimer A
#define INT_TIMER3B             52          // Timer 3 subtimer B

#ifdef CC2538_USE_ALTERNATE_INTERRUPT_MAP
#define INT_USB2538             60           // USB new for 2538
#endif // CC2538_USE_ALTERNATE_INTERRUPT_MAP

#define INT_UDMA                62          // uDMA controller
#define INT_UDMAERR             63          // uDMA Error

#ifndef CC2538_USE_ALTERNATE_INTERRUPT_MAP
#define INT_USB2538            156          // USB new for 2538
#define INT_RFCORERTX          157          // RFCORE RX/TX
#define INT_RFCOREERR          158          // RFCORE Error
#define INT_AES                159          // AES
#define INT_PKA                160          // PKA
#define INT_SMTIM              161          // SMTimer
#define INT_MACTIMR            162          // MACTimer
#endif // not CC2538_USE_ALTERNATE_INTERRUPT_MAP

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __INTERRUPT_H__

#ifndef __SHAINIT_H__
#define __SHAINIT_H__

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

#include <system/config.h>
#include <stdint.h>
#include <cc-ctrl-defines.h>
#include <hw_types.h>

//*****************************************************************************
//
// Arrays that maps the "peripheral set" number (which is stored in the
// third nibble of the SYS_CTRL_PERIPH_* defines) to the SYSCTL register that
// contains the relevant bit for that peripheral.
//
//*****************************************************************************

// Run mode registers
static const uint32_t g_pui32RCGCRegs[] =
{
    SYS_CTRL_RCGCGPT,
    SYS_CTRL_RCGCSSI,
    SYS_CTRL_RCGCUART,
    SYS_CTRL_RCGCI2C,
    SYS_CTRL_RCGCSEC,
    SYS_CTRL_RCGCRFC
};

// Reset registers
static const uint32_t g_pui32SRRegs[] =
{
    SYS_CTRL_SRGPT,
    SYS_CTRL_SRSSI,
    SYS_CTRL_SRUART,
    SYS_CTRL_SRI2C,
    SYS_CTRL_SRSEC,
};

//*****************************************************************************
//
// This macro extracts the array index out of the peripheral number.
//
//*****************************************************************************

#define SYS_CTRL_PERIPH_INDEX(a)  (((a) >> 8) & 0xf)

// This macro extracts the peripheral instance number and generates bit mask
//
//*****************************************************************************

#define SYS_CTRL_PERIPH_MASKBIT(a) (0x00000001 << ((a) & 0xf))

//*****************************************************************************
//
//! Performs a software reset of a peripheral
//!
//! \param ui32Peripheral is the peripheral to reset.
//!
//! This function performs a software reset of the specified peripheral.  An
//! individual peripheral reset signal is asserted for a brief period and then
//! deasserted, leaving the peripheral in a operating state but in its reset
//! condition.
//!
//! The \e ui32Peripheral parameter must be one of the values:
//! \b SYS_CTRL_PERIPH_GPT0 , \b SYS_CTRL_PERIPH_GPT1,
//! \b SYS_CTRL_PERIPH_GPT2,  \b SYS_CTRL_PERIPH_GPT3,
//! \b SYS_CTRL_PERIPH_SSI0,  \b SYS_CTRL_PERIPH_SSI1,
//! \b SYS_CTRL_PERIPH_UART0, \b SYS_CTRL_PERIPH_UART1,
//! \b SYS_CTRL_PERIPH_I2C,   \b SYS_CTRL_PERIPH_PKA,
//! \b SYS_CTRL_PERIPH_AES.
//!
//! \return None
//
//*****************************************************************************

extern void SysCtrlPeripheralReset(uint32_t ui32Peripheral)
{
    volatile uint32_t ui32Delay;

    // Check the arguments.
    assert(SysCtrlPeripheralValid(ui32Peripheral));
    assert(!(ui32Peripheral == SYS_CTRL_PERIPH_RFC));

    // Put the peripheral into the reset state.
    HWREG(g_pui32SRRegs[SYS_CTRL_PERIPH_INDEX(ui32Peripheral)]) |=
        SYS_CTRL_PERIPH_MASKBIT(ui32Peripheral);

    // Delay for a little bit.
    for(ui32Delay = 0; ui32Delay < 16; ui32Delay++) { }

    // Take the peripheral out of the reset state.
    HWREG(g_pui32SRRegs[SYS_CTRL_PERIPH_INDEX(ui32Peripheral)]) &=
        ~SYS_CTRL_PERIPH_MASKBIT(ui32Peripheral);
}


//*****************************************************************************
//
//! Enables a peripheral (in Run mode)
//!
//! \param ui32Peripheral is the peripheral to enable.
//!
//! Peripherals are enabled with this function.  At power-up, some peripherals
//! are disabled and must be enabled to operate or respond to
//! register reads/writes.
//!
//! The \e ui32Peripheral parameter must be one of the values:
//! \b SYS_CTRL_PERIPH_GPT0 , \b SYS_CTRL_PERIPH_GPT1,
//! \b SYS_CTRL_PERIPH_GPT2,  \b SYS_CTRL_PERIPH_GPT3,
//! \b SYS_CTRL_PERIPH_SSI0,  \b SYS_CTRL_PERIPH_SSI1,
//! \b SYS_CTRL_PERIPH_UART0, \b SYS_CTRL_PERIPH_UART1,
//! \b SYS_CTRL_PERIPH_I2C,   \b SYS_CTRL_PERIPH_PKA,
//! \b SYS_CTRL_PERIPH_AES,   \b SYS_CTRL_PERIPH_RFC.
//!
//! \note The actual enabling of the peripheral might be delayed until some
//! time after this function returns. Ensure that the peripheral is not
//! accessed until enabled.
//!
//! \return None
//
//*****************************************************************************

void SysCtrlPeripheralEnable(uint32_t ui32Peripheral)
{
    // Check the arguments.
    assert(SysCtrlPeripheralValid(ui32Peripheral));

    // Enable module in Run Mode
    HWREG(g_pui32RCGCRegs[SYS_CTRL_PERIPH_INDEX(ui32Peripheral)]) |=
        SYS_CTRL_PERIPH_MASKBIT(ui32Peripheral);
}


#ifdef __cplusplus
}
#endif

#endif // __SYS_CTRL_H__

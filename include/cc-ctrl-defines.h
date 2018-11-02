#ifndef __CCCTRLDEFINE_H__
#define __CCCTRLDEFINE_H__

//*****************************************************************************
//
// The following are defines for the SYS_CTRL register offsets.
//
//*****************************************************************************

#define SYS_CTRL_CLOCK_CTRL     0x400D2000  // The clock control register
                                            // handels clock settings in the
                                            // CC2538. The settings in
                                            // CLOCK_CTRL do not always reflect
                                            // the current chip status which is
                                            // found in CLOCK_STA register.
#define SYS_CTRL_CLOCK_STA      0x400D2004  // Clock status register This
                                            // register reflects the current
                                            // chip status.
#define SYS_CTRL_RCGCGPT        0x400D2008  // This register defines the
                                            // module clocks for GPT[3:0] when
                                            // the CPU is in active (run) mode.
                                            // This register setting is don't
                                            // care for PM1-3, because the
                                            // system clock is powered down in
                                            // these modes.
#define SYS_CTRL_SCGCGPT        0x400D200C  // This register defines the
                                            // module clocks for GPT[3:0] when
                                            // the CPU is in sleep mode. This
                                            // register setting is don't care
                                            // for PM1-3, because the system
                                            // clock is powered down in these
                                            // modes.
#define SYS_CTRL_DCGCGPT        0x400D2010  // This register defines the
                                            // module clocks for GPT[3:0] when
                                            // the CPU is in PM0. This register
                                            // setting is don't care for PM1-3,
                                            // because the system clock is
                                            // powered down in these modes.
#define SYS_CTRL_SRGPT          0x400D2014  // This register controls the
                                            // reset for GPT[3:0].
#define SYS_CTRL_RCGCSSI        0x400D2018  // This register defines the
                                            // module clocks for SSI[1:0] when
                                            // the CPU is in active (run) mode.
                                            // This register setting is don't
                                            // care for PM1-3, because the
                                            // system clock is powered down in
                                            // these modes.
#define SYS_CTRL_SCGCSSI        0x400D201C  // This register defines the
                                            // module clocks for SSI[1:0] when
                                            // the CPU is insSleep mode. This
                                            // register setting is don't care
                                            // for PM1-3, because the system
                                            // clock is powered down in these
                                            // modes.
#define SYS_CTRL_DCGCSSI        0x400D2020  // This register defines the
                                            // module clocks for SSI[1:0] when
                                            // the CPU is in PM0. This register
                                            // setting is don't care for PM1-3,
                                            // because the system clock is
                                            // powered down in these modes.
#define SYS_CTRL_SRSSI          0x400D2024  // This register controls the
                                            // reset for SSI[1:0].
#define SYS_CTRL_RCGCUART       0x400D2028  // This register defines the
                                            // module clocks for UART[1:0] when
                                            // the CPU is in active (run) mode.
                                            // This register setting is don't
                                            // care for PM1-3, because the
                                            // system clock is powered down in
                                            // these modes.
#define SYS_CTRL_SCGCUART       0x400D202C  // This register defines the
                                            // module clocks for UART[1:0] when
                                            // the CPU is in sleep mode. This
                                            // register setting is don't care
                                            // for PM1-3, because the system
                                            // clock is powered down in these
                                            // modes.
#define SYS_CTRL_DCGCUART       0x400D2030  // This register defines the
                                            // module clocks for UART[1:0] when
                                            // the CPU is in PM0. This register
                                            // setting is don't care for PM1-3,
                                            // because the system clock is
                                            // powered down in these modes.
#define SYS_CTRL_SRUART         0x400D2034  // This register controls the
                                            // reset for UART[1:0].
#define SYS_CTRL_RCGCI2C        0x400D2038  // This register defines the
                                            // module clocks for I2C when the
                                            // CPU is in active (run) mode.
                                            // This register setting is don't
                                            // care for PM1-3, because the
                                            // system clock is powered down in
                                            // these modes.
#define SYS_CTRL_SCGCI2C        0x400D203C  // This register defines the
                                            // module clocks for I2C when the
                                            // CPU is in sleep mode. This
                                            // register setting is don't care
                                            // for PM1-3, because the system
                                            // clock is powered down in these
                                            // modes.
#define SYS_CTRL_DCGCI2C        0x400D2040  // This register defines the
                                            // module clocks for I2C when the
                                            // CPU is in PM0. This register
                                            // setting is don't care for PM1-3,
                                            // because the system clock is
                                            // powered down in these modes.
#define SYS_CTRL_SRI2C          0x400D2044  // This register controls the
                                            // reset for I2C.
#define SYS_CTRL_RCGCSEC        0x400D2048  // This register defines the
                                            // module clocks for the security
                                            // module when the CPU is in active
                                            // (run) mode. This register
                                            // setting is don't care for PM1-3,
                                            // because the system clock is
                                            // powered down in these modes.
#define SYS_CTRL_SCGCSEC        0x400D204C  // This register defines the
                                            // module clocks for the security
                                            // module when the CPU is in sleep
                                            // mode. This register setting is
                                            // don't care for PM1-3, because
                                            // the system clock is powered down
                                            // in these modes.
#define SYS_CTRL_DCGCSEC        0x400D2050  // This register defines the
                                            // module clocks for the security
                                            // module when the CPU is in PM0.
                                            // This register setting is don't
                                            // care for PM1-3, because the
                                            // system clock is powered down in
                                            // these modes.
#define SYS_CTRL_SRSEC          0x400D2054  // This register controls the
                                            // reset for the security module.
#define SYS_CTRL_PMCTL          0x400D2058  // This register controls the
                                            // power mode. Note: The
                                            // Corresponding PM is not entered
                                            // before the WFI instruction is
                                            // asserted. To enter PM1-3 the
                                            // DEEPSLEEP bit in SYSCTRL must be
                                            // 1.
#define SYS_CTRL_SRCRC          0x400D205C  // This register controls CRC on
                                            // state retention.
#define SYS_CTRL_PWRDBG         0x400D2074  // Power debug register
#define SYS_CTRL_CLD            0x400D2080  // This register controls the
                                            // clock loss detection feature.
#define SYS_CTRL_IWE            0x400D2094  // This register controls
                                            // interrupt wake-up.
#define SYS_CTRL_I_MAP          0x400D2098  // This register selects which
                                            // interrupt map to be used.
#define SYS_CTRL_RCGCRFC        0x400D20A8  // This register defines the
                                            // module clocks for RF CORE when
                                            // the CPU is in active (run) mode.
                                            // This register setting is don't
                                            // care for PM1-3, because the
                                            // system clock is powered down in
                                            // these modes.
#define SYS_CTRL_SCGCRFC        0x400D20AC  // This register defines the
                                            // module clocks for RF CORE when
                                            // the CPU is in sleep mode. This
                                            // register setting is don't care
                                            // for PM1-3, because the system
                                            // clock is powered down in these
                                            // modes.
#define SYS_CTRL_DCGCRFC        0x400D20B0  // This register defines the
                                            // module clocks for RF CORE when
                                            // the CPU is in PM0. This register
                                            // setting is don't care for PM1-3,
                                            // because the system clock is
                                            // powered down in these modes.
#define SYS_CTRL_EMUOVR         0x400D20B4  // This register defines the
                                            // emulator override controls for
                                            // power mode and peripheral clock
                                            // gate.


//*****************************************************************************
//
// The following are values that can be passed to the
// SysCtrlPeripheralReset(), SysCtrlPeripheralEnable(),
// SysCtrlPeripheralDisable(), SysCtrlPeripheralSleepEnable(),
// SysCtrlPeripheralSleepDisable(), SysCtrlPeripheralDeepSleepEnable() and
// SysCtrlPeripheralDeepSleepDisable()  APIs as the ui32Peripheral parameter.
//
//*****************************************************************************

#define SYS_CTRL_PERIPH_GPT0       0x00000000  // General Purpose Timer 0
#define SYS_CTRL_PERIPH_GPT1       0x00000001  // General Purpose Timer 1
#define SYS_CTRL_PERIPH_GPT2       0x00000002  // General Purpose Timer 2
#define SYS_CTRL_PERIPH_GPT3       0x00000003  // General Purpose Timer 3
#define SYS_CTRL_PERIPH_SSI0       0x00000100  // SSI 0
#define SYS_CTRL_PERIPH_SSI1       0x00000101  // SSI 1
#define SYS_CTRL_PERIPH_UART0      0x00000200  // UART 0
#define SYS_CTRL_PERIPH_UART1      0x00000201  // UART 1
#define SYS_CTRL_PERIPH_I2C        0x00000300  // I2C0
#define SYS_CTRL_PERIPH_PKA        0x00000400  // PKA
#define SYS_CTRL_PERIPH_AES        0x00000401  // AES
#define SYS_CTRL_PERIPH_RFC        0x00000500  // RF Core

//*****************************************************************************
//
// The following are defines for the bit fields in the
// SYS_CTRL_I_MAP register.
//
//*****************************************************************************

#define SYS_CTRL_I_MAP_ALTMAP   0x00000001  // 1: Select alternate interrupt
                                            // map. 0: Select regular interrupt
                                            // map. (See the ASD document for
                                            // details.)
#define SYS_CTRL_I_MAP_ALTMAP_M 0x00000001
#define SYS_CTRL_I_MAP_ALTMAP_S 0

#endif // __CCCTRLDEFINE_H__

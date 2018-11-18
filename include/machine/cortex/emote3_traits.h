// EPOS EPOSMoteIII (ARM Cortex-M3) MCU Metainfo and Configuration

#ifndef __machine_traits_h
#define __machine_traits_h

#include <system/config.h>

__BEGIN_SYS

class Machine_Common;
template<> struct Traits<Machine_Common>: public Traits<void>
{
    static const bool debugged = Traits<void>::debugged;
};

class PUF;
template<> struct Traits<PUF>
{
    static const unsigned int KEY_SIZE = 256;
    static const unsigned int KEY_SIZE_BYTES = static_cast<unsigned int>((static_cast<signed int>(KEY_SIZE) - 1) + 8) / 8;
};

class BCH;
template<> struct Traits<BCH>
{
  static const unsigned int EC_KEY[];
  static const unsigned int ECC_CODE[];
  static const unsigned int CHUNK_SIZE = 256;
  static const unsigned int TEST_CHUNK_SIZE = (CHUNK_SIZE + 8);
  static const unsigned int SYNS_SIZE = 2;
  static const unsigned int GENERATOR = 0x201b;
  static const unsigned int POLY_DEGREE = 13;
  static const unsigned int ECC_BYTES = 2;
};
const unsigned int Traits<BCH>::EC_KEY[] = {
237, 0, 0, 0, 253, 0, 0, 0, 241, 0, 0, 0, 215, 0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 245, 175, 183, 234, 255, 207, 243, 127, 253, 151, 190, 127, 254, 202, 191, 63, 213, 115, 250, 219, 185, 250, 159, 55, 186, 235, 171, 253, 237, 237, 209, 251, 175, 231, 235, 113, 171, 187, 255, 255, 179, 127, 251, 255, 249, 249, 125, 247, 187, 85, 122, 183, 221, 207, 239, 123, 212, 255, 119, 76, 254, 247, 63, 253, 237, 208, 109, 238, 207, 223, 189, 47, 126, 191, 15, 187, 251, 233, 247, 155, 221, 255, 221, 46, 12, 21, 185, 227, 127, 251, 245, 181, 141, 222, 111, 190, 245, 223, 255, 221, 63, 6, 215, 237, 196, 42, 253, 253, 238, 191, 249, 239, 119, 179, 155, 222, 254, 251, 245, 4, 181, 238, 62, 57, 111, 245, 255, 223, 239, 187, 216, 59, 231, 239, 232, 184, 111, 255, 77, 181, 255, 246, 87, 221, 254, 246, 191, 126, 255, 127, 239, 191, 215, 242, 253, 103, 221, 110, 205, 251, 255, 243, 254, 187, 95, 250, 219, 187, 173, 217, 207, 211, 255, 31, 255, 251, 179, 125, 255, 243, 248, 231, 231, 253, 255, 254, 127, 109, 183, 255, 254, 223, 126, 191, 141, 100, 237, 253, 241, 199, 10, 0, 0, 0, 28, 0, 0, 32, 105, 188, 249, 111, 126, 253, 225, 247, 32, 114, 0, 32, 255, 191, 184, 175
};
const unsigned int Traits<BCH>::ECC_CODE[] = {190, 230, 117, 159, 231, 68, 32, 0,};

template<> struct Traits<Machine>: public Traits<Machine_Common>
{
    static const unsigned int CPUS = Traits<Build>::CPUS;
    static const unsigned int MEM_BEG    = 0x20000004;
    static const unsigned int MEM_END    = 0x20007ff7; // 32 KB (MAX for 32-bit is 0x70000000 / 1792 MB)
    // Physical Memory
    static const unsigned int MEM_BASE   = MEM_BEG;
    static const unsigned int MEM_TOP    = 0x20007ef7;
    static const unsigned int PUF_BASE   = MEM_TOP + 1; //One past last byte
    static const unsigned int PUF_END    = PUF_BASE + (static_cast<unsigned int>((static_cast<signed int>(Traits<PUF>::KEY_SIZE_BYTES) - 1) + 4) / 4) * 4;
    static const unsigned int FLASH_BASE = 0x00200000;
    static const unsigned int FLASH_TOP  = 0x0027ffff; // 512 KB

    // Logical Memory Map
    static const unsigned int APP_LOW   = 0x20000004;
    static const unsigned int APP_CODE  = 0x00204000;
    static const unsigned int APP_DATA  = 0x20000004;
    static const unsigned int APP_HIGH  = 0x20007ff7;

    static const unsigned int PHY_MEM   = 0x20000004;
    static const unsigned int IO_BASE   = 0x40000000;
    static const unsigned int IO_TOP    = 0x440067ff;

    static const unsigned int SYS       = 0x00204000;
    static const unsigned int SYS_CODE  = 0x00204000; // Library mode only => APP + SYS
    static const unsigned int SYS_DATA  = 0x20000004; // Library mode only => APP + SYS

    static const unsigned int FLASH_STORAGE_BASE = 0x00250000;
    static const unsigned int FLASH_STORAGE_TOP  = 0x0027f7ff;

    // Default Sizes and Quantities
    static const unsigned int STACK_SIZE = 3 * 1024;
    static const unsigned int HEAP_SIZE = 3 * 1024;
    static const unsigned int MAX_THREADS = 7;
};

template<> struct Traits<IC>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;
};

template<> struct Traits<Timer>: public Traits<Machine_Common>
{
    static const bool debugged = hysterically_debugged;

    // Meaningful values for the timer frequency range from 100 to
    // 10000 Hz. The choice must respect the scheduler time-slice, i. e.,
    // it must be higher than the scheduler invocation frequency.
    static const int FREQUENCY = 1000; // Hz
};

template<> struct Traits<UART>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 2;

    static const unsigned int CLOCK = Traits<CPU>::CLOCK;

    static const unsigned int DEF_UNIT = 0;
    static const unsigned int DEF_BAUD_RATE = 115200;
    static const unsigned int DEF_DATA_BITS = 8;
    static const unsigned int DEF_PARITY = 0; // none
    static const unsigned int DEF_STOP_BITS = 1;
};

template<> struct Traits<SPI>: public Traits<Machine_Common>
{
    static const unsigned int UNITS = 1;
};

template<> struct Traits<USB>: public Traits<Machine_Common>
{
    // Some observed objects are created before initializing the Display, which may use the USB.
    // Enabling debug may cause trouble in some Machines
    static const bool debugged = false;

    static const unsigned int UNITS = 1;
    static const bool blocking = false;
    static const bool enabled = Traits<Serial_Display>::enabled && (Traits<Serial_Display>::ENGINE == Traits<Serial_Display>::USB);
};

template<> struct Traits<Watchdog>: public Traits<Machine_Common>
{
    enum {
        MS_1_9,    // 1.9ms
        MS_15_625, // 15.625ms
        S_0_25,    // 0.25s
        S_1,       // 1s
    };
    static const int PERIOD = S_1;
};

template<> struct Traits<Smart_Plug>: public Traits<Machine_Common>
{
    static const bool enabled = false;

    enum { DIMMER, SWITCH, DISABLED };
    static const unsigned int P1_ACTUATOR = SWITCH;
    static const unsigned int P2_ACTUATOR = DIMMER;
    static const unsigned int PWM_TIMER_CHANNEL = 0;
    static const unsigned int PWM_PERIOD = 100; // us

    static const bool P1_power_meter_enabled = true;
    static const bool P2_power_meter_enabled = true;
};

template<> struct Traits<Hydro_Board>: public Traits<Machine_Common>
{
    static const bool enabled = false;

    static const unsigned int INTERRUPT_DEBOUNCE_TIME = 100000; // us

    // Enable/disable individual relays / ADCs
    static const bool P3_enabled = true;
    static const bool P4_enabled = false;
    static const bool P5_enabled = true;
    static const bool P6_enabled = true;
    static const bool P7_enabled = true;
};

template<> struct Traits<Scratchpad>: public Traits<Machine_Common>
{
    static const bool enabled = false;
};

template<> struct Traits<RFID_Reader>: public Traits<Machine_Common>
{
    enum {MFRC522, W400};
    static const int ENGINE = W400;
};

template<> struct Traits<NIC>: public Traits<Machine_Common>
{
    static const bool enabled = (Traits<Build>::NODES > 1);

    // NICS that don't have a network in Traits<Network>::NETWORKS will not be enabled
    typedef LIST<CC2538, M95> NICS;
    static const unsigned int UNITS = NICS::Length;
    static const bool promiscuous = false;
};

template<> struct Traits<CC2538>: public Traits<NIC>
{
    static const unsigned int UNITS = NICS::Count<CC2538>::Result;
    static const unsigned int RECEIVE_BUFFERS = 20; // per unit
    static const bool gpio_debug = false;
    static const bool reset_backdoor = false;
    static const unsigned int DEFAULT_CHANNEL = 15;
};

template<> struct Traits<M95>: public Traits<NIC>
{
    static const unsigned int UNITS = NICS::Count<M95>::Result;

    enum {CLARO, TIM, OI};
    static const unsigned int PROVIDER = CLARO;

    static const unsigned int UART_UNIT = 1;
    static const unsigned int UART_BAUD_RATE = 9600;
    static const unsigned int UART_DATA_BITS = 8;
    static const unsigned int UART_PARITY = 0;
    static const unsigned int UART_STOP_BITS = 1;

    static const char PWRKEY_PORT = 'C';
    static const unsigned int PWRKEY_PIN = 4;
    static const char STATUS_PORT = 'C';
    static const unsigned int STATUS_PIN = 1;
};

template<> struct Traits<A2035>: public Traits<Machine_Common>
{
    static const bool enabled = false;
};

__END_SYS

#endif

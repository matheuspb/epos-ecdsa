// EPOS ARM Cortex CAN controller Mediator Declarations

#include <gpio.h>
#include <spi.h>
#include <machine.h>
#include <cpu.h>
#include <can.h>

__BEGIN_SYS

// MCP2515 CAN controller
class CAN_Engine: public CAN_Common
{
    enum SPI_Instruction {
        WRITE         = 0x02,
        READ          = 0x03,
        BIT_MODIFY    = 0x05,
        READ_STATUS   = 0xA0,
        RX_STATUS     = 0xB0,
        RESET         = 0xC0,
        RTS           = 0x80,
        LOAD_TXB0SIDH = 0x31,
        LOAD_TXB0D0   = 0x36,
        LOAD_TXB1SIDH = 0x41,
        LOAD_TXB1D0   = 0x46,
        LOAD_TXB2SIDH = 0x51,
        LOAD_TXB2D0   = 0x56,
    };

    enum Register {
        BFPCTRL   = 0x0C,
        TXRTSCTRL = 0x0D,
        STAT      = 0x0E,
        CTRL      = 0x0F,
        RXM0SIDH  = 0x20,
        RXM0SIDL  = 0x21,
        RXM1SIDH  = 0x24,
        RXM1SIDL  = 0x25,
        CNF3      = 0x28,
        CNF2      = 0x29,
        CNF1      = 0x2A,
        INTE      = 0x2B,
        INTF      = 0x2C,
        EFLG      = 0x2D,
        TXB0CTRL  = 0x30,
        TXB0SIDH  = 0x31,
        TXB1CTRL  = 0x40,
        TXB0D0    = 0x41,
        TXB2CTRL  = 0x50,
        RXB0CTRL  = 0x60,
        RXB0D0    = 0x66, // Buffer 0 - First byte of message data
        RXB1CTRL  = 0x70,
        RXB1D0    = 0x76, // Buffer 1 - First byte of message data
        RXB0SIDH  = 0x90, // Buffer 0 - 8 most significant bits of the ID of a Standard message
        RXB0SIDL  = 0x91,
        RXB1SIDH  = 0x94, // Buffer 1 - 8 most significant bits of the ID of a Standard message
        RXB1SIDL  = 0x95,
    };

protected:
    CAN_Engine(): _spi(0, Traits<CPU>::CLOCK, SPI::FORMAT_MOTO_0, SPI::MASTER, 2000000, 8),
        _enable('D', 0, GPIO::OUT, GPIO::UP, 0),
        _interrupt('A', 6, GPIO::IN),
        _cscan('A',3,GPIO::OUT),
        _clock('A', 7, GPIO::IN)
    {
        disable();

        _cscan.set(0);
        reset();
        while(_spi.is_busy());
        _cscan.set(1);

        // Bit Timing Configuration Registers
        // According to CAN with baud rate of 500 Khz; Oscillator with 32MHz; BRP = 0 to determine Tqs=16
        reg(CNF1, 0x01); // SJW=1*Tqs='00'; BRP=2='000001' for 1MHz of baude set BRP = 0
        reg(CNF2, 0xB1); // BTLMODE= Length of PS2=Defined in CNF3='1'; SAM='0'; PHSEG1=PS1 Lenght='000'; PRSEG=Propagation='001'
        reg(CNF3, 0x05); // SOF='0'; WAKFIL='0'; PHSEG2<2:0>='101'

        enable();
    };
    ~CAN_Engine() { disable(); }

    void enable() { _enable.set(0); }
    void disable() { _enable.set(1); }

    void int_enable(bool error = true, bool wake_up = true, bool tx_buffer0 = true, bool tx_buffer1 = true, bool tx_buffer2 = true, bool rx_buffer0 = true,bool rx_buffer1 = true) { }

    void int_disable(bool error = true, bool wake_up = true, bool tx_buffer0 = true, bool tx_buffer1 = true, bool tx_buffer2 = true, bool rx_buffer0 = true,bool rx_buffer1 = true) { }

    void filter(const Filter & f) {
        switch(f) {
        case LISTEN_ONLY: {
            reg(RXB0CTRL, 0x60);
            reg(RXB1CTRL, 0x60);
            reg(CTRL, 0x64);
        } break;
        case NORMAL: {
            reg(RXM0SIDH, 0x00);
            reg(RXM0SIDL, 0x00);

            reg(RXB0CTRL, 0x20);

            reg(RXM1SIDH, 0x00);
            reg(RXM1SIDL, 0x00);

            reg(RXB1CTRL, 0x20);
        } break;
        default:
            break;
        }
    }

    void reset() {
        _cscan.set(0);
        spi_send(RESET);
        while(_spi.is_busy());
        _cscan.set(1);
    }

    void get(char * data, unsigned int data_size, unsigned int rx_buffer = 0) {
        char address;
        if(rx_buffer == 0)
            address = RXB0D0;
        else if(rx_buffer == 1)
            address = RXB1D0;
        else
            return;

        _cscan.set(0);
        spi_send(READ);
        spi_send(address);
        while(_spi.is_busy());
        for(unsigned int i = 0; i < data_size; i++)
            data[i] = spi_receive();
        _cscan.set(1);
    }

    char get(unsigned int rx_buffer = 0) {
        char ret;
        get(&ret, 1, rx_buffer);
        return ret;
    }

    char get_id(unsigned int rx_buffer = 0) {
        char address;
        if(rx_buffer == 0)
            address = RXB0SIDH;
        else if(rx_buffer == 1)
            address = RXB1SIDH;
        else
            return 0;

        char ret;
        _cscan.set(0);
        spi_send(READ);
        spi_send(address);
        while(_spi.is_busy());
        ret = spi_receive() << 3;
        _cscan.set(1);

        if(rx_buffer == 0)
            address = RXB0SIDL;
        else if(rx_buffer == 1)
            address = RXB1SIDL;
        _cscan.set(0);
        spi_send(READ);
        spi_send(address);
        while(_spi.is_busy());
        ret |= spi_receive() >> 5;
        _cscan.set(1);
    }

    void put_id(char id, unsigned int tx_buffer = 0) {
        char instruction;
        if(tx_buffer == 0)
            instruction = LOAD_TXB0SIDH;
        else if(tx_buffer == 1)
            instruction = LOAD_TXB1SIDH;
        else if(tx_buffer == 2)
            instruction = LOAD_TXB2SIDH;
        else
            return;

        char ret;
        _cscan.set(0);
        spi_send(instruction);
        spi_send(id);
        while(_spi.is_busy());
        _cscan.set(1);
    }

    void put(char * data, unsigned int data_size, unsigned int tx_buffer = 0) {
        char instruction;
        if(tx_buffer == 0)
            instruction = LOAD_TXB0D0;
        else if(tx_buffer == 1)
            instruction = LOAD_TXB1D0;
        else if(tx_buffer == 2)
            instruction = LOAD_TXB2D0;
        else
            return;

        _cscan.set(0);
        spi_send(instruction);
        for(unsigned int i = 0; (i < 8) && (i < data_size); i++)
            spi_send(data[i]);
        while(_spi.is_busy());
        _cscan.set(1);
    }

    // Request To Send
    void rts(bool tx_buffer_0 = true, bool tx_buffer_1 = false, bool tx_buffer_2 = false) {
        if(!(tx_buffer_0 || tx_buffer_1 || tx_buffer_2))
            return;

        char command = RTS | tx_buffer_0 | (tx_buffer_1 << 1) | (tx_buffer_2 << 2);

        _cscan.set(0);
        spi_send(RTS);
        while(_spi.is_busy());
        _cscan.set(1);
    }

    // Returns status of all RX and TX buffers in a single byte
    char status() {
        _cscan.set(0);
        spi_send(READ_STATUS);
        while(_spi.is_busy());
        char ret = spi_receive();
        _cscan.set(1);
        return ret;
    }

    // Bitwise modification of the desired register (only some registers support this option)
    void bit_modify(const Register & address, char mask, char data) {
        _cscan.set(0);
        spi_send(BIT_MODIFY);
        spi_send(address);
        spi_send(mask);
        spi_send(data);
        while(_spi.is_busy());
        _cscan.set(1);
    }

    bool ready_to_get() {
        _cscan.set(0);
        spi_send(RX_STATUS);
        while(_spi.is_busy());
        char ret = spi_receive();
        _cscan.set(1);
        return ret & 0x60;;
    }

    bool ready_to_put() { return true; }

private:
    void reg(const Register & r, char data) {
        _cscan.set(0);
        spi_send(WRITE);
        spi_send(r);
        spi_send(data);
        while(_spi.is_busy());
        _cscan.set(1);
    }

    void spi_send(char data) { _spi.put_data(data); }

    char spi_receive() {
        _spi.get_datamod();
        spi_send(0x00);
        char data = _spi.get_data();
        while(_spi.is_busy());
        return data;
    }

private:
    SPI  _spi;
    GPIO _enable;
    GPIO _interrupt;
    GPIO _cscan;
    GPIO _clock;
};

class CAN: public CAN_Engine
{
    typedef CAN_Engine Engine;
public:
    CAN() {}
    ~CAN() {}

    void enable() { Engine::enable(); }
    void disable() { Engine::disable(); }
    void reset() { Engine::reset(); }

    bool ready_to_get() { return Engine::ready_to_get(); }
    bool ready_to_put() { return Engine::ready_to_put(); }

    void filter(const Filter & f) { Engine::filter(f); }

    void put(char * data, unsigned int data_size, unsigned int tx_buffer = 0) { Engine::put(data, data_size, tx_buffer); }
    void put_id(unsigned int id, unsigned int tx_buffer = 0) { Engine::put_id(id, tx_buffer); }

    void get(char * data, unsigned int data_size, unsigned int rx_buffer = 0) { Engine::get(data, data_size, rx_buffer); }
    char get(unsigned int rx_buffer = 0) { return Engine::get(rx_buffer); }
    char get_id(unsigned int rx_buffer = 0) { return Engine::get_id(rx_buffer); }

    void int_enable(bool error = true, bool wake_up = true, bool tx_buffer0 = true, bool tx_buffer1 = true, bool tx_buffer2 = true, bool rx_buffer0 = true, bool rx_buffer1 = true) {
        Engine::int_enable(error, wake_up, tx_buffer0, tx_buffer1, tx_buffer2, rx_buffer0, rx_buffer1);
    }
    void int_disable(bool error = true, bool wake_up = true, bool tx_buffer0 = true, bool tx_buffer1 = true, bool tx_buffer2 = true, bool rx_buffer0 = true,bool rx_buffer1 = true) {
        Engine::int_disable(error, wake_up, tx_buffer0, tx_buffer1, tx_buffer2, rx_buffer0, rx_buffer1);
    }
};

__END_SYS

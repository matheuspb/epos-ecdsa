// EPOS Trustful SpaceTime Protocol MAC Declarations

#ifndef __tstp_mac_h
#define __tstp_mac_h

// Include only TSTP_Common from tstp.h
#ifdef __tstp_h
#include <tstp.h>
#else
#define __tstp_h
#include <tstp.h>
#undef __tstp_h
#endif

#include <ic.h>
#include <utility/random.h>
#include <utility/math.h>
#include <watchdog.h>

__BEGIN_SYS

template<typename Radio>
class TSTP_MAC: public TSTP_Common, public Radio
{
    typedef IEEE802_15_4 Phy_Layer;

    enum State {
        UPDATE_TX_SCHEDULE = 0,
        SLEEP_S            = 1,
        RX_MF              = 2,
        SLEEP_DATA         = 3,
        RX_DATA            = 4,
        CONTEND            = 5,
        CCA                = 6,
        TX_MF              = 7,
        TX_DATA            = 8,
    };

public:
    using TSTP_Common::Address;
    using TSTP_Common::Header;
    using TSTP_Common::Frame;
    typedef typename Radio::Timer Timer;
    typedef typename Radio::Timer::Time_Stamp Time_Stamp;

    static const unsigned int MTU = Frame::MTU;

    static unsigned int period() { return PERIOD; }

private:
public: //TODO: for debugging

    static const bool sniffer = Traits<NIC>::promiscuous;
    static const bool state_machine_debugged = false;
    static const bool random_backoff = true;
    static const bool silence = true;
    static const bool multichannel = false; // TODO
    static const unsigned int MICROFRAME_CHANNEL = 26; // TODO

    static const unsigned int INT_HANDLING_DELAY = 9; // Time delay between scheduled tx_mf interrupt and actual Radio TX
    static const unsigned int TX_DELAY = INT_HANDLING_DELAY + Radio::RX_TO_TX_DELAY;

    static const unsigned int Tu = IEEE802_15_4::TURNAROUND_TIME;
    static const unsigned int Ti = Tu + Radio::RX_TO_TX_DELAY + INT_HANDLING_DELAY + 100; // 100us margin for delay between Microframes // FIXME
    static const unsigned int TIME_BETWEEN_MICROFRAMES = Ti;
    static const unsigned int Ts = (sizeof(Microframe) + Phy_Layer::PHY_HEADER_SIZE) * 1000000ull
                                    / Phy_Layer::BYTE_RATE
                                    + Radio::TX_TO_RX_DELAY; // Time to send a single Microframe (including PHY headers)
    static const unsigned int MICROFRAME_TIME = Ts;
    static const unsigned int Tr = 2*Ts + Ti + (2*Ts + Ti) / 10;
    static const unsigned int RX_MF_TIMEOUT = Tr;

    static const unsigned int NMF = 1 + (((1000000ull * Tr) / Traits<System>::DUTY_CYCLE) + (Ti + Ts) - 1) / (Ti + Ts);
    static const unsigned int N_MICROFRAMES = NMF;

    static const unsigned int CI = Ts + (NMF - 1) * (Ts + Ti);
    static const unsigned int PERIOD = CI;
    static const unsigned int SLEEP_PERIOD = CI - RX_MF_TIMEOUT;

    static const typename IF<(Tr * 1000000ull / CI <= Traits<System>::DUTY_CYCLE), unsigned int, void>::Result
        DUTY_CYCLE = Tr * 1000000ull / CI; // in ppm. This line failing means that TSTP_MAC is unable to provide a duty cycle smaller than or equal to Traits<System>::DUTY_CYCLE

    // TODO
    static const unsigned int DATA_LISTEN_MARGIN = (TIME_BETWEEN_MICROFRAMES + MICROFRAME_TIME) * 2; // Subtract this amount when calculating time until data transmission
    static const unsigned int DATA_SKIP_TIME = (Phy_Layer::MTU + Phy_Layer::PHY_HEADER_SIZE) * 1000000ull / Phy_Layer::BYTE_RATE;

    static const unsigned int RX_DATA_TIMEOUT = DATA_SKIP_TIME + DATA_LISTEN_MARGIN;

    static const unsigned int G = IEEE802_15_4::CCA_TX_GAP;
    static const unsigned int CCA_TIME = Tr;

    static const unsigned int OFFSET_LOWER_BOUND = G + Radio::SLEEP_TO_RX_DELAY;
    static const unsigned int OFFSET_GENERAL_LOWER_BOUND = OFFSET_LOWER_BOUND + 3 * G;
    static const unsigned int OFFSET_UPPER_BOUND = SLEEP_PERIOD - CCA_TIME - Radio::RX_TO_TX_DELAY - MICROFRAME_TIME;
    static const unsigned int OFFSET_GENERAL_UPPER_BOUND = OFFSET_UPPER_BOUND - 3 * G;


protected:
    TSTP_MAC(unsigned int unit) : _unit(unit) {
        db<TSTP_MAC<Radio>, Init>(TRC) << "TSTP_MAC(u=" << unit << ")" << endl;
    }

    ~TSTP_MAC() {
        for(Buffer::Element * el = _tx_schedule.head(); el; el = _tx_schedule.head()) {
            Buffer * b = el->object();
            _tx_schedule.remove(el);
            if(b)
                free(b);
        }
    }

    // Called after the Radio's constructor
    void constructor_epilogue() {
        db<TSTP_MAC<Radio>, Init>(TRC) << "TSTP_MAC::constructor_epilogue()" << endl;
        if(sniffer) {
            Radio::power(Power_Mode::FULL);
            Radio::listen();
        } else {
            Watchdog::enable();
            update_tx_schedule(0);
        }
    }

    // Filter and assemble RX Buffer Metainformation
    bool pre_notify(Buffer * buf) {
        CPU::int_disable();
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "pre_notify(buf=" << buf << ")" << endl;

        if(sniffer) {
            static unsigned int last_id = 0;
            static unsigned int last_hint = 0;
            if(buf->size() == sizeof(Microframe)) {
                Microframe * mf = buf->frame()->data<Microframe>();
                //if((mf->id() != last_id) || (mf->hint() != last_hint)) {
                    last_id = mf->id();
                    last_hint = mf->hint();

                    buf->id = last_id;
                    buf->downlink = mf->all_listen();
                    buf->is_new = false;
                    buf->is_microframe = true;
                    buf->microframe_count = mf->count();
                    buf->offset = OFFSET_GENERAL_UPPER_BOUND;

                    // Forge a TSTP identifier to make the radio notify listeners
                    mf->all_listen(false);
                    mf->count(TSTP_Common::V0 >> 1);

                    CPU::int_enable();
                    return true;
                //}
                CPU::int_enable();
                return false;
            } else {
                last_id = 0;
                last_hint = 0;
                buf->is_microframe = false;
                CPU::int_enable();
                return true;
            }
            CPU::int_enable();
            return false;
        }

        if(_in_rx_mf) { // State: RX MF (part 2/3)
                                                    // TODO: I don't know why, but some MFs with a huge count are getting here
                                                    // TODO: check if this is still true
            if((buf->size() == sizeof(Microframe)) && (buf->frame()->data<Microframe>()->count() < N_MICROFRAMES)) {

                Timer::int_disable();

                Radio::power(Power_Mode::SLEEP);

                _in_rx_mf = false;

                Microframe * mf = buf->frame()->data<Microframe>();
                Frame_ID id = mf->id();

                // Initialize Buffer Metainformation
                buf->id = id;
                buf->downlink = mf->all_listen();
                buf->is_new = false;
                buf->is_microframe = true;
                buf->relevant = mf->all_listen();
                buf->trusted = false;
                buf->hint = mf->hint();
                buf->microframe_count = mf->count();
                buf->offset = OFFSET_GENERAL_UPPER_BOUND;
                buf->times_txed = 0;

                // Forge a TSTP identifier to make the radio notify listeners
                mf->all_listen(false);
                mf->count(TSTP_Common::V0 >> 1);

                CPU::int_enable();
                return true;
            }
            CPU::int_enable();
            return false;
        } else if(_in_rx_data) { // State: RX Data (part 2/3)

            if(buf->size() == sizeof(Microframe)) {
                CPU::int_enable();
                return false;
            }

            Radio::power(Power_Mode::SLEEP);

            db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::pre_notify: Frame received: " << buf->frame() << " at " << Radio::Timer::count2us(buf->sfd_time_stamp) << endl;

            // Initialize Buffer Metainformation
            buf->id = TSTP_Common::id(buf->frame()->data<TSTP_Common::Frame>());
            buf->hint = _receiving_data_hint;
            buf->is_new = false;
            buf->is_microframe = false;
            buf->trusted = false;
            buf->random_backoff_exponent = 0;
            buf->microframe_count = 0;
            buf->offset = OFFSET_GENERAL_UPPER_BOUND;
            buf->times_txed = 0;

            // Clear scheduled messages that are equivalent
            Buffer::Element * next;
            for(Buffer::Element * el = _tx_schedule.head(); el; el = next) {
                next = el->next();
                Buffer * queued_buf = el->object();
                if(equals(queued_buf, buf)) {
                    if(!queued_buf->destined_to_me) {
                        db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::pre_notify: ACK received, ID=" << queued_buf->id << endl;
                        _tx_schedule.remove(el);
                        delete queued_buf;
                    }
                }
            }

            CPU::int_enable();
            return true;
        } else {
            CPU::int_enable();
            return false;
        }
    }

    bool post_notify(Buffer * buf) {
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "post_notify(buf=" << buf << ")" << endl;

        if(buf->is_microframe && !sniffer) { // State: RX MF (part 3/3)
            Time_Stamp data_time = buf->sfd_time_stamp + Timer::us2count(TIME_BETWEEN_MICROFRAMES) + buf->microframe_count * Timer::us2count(TIME_BETWEEN_MICROFRAMES + MICROFRAME_TIME);

            Watchdog::kick();
            if(state_machine_debugged)
                kout << SLEEP_DATA;

            // If we have a buffer with the same ID, we need to receive the data
            // to make sure we can clear it from the tx schedule
            if(!buf->relevant)
                for(Buffer::Element * el = _tx_schedule.head(); el; el = el->next())
                    if(el->object()->id == buf->id) {
                        buf->relevant = true;
                        break;
                    }

            if(buf->relevant) { // Transition: [Relevant MF]
                _receiving_data_hint = buf->hint;
                if(multichannel)
                    _receiving_data_channel = channel(buf);
                // State: Sleep until Data
                if(data_time > Timer::read() + Timer::us2count(DATA_LISTEN_MARGIN / 2 + Radio::SLEEP_TO_RX_DELAY))
                    data_time -= Timer::us2count(DATA_LISTEN_MARGIN / 2);
                else
                    data_time = Timer::read() + Timer::us2count(Radio::SLEEP_TO_RX_DELAY + 1);
                Timer::interrupt(data_time, rx_data);
            } else { // Transition: [Irrelevant MF]
                Timer::interrupt(data_time + Timer::us2count(DATA_SKIP_TIME), update_tx_schedule);
            }
        }

        free(buf);
        return true;
    }

public:
    // Assemble TX Buffer Metainformation
    void marshal(Buffer * buf, const Address & src, const Address & dst, const Type & type) {
        buf->is_microframe = false;
        buf->trusted = false;
        buf->is_new = true;
        buf->random_backoff_exponent = 0;
        buf->microframe_count = 0;
        buf->times_txed = 0;
        buf->offset = OFFSET_GENERAL_UPPER_BOUND;
    }

    unsigned int unmarshal(Buffer * buf, Address * src, Address * dst, Type * type, void * data, unsigned int size) {
        *src = Address::BROADCAST;
        *dst = Address::BROADCAST;
        *type = buf->frame()->data<Header>()->version();
        memcpy(data, buf->frame()->data<Frame>(), (buf->size() < size ? buf->size() : size));
        return buf->size();
    }

    static bool drop(unsigned int id) {
        bool ret = false;
        bool int_disabled = CPU::int_disabled();
        if(!int_disabled)
            CPU::int_disable();
        for(Buffer::Element * el = _tx_schedule.head(); el; el = el->next()) {
            Buffer * buf = el->object();
            if(buf->id == id) {
                _tx_schedule.remove(el);
                if(_tx_pending == buf)
                    _tx_pending = 0;
                ret = true;
                delete buf;
                break;
            }
        }
        if(!int_disabled)
            CPU::int_enable();
        return ret;
    }

    int send(Buffer * buf) {
        if(sniffer) {
            delete buf;
            return 0;
        }

        buf->offset = Timer::us2count(buf->offset);

        if(buf->destined_to_me)
            buf->offset = Timer::us2count(OFFSET_LOWER_BOUND);
        else {
            if(buf->offset < Timer::us2count(OFFSET_GENERAL_LOWER_BOUND))
                buf->offset = Timer::us2count(OFFSET_GENERAL_LOWER_BOUND);
            else if(buf->offset > Timer::us2count(OFFSET_GENERAL_UPPER_BOUND))
                buf->offset = Timer::us2count(OFFSET_GENERAL_UPPER_BOUND);
        }

        // Check if we already have this message queued. If so, replace it
        CPU::int_disable();
        Buffer::Element * next;
        for(Buffer::Element * el = _tx_schedule.head(); el; el = next) {
            next = el->next();
            Buffer * queued_buf = el->object();
            if(equals(queued_buf, buf)) {
                if(_tx_pending && (_tx_pending == queued_buf))
                    _tx_pending = buf;
                _tx_schedule.remove(queued_buf->link());
                delete queued_buf;
            }
        }
        _tx_schedule.insert(buf->link());
        CPU::int_enable();

        return buf->size();
    }

private:

    // State Machine

    static void update_tx_schedule(const IC::Interrupt_Id & id) {
        // State: Update TX Schedule
        CPU::int_disable();
        Timer::int_disable();
        if(state_machine_debugged)
            kout << UPDATE_TX_SCHEDULE;
        Watchdog::kick();
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::update_tx_schedule(id=" << id << ")" << endl;

        Radio::power(Power_Mode::SLEEP);
        _in_rx_data = false;
        _in_rx_mf = false;

        _tx_pending = 0;

        Time_Stamp now_ts = Timer::read();
        Microsecond now_us = Timer::count2us(now_ts);

        if(_silence_periods && silence)
            _silence_periods--;
        else {
            // Fetch next message and remove expired ones
            // TODO: Turn _tx_schedule into an ordered list
            for(Buffer::Element * el = _tx_schedule.head(); el; ) {
                Buffer::Element * next = el->next();
                Buffer * b = el->object();
                /*// Message was created in the future. This might happen when Timekeeper adjusts the timer.
                if(b->frame()->data<Header>()->time() > now_us) {
                    _tx_schedule.remove(el);
                    delete b;
                } else */
                // Drop expired messages
                if(drop_expired && (b->deadline <= now_us)) {
                    _tx_schedule.remove(el);
                    delete b;
                } else if(!_tx_pending) {
                    _tx_pending = b;
                // Prioritize ACKs
                } else if(_tx_pending->destined_to_me) {
                    if(b->destined_to_me) {
                        if(b->times_txed < _tx_pending->times_txed)
                            _tx_pending = b;
                        else if((b->times_txed == _tx_pending->times_txed)
                                && (b->deadline < _tx_pending->deadline))
                            _tx_pending = b;
                    }
                // Non-keepalives are prioritized over keepalives
                } else if((_tx_pending->frame()->data<Header>()->type() == CONTROL)
                        && (_tx_pending->frame()->data<Control>()->subtype() == KEEP_ALIVE)) {
                    if(((b->frame()->data<Header>()->type() == CONTROL)
                            && (b->frame()->data<Control>()->subtype() == KEEP_ALIVE))) {
                        if(b->times_txed < _tx_pending->times_txed)
                            _tx_pending = b;
                        else if((b->times_txed == _tx_pending->times_txed)
                                && (b->deadline < _tx_pending->deadline))
                            _tx_pending = b;
                    } else
                        _tx_pending = b;
                } else {
                    if(b->times_txed < _tx_pending->times_txed)
                        _tx_pending = b;
                    else if((b->times_txed == _tx_pending->times_txed)
                            && (b->deadline < _tx_pending->deadline))
                        _tx_pending = b;
                }
                el = next;
            }
        }

        if(_tx_pending) { // Transition: [TX pending]
            // State: Contend CCA (Contend part)

            Watchdog::kick();
            if(state_machine_debugged)
                kout << CONTEND;

            Time_Stamp offset = _tx_pending->offset;
            if(random_backoff) {
                // Increase ACK priority and decrease non-ACK priority by a random component,
                // based on number of transmission attempts.
                // This prevents permanent inteference by a given pair of nodes, and
                // makes unresponded messages have the lowest priorities
                _tx_pending->random_backoff_exponent++;
                unsigned int lim = G * _tx_pending->random_backoff_exponent;
                if((lim > Timer::us2count(OFFSET_UPPER_BOUND)) || (lim == 0))
                    lim = Timer::us2count(OFFSET_UPPER_BOUND);
                if(_tx_pending->destined_to_me) {
                    offset -= ((unsigned int) (Random::random()) % lim);
                    if((offset < Timer::us2count(OFFSET_LOWER_BOUND)) || (offset > Timer::us2count(OFFSET_GENERAL_LOWER_BOUND))) {
                        offset = Timer::us2count(OFFSET_LOWER_BOUND);
                        _tx_pending->random_backoff_exponent--;
                    }
                } else {
                    offset += ((unsigned int) (Random::random()) % lim);

                    if(offset < Timer::us2count(OFFSET_GENERAL_LOWER_BOUND)) {
                        offset = Timer::us2count(OFFSET_GENERAL_LOWER_BOUND);
                        _tx_pending->random_backoff_exponent--;
                    } else if(offset > Timer::us2count(OFFSET_UPPER_BOUND)) {
                        offset = Timer::us2count(OFFSET_UPPER_BOUND);
                        _tx_pending->random_backoff_exponent--;
                    }
                }
            }

            bool is_model = (_tx_pending->frame()->data<Header>()->type() == CONTROL)
                            && (_tx_pending->frame()->data<Control>()->subtype() == MODEL);

            new (&_mf) Microframe(((!_tx_pending->destined_to_me) && _tx_pending->downlink) || (is_model),
                    _tx_pending->id, N_MICROFRAMES - 1, _tx_pending->hint);

            Radio::copy_to_nic(&_mf, sizeof(Microframe));

            // TODO: should we add the radio DELAYs ?
            Timer::interrupt(now_ts + offset - Timer::us2count(Radio::SLEEP_TO_RX_DELAY), cca);
            //Timer::interrupt(now_ts + offset, cca);
        } else { // Transition: [No TX pending]
            // State: Sleep S
            if(state_machine_debugged)
                kout << SLEEP_S ;
            Watchdog::kick();
            Timer::interrupt(now_ts + Timer::us2count(SLEEP_PERIOD - Radio::SLEEP_TO_RX_DELAY), rx_mf);
        }
        CPU::int_enable();
    }

    // State: Contend CCA (CCA part)
    static void cca(const IC::Interrupt_Id & id) {
        if(state_machine_debugged)
            kout << CCA ;
        Watchdog::kick();
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::cca(id=" << id << ")" << endl;

        assert(N_MICROFRAMES > 1);

        if(multichannel)
            Radio::channel(MICROFRAME_CHANNEL);
        Radio::power(LIGHT);
        Radio::listen();

        // Try to send the first Microframe
        if(Radio::cca(CCA_TIME)) {
            _mf_time = Timer::read() + Timer::us2count(Radio::RX_TO_TX_DELAY);
            if(Radio::transmit()) { // Transition: [Channel free]
                _mf_time += Timer::us2count(TIME_BETWEEN_MICROFRAMES + MICROFRAME_TIME);
                _mf.dec_count();
                if(state_machine_debugged)
                    kout << TX_MF ;
                Watchdog::kick();
                while(!Radio::tx_done());
                Radio::copy_to_nic(&_mf, sizeof(Microframe));
                //tx_mf(0);
                Timer::interrupt(_mf_time - Timer::us2count(TX_DELAY), tx_mf);
            } else { // Transition: [Channel busy]
                rx_mf(0);
            }
        } else { // Transition: [Channel busy]
            rx_mf(0);
        }
    }

    // State: RX MF (part 1/3)
    static void rx_mf(const IC::Interrupt_Id & id) {
        if(state_machine_debugged)
            kout << RX_MF ;
        Watchdog::kick();
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::rx_mf(id=" << id << ")" << endl;

        _in_rx_data = false;
        _in_rx_mf = true;

        // If timeout is reached, Transition: [No MF]
        // TODO: radio DELAYs
        Timer::interrupt(Timer::read() + Timer::us2count(RX_MF_TIMEOUT), update_tx_schedule);

        //if(multichannel && radio_sleeping)
        //    Radio::channel(MICROFRAME_CHANNEL);
        Radio::power(Power_Mode::FULL);
        Radio::listen();
    }

    // State: RX Data (part 1/3)
    static void rx_data(const IC::Interrupt_Id & id) {
        if(state_machine_debugged)
            kout << RX_DATA ;
        Watchdog::kick();
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::rx_data(id=" << id << ")" << endl;

        _in_rx_data = true;
        _in_rx_mf = false;

        // Set timeout
        Timer::interrupt(Timer::read() + Timer::us2count(RX_DATA_TIMEOUT), update_tx_schedule);

        if(multichannel)
            Radio::channel(_receiving_data_channel);
        Radio::power(Power_Mode::FULL);
        Radio::listen();
    }

    // State: TX MFs
    static void tx_mf(const IC::Interrupt_Id & id) {
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::tx_mf()" << endl;

        // The first Microframe is sent at cca()
        Radio::transmit_no_cca();

        _mf_time += Timer::us2count(TIME_BETWEEN_MICROFRAMES + MICROFRAME_TIME);

        Watchdog::kick();

        if(_mf.dec_count() > 0) {
            while(!Radio::tx_done());
            Radio::copy_to_nic(&_mf, sizeof(Microframe));
            Timer::interrupt(_mf_time - Timer::us2count(TX_DELAY), tx_mf);
        } else {
            CPU::int_disable();
            // The precise time when this frame is actually sent by the physical layer
            _tx_pending->frame()->data<Header>()->last_hop_time(_mf_time + Timer::us2count(TX_DELAY + Tu));
            while(!Radio::tx_done());
            Radio::copy_to_nic(_tx_pending->frame(), _tx_pending->size());
            Timer::interrupt(_mf_time - Timer::us2count(TX_DELAY), tx_data);
            if(multichannel)
                Radio::channel(channel(_tx_pending));
            CPU::int_enable();
        }
    }

    static void tx_data(const IC::Interrupt_Id & id) {
        if(state_machine_debugged)
            kout << TX_DATA;
        if(Traits<TSTP_MAC>::hysterically_debugged)
            db<TSTP_MAC<Radio>>(TRC) << "TSTP_MAC::tx_data()" << endl;

        // State: TX Data
        CPU::int_disable();
        if(_tx_pending) { // _tx_pending might have been deleted at drop()
            Radio::transmit_no_cca();
            Watchdog::kick();

            bool is_keep_alive = (_tx_pending->frame()->data<Header>()->type() == CONTROL)
                && (_tx_pending->frame()->data<Control>()->subtype() == KEEP_ALIVE);

            _tx_pending->times_txed++;

            if(silence && !is_keep_alive && !_tx_pending->destined_to_me) {
                unsigned int r = Random::random();
                _silence_periods += (r % _tx_pending->times_txed) + 1;
            }

            while(!Radio::tx_done());

            _mf_time = Timer::read();

            // Keep Alive messages are never ACK'ed or forwarded
            if(is_keep_alive || _tx_pending->destined_to_me) {
                _tx_schedule.remove(_tx_pending->link());
                delete _tx_pending;
                _tx_pending = 0;
            }
        } else {
            _mf_time = Timer::read();
        }
        CPU::int_enable();

        // State: Sleep S
        Radio::power(Power_Mode::SLEEP);
        if(state_machine_debugged)
            kout << SLEEP_S;
        if(multichannel)
            Radio::channel(MICROFRAME_CHANNEL);
        // TODO: Radio DELAYs
        Timer::interrupt(_mf_time + Timer::us2count(SLEEP_PERIOD), rx_mf);
    }

    static unsigned int channel(Buffer * b) {
        return (b->id % 14) + 11;
    }

    void free(Buffer * b);
    bool equals(Buffer *, Buffer *);

    static Microframe _mf;
    static Time_Stamp _mf_time;
    static Hint _receiving_data_hint;
    static Buffer::List _tx_schedule;
    static Buffer * _tx_pending;
    static bool _in_rx_mf;
    static bool _in_rx_data;
    static unsigned int _receiving_data_channel;
    static unsigned int _silence_periods;

    unsigned int _unit;
};

// The compiler makes sure that template static variables are only defined once

// Class attributes
template<typename Radio>
TSTP_Common::Microframe TSTP_MAC<Radio>::_mf;

template<typename Radio>
typename TSTP_MAC<Radio>::Time_Stamp TSTP_MAC<Radio>::_mf_time;

template<typename Radio>
typename TSTP_MAC<Radio>::Hint TSTP_MAC<Radio>::_receiving_data_hint;

template<typename Radio>
TSTP_MAC<Radio>::Buffer::List TSTP_MAC<Radio>::_tx_schedule;

template<typename Radio>
typename TSTP_MAC<Radio>::Buffer * TSTP_MAC<Radio>::_tx_pending;

template<typename Radio>
bool TSTP_MAC<Radio>::_in_rx_mf;

template<typename Radio>
bool TSTP_MAC<Radio>::_in_rx_data;

template<typename Radio>
unsigned int TSTP_MAC<Radio>::_receiving_data_channel;

template<typename Radio>
unsigned int TSTP_MAC<Radio>::_silence_periods;



template<typename Radio>
class TSTP_MAC_NOMF: public TSTP_Common, public Radio
{
    typedef IEEE802_15_4 Phy_Layer;

public:
    enum State {
        UPDATE_TX_SCHEDULE = 0,
        RX                 = 1,
        CCA_TX             = 2,
    };

    using TSTP_Common::Address;
    using TSTP_Common::Header;
    using TSTP_Common::Frame;
    typedef typename Radio::Timer Timer;
    typedef typename Radio::Timer::Time_Stamp Time_Stamp;

    static const unsigned int MTU = Frame::MTU;

private:
    static const bool sniffer = Traits<NIC>::promiscuous;
    static const bool state_machine_debugged = false;
    static const bool random_backoff = true;
    static const unsigned int PERIOD = 150000; // TODO
    static const unsigned int OFFSET_LOWER_BOUND = 2 * IEEE802_15_4::CCA_TX_GAP;
    static const unsigned int OFFSET_UPPER_BOUND = PERIOD;
    static const unsigned int OFFSET_GENERAL_UPPER_BOUND = OFFSET_UPPER_BOUND - 2 * IEEE802_15_4::CCA_TX_GAP;
    static const unsigned int OFFSET_GENERAL_LOWER_BOUND = 2 * OFFSET_LOWER_BOUND;
    static const unsigned int CCA_TIME = IEEE802_15_4::CCA_TX_GAP;
    static const unsigned int RX_TO_TX_DELAY = IEEE802_15_4::TURNAROUND_TIME + Radio::RX_TO_TX_DELAY;

protected:
    TSTP_MAC_NOMF(unsigned int unit) : _unit(unit) {
        db<TSTP_MAC_NOMF<Radio>, Init>(TRC) << "TSTP_MAC_NOMF(u=" << unit << ")" << endl;
    }

    // Called after the Radio's constructor
    void constructor_epilogue() {
        db<TSTP_MAC_NOMF<Radio>, Init>(TRC) << "TSTP_MAC_NOMF::constructor_epilogue()" << endl;
        Radio::power(Power_Mode::FULL);
        Radio::listen(); // Radio is assumed to always return to RX after TX
        if(!sniffer)
            update_tx_schedule(0);
        rx();
    }

public:
    static unsigned int period() { return PERIOD; }

    // Assemble TX Buffer Metainformation
    void marshal(Buffer * buf, const Address & src, const Address & dst, const Type & type) {
        buf->id = 0;
        buf->is_microframe = false;
        buf->trusted = false;
        buf->is_new = true;
        buf->random_backoff_exponent = 0;
        buf->microframe_count = 0;
        buf->offset = OFFSET_GENERAL_UPPER_BOUND;
    }

    unsigned int unmarshal(Buffer * buf, Address * src, Address * dst, Type * type, void * data, unsigned int size) {
        *src = Address::BROADCAST;
        *dst = Address::BROADCAST;
        *type = buf->frame()->data<Header>()->version();
        memcpy(data, buf->frame()->data<Frame>(), (buf->size() < size ? buf->size() : size));
        return buf->size();
    }

    static bool drop(unsigned int id) {
        bool ret = false;
        bool int_disabled = CPU::int_disabled();
        if(!int_disabled)
            CPU::int_disable();
        for(Buffer::Element * el = _tx_schedule.head(); el; el = el->next()) {
            Buffer * buf = el->object();
            if(buf->id == id) {
                _tx_schedule.remove(el);
                if(_tx_pending == buf)
                    _tx_pending = 0;
                ret = true;
                delete buf;
                break;
            }
        }
        if(!int_disabled)
            CPU::int_enable();
        return ret;
    }

    int send(Buffer * buf) {
        if(sniffer) {
            delete buf;
            return 0;
        }

        db<TSTP_MAC_NOMF<Radio>>(TRC) << "TSTP_MAC_NOMF::send(b=" << buf << ")" << endl;
        if(buf->destined_to_me)
            buf->offset = Timer::us2count(OFFSET_GENERAL_LOWER_BOUND);
        else {
            buf->offset = Timer::us2count(buf->offset);

            if(buf->offset < Timer::us2count(OFFSET_GENERAL_LOWER_BOUND))
                buf->offset = Timer::us2count(OFFSET_GENERAL_LOWER_BOUND);
            else if(buf->offset > Timer::us2count(OFFSET_GENERAL_UPPER_BOUND))
                buf->offset = Timer::us2count(OFFSET_GENERAL_UPPER_BOUND);
        }

        CPU::int_disable();

        // Check if we have a message with the same ID. If so, replace it
        Buffer::Element * next;
        for(Buffer::Element * el = _tx_schedule.head(); el; el = next) {
            next = el->next();
            Buffer * queued_buf = el->object();
            if(equals(queued_buf, buf)) {
                if(_tx_pending && (_tx_pending == queued_buf))
                    _tx_pending = buf;
                _tx_schedule.remove(queued_buf->link());
                delete queued_buf;
                db<TSTP_MAC_NOMF<Radio>>(TRC) << "TSTP_MAC_NOMF::send(b=" << buf << ") => deleted buffer with equivalent message" << endl;
            }
        }

        _tx_schedule.insert(buf->link());

        if(!_notifying)
            update_tx_schedule(0); // implicit int_enable
        // else, int_enable at post_notify

        return buf->size();
    }

protected:
    bool pre_notify(Buffer * buf) {
        if(Traits<TSTP_MAC_NOMF>::hysterically_debugged)
            db<TSTP_MAC_NOMF<Radio>>(TRC) << "TSTP_MAC_NOMF::pre_notify(b=" << buf << endl;

        CPU::int_disable();
        _notifying = true;

        _stats.rx_packets++;

        // Initialize Buffer Metainformation
        buf->is_new = false;
        buf->is_microframe = false;
        buf->trusted = false;
        buf->random_backoff_exponent = 0;
        buf->relevant = false;
        buf->offset = OFFSET_GENERAL_UPPER_BOUND;
        buf->id = TSTP_Common::id(buf->frame()->data<TSTP_Common::Frame>());

        // Clear scheduled messages with same ID
        Buffer::Element * next;
        for(Buffer::Element * el = _tx_schedule.head(); el; el = next) {
            next = el->next();
            Buffer * queued_buf = el->object();
            if(queued_buf->id == buf->id) {
                if(!queued_buf->destined_to_me) {
                    db<TSTP_MAC_NOMF<Radio>>(TRC) << "TSTP_MAC_NOMF::pre_notify: ACK received" << endl;
                    if(queued_buf == _tx_pending) {
                        Timer::int_disable();
                        _tx_pending = 0;
                    }
                    _tx_schedule.remove(el);
                    delete queued_buf;
                }
            }
        }

        // int_enable at post_notify
        return true;
    }

    bool post_notify(Buffer * buf) {
        free(buf);
        _notifying = false;
        if(sniffer)
            CPU::int_enable();
        else
            update_tx_schedule(0); // implicit int_enable
        return true;
    }

    bool equals(Buffer *, Buffer *);

private:
    // State Machine

    static void update_tx_schedule(const IC::Interrupt_Id & id) {
        if(state_machine_debugged)
            kout << UPDATE_TX_SCHEDULE;
        if(Traits<TSTP_MAC_NOMF>::hysterically_debugged)
            db<TSTP_MAC_NOMF<Radio>>(TRC) << "State: Update TX Schedule" << endl;

        CPU::int_disable();
        Buffer * tx_already_pending = _tx_pending;

        Time_Stamp now_ts = Timer::read();
        Microsecond now_us = Timer::count2us(now_ts);

        // Fetch next message and remove expired ones
        // TODO: Turn _tx_schedule into an ordered list
        for(Buffer::Element * el = _tx_schedule.head(); el; ) {
            Buffer::Element * next = el->next();
            Buffer * b = el->object();
            /*
            // Message was created in the future. This might happen when Timekeeper adjusts the timer.
            if(b->frame()->data<Header>()->time() > now_us) {
            if(b == _tx_pending) {
            Timer::int_disable();
            _tx_pending = 0;
            }
            _tx_schedule.remove(el);
            delete b;
            } else */
            if(drop_expired && (b->deadline <= now_us)) {
                if(b == _tx_pending) {
                    Timer::int_disable();
                    _tx_pending = 0;
                }
                _tx_schedule.remove(el);
                delete b;
            } else if(!_tx_pending) {
                _tx_pending = b;
            } else if(_tx_pending->destined_to_me) {
                if(b->destined_to_me) {
                    if(b->random_backoff_exponent < _tx_pending->random_backoff_exponent) {
                        _tx_pending = b;
                    } else if((b->random_backoff_exponent == _tx_pending->random_backoff_exponent) && (b->deadline < _tx_pending->deadline)) {
                        _tx_pending = b;
                    }
                }
            } else {
                if(b->random_backoff_exponent < _tx_pending->random_backoff_exponent) {
                    _tx_pending = b;
                } else if((b->random_backoff_exponent == _tx_pending->random_backoff_exponent) && (b->deadline < _tx_pending->deadline)) {
                    _tx_pending = b;
                }
            }
            el = next;
        }

        if(_tx_pending && (_tx_pending != tx_already_pending)) { // Transition: [TX pending]
            if(Traits<TSTP_MAC_NOMF>::hysterically_debugged)
                db<TSTP_MAC_NOMF<Radio>>(TRC) << "Transition: [TX pending]" << endl;

            Time_Stamp offset = _tx_pending->offset;
            if(random_backoff) {
                // Increase ACK priority and decrease non-ACK priority by a random component,
                // based on number of transmission attempts.
                // This prevents permanent inteference by a given pair of nodes, and
                // makes unresponded messages have the lowest priorities
                unsigned int lim = Timer::us2count(CCA_TX_GAP) << _tx_pending->random_backoff_exponent;
                if(!lim)
                    lim = Timer::us2count(OFFSET_UPPER_BOUND);
                if(_tx_pending->destined_to_me) {
                    offset -= ((unsigned int)(Random::random()) % lim);// * Timer::us2count(G);
                    if((offset < Timer::us2count(OFFSET_LOWER_BOUND)) || (offset > Timer::us2count(OFFSET_UPPER_BOUND))) {
                        offset = Timer::us2count(OFFSET_LOWER_BOUND);
                    }
                } else {
                    offset += ((unsigned int)(Random::random()) % lim);// * Timer::us2count(G);
                    if(offset < Timer::us2count(OFFSET_GENERAL_LOWER_BOUND)) {
                        offset = Timer::us2count(OFFSET_GENERAL_LOWER_BOUND);
                    } else if(offset > Timer::us2count(OFFSET_UPPER_BOUND)) {
                        offset = Timer::us2count(OFFSET_UPPER_BOUND);
                    }
                }
            }

            // The precise time when this frame is actually sent by the physical layer
            _tx_pending->frame()->data<Header>()->last_hop_time(now_ts + offset + Timer::us2count(Radio::RX_TO_TX_DELAY + CCA_TIME));
            Radio::copy_to_nic(_tx_pending->frame(), _tx_pending->size());

            Timer::interrupt(now_ts + offset, cca_tx);
        }
        CPU::int_enable();
        rx();
    }

    static void rx() {
        if(state_machine_debugged)
            kout << RX;
        // Radio is assumed to always return to RX after TX
    }

    static void cca_tx(const IC::Interrupt_Id & id) {
        if(state_machine_debugged)
            kout << CCA_TX;

        Radio::power(Power_Mode::LIGHT);
        if(Radio::cca(CCA_TIME)) {
            CPU::int_disable();
            if(_tx_pending) { // _tx_pending might have been deleted by drop()
                Radio::transmit_no_cca();

                _stats.tx_packets++;
                _stats.tx_bytes += _tx_pending->size();
                if(!_tx_pending->is_new)
                    _stats.tx_relayed++;

                if(_tx_pending->destined_to_me || ((_tx_pending->frame()->data<Header>()->type() == CONTROL) && (_tx_pending->frame()->data<Control>()->subtype() == KEEP_ALIVE))) {
                    _tx_schedule.remove(_tx_pending->link());
                    delete _tx_pending;
                } else
                    _tx_pending->random_backoff_exponent++;
                _tx_pending = 0;
                CPU::int_enable();
                while(!Radio::tx_done());
            }
        } else {
            CPU::int_disable();
            if(_tx_pending) {
                _tx_pending->random_backoff_exponent++;
                _tx_pending = 0;
            }
            CPU::int_enable();
        }

        Timer::interrupt(Timer::read() + Timer::us2count(PERIOD), update_tx_schedule);
        Radio::power(Power_Mode::FULL);
        rx();
    }

private:
    void free(Buffer * b);

    static Buffer::List _tx_schedule;
    static Buffer * _tx_pending;
    static Statistics _stats;
    static bool _notifying;

    unsigned int _unit;
};

template<typename Radio>
TSTP_MAC_NOMF<Radio>::Buffer::List TSTP_MAC_NOMF<Radio>::_tx_schedule;

template<typename Radio>
typename TSTP_MAC_NOMF<Radio>::Buffer * TSTP_MAC_NOMF<Radio>::_tx_pending;

template<typename Radio>
typename TSTP_MAC_NOMF<Radio>::Statistics TSTP_MAC_NOMF<Radio>::_stats;

template<typename Radio>
bool TSTP_MAC_NOMF<Radio>::_notifying;

__END_SYS

#endif

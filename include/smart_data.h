// EPOS Smart Data Declarations

#ifndef __smart_data_h
#define __smart_data_h

#include <tstp.h>
#include <periodic_thread.h>
#include <utility/observer.h>
#include <utility/predictor.h>

__BEGIN_SYS

class Smart_Data_Common: public _UTIL::Observed
{
public:
    typedef _UTIL::Observed Observed;
    typedef _UTIL::Observer Observer;

    struct DB_Series {
        unsigned char version;
        unsigned long unit;
        long x;
        long y;
        long z;
        unsigned long r;
        unsigned long long t0;
        unsigned long long t1;
        unsigned long dev;
        friend OStream & operator<<(OStream & os, const DB_Series & d) {
            os << "{ve=" << d.version << ",u=" << d.unit << ",dst=(" << d.x << "," << d.y << "," << d.z << ")+" << d.r << ",t=[" << d.t0 << "," << d.t1 << "]}";
            return os;
        }
    }__attribute__((packed));

    struct SI_Record {
        unsigned char version;
        unsigned long unit;
        double value;
        unsigned char error;
        unsigned char confidence;
        long x;
        long y;
        long z;
        unsigned long long t;
        unsigned long dev;
        friend OStream & operator<<(OStream & os, const SI_Record & d) {
            os << "{ve=" << d.version << ",u=" << d.unit << ",va=" << d.value << ",e=" << d.error << ",src=(" << d.x << "," << d.y << "," << d.z << "),t=" << d.t << ",d=" << d.dev << "}";
            return os;
        }
    }__attribute__((packed));

    template<unsigned int S>
    struct Digital_Record {
        unsigned char version;
        unsigned long unit;
        unsigned char value[S];
        unsigned char error;
        unsigned char confidence;
        long x;
        long y;
        long z;
        unsigned long long t;
        unsigned long dev;
        friend OStream & operator<<(OStream & os, const Digital_Record<S> & d) {
            os << "{ve=" << d.version << ",u=" << d.unit << ",va=[digital]" << ",e=" << d.error << ",src=(" << d.x << "," << d.y << "," << d.z << "),t=" << d.t << ",d=" << d.dev << "}";
            return os;
        }
    }__attribute__((packed));
};

template <typename T>
struct Smart_Data_Type_Wrapper
{
   typedef T Type;
};

// Smart Data encapsulates Transducers (i.e. sensors and actuators), local or remote, and bridges them with TSTP
// Transducers must be Observed objects, must implement either sense() or actuate(), and must define UNIT, NUM, and ERROR.
template<typename Transducer>
class Smart_Data: public Smart_Data_Common, private TSTP::Observer, private Transducer::Observer
{
    friend class Smart_Data_Type_Wrapper<Transducer>::Type; // friend S is OK in C++11, but this GCC does not implement it yet. Remove after GCC upgrade.
    friend class Smart_Data_Type_Wrapper<typename Transducer::Predictor>::Type;

private:
    typedef TSTP::Buffer Buffer;
    typedef typename TSTP::Responsive Responsive;
    typedef typename TSTP::Interested Interested;

    typedef typename TSTP::Predictive Predictive;
    typedef typename Transducer::Predictor Predictor;
    typedef typename Transducer::Predictor_Configuration Predictor_Configuration;

public:
    static const unsigned int UNIT = Transducer::UNIT;
    static const unsigned int NUM = Transducer::NUM;
    static const unsigned int ERROR = Transducer::ERROR;
    typedef typename TSTP::Unit::Get<NUM>::Type Value;
    typedef typename IF<(Transducer::UNIT >> 31) == 1, SI_Record, Digital_Record<Transducer::UNIT & 0xFFFF>>::Result DB_Record;

    enum {
        STATIC_VERSION = (1 << 4) + (1 << 0),
        MOBILE_VERSION = (1 << 4) + (2 << 0),
    };

    enum Mode {
        PRIVATE    = (0),
        ADVERTISED = (1 << 0),
        COMMANDED  = (1 << 1) | ADVERTISED,
        CUMULATIVE = (1 << 2),
        DISPLAYED  = (1 << 3),
        PREDICTIVE = (1 << 4)
    };

    static const unsigned int REMOTE = -1;

    typedef RTC::Microsecond Microsecond;

    typedef TSTP::Unit Unit;
    typedef TSTP::Error Error;
    typedef TSTP::Coordinates Coordinates;
    typedef TSTP::Region Region;
    typedef TSTP::Time Time;
    typedef TSTP::Time_Offset Time_Offset;

public:
    // Local data source, possibly advertised to or commanded by the network
    Smart_Data(unsigned int dev, const Microsecond & expiry, const unsigned char & mode = PRIVATE)
    : _unit(UNIT), _value(0), _error(ERROR), _coordinates(TSTP::here()), _time(TSTP::now()), _expiry(expiry), _device(dev), _mode(mode), _thread(0), _interested(0), _responsive(((mode & ADVERTISED) == ADVERTISED) ? new Responsive(this, UNIT, ERROR, expiry, ((mode & DISPLAYED) == DISPLAYED)) : 0), _predictor(0) {
        db<Smart_Data>(TRC) << "Smart_Data(dev=" << dev << ",exp=" << expiry << ",mode=" << mode << ")" << endl;
        if(Transducer::POLLING)
            Transducer::sense(_device, this);
        if(Transducer::INTERRUPT)
            Transducer::attach(this);
        if(_responsive)
            TSTP::attach(this, _responsive);
        db<Smart_Data>(INF) << "Smart_Data(dev=" << dev << ",exp=" << expiry << ",mode=" << mode << ") => " << *this << endl;
    }
    // Remote, event-driven (period = 0) or time-triggered data source
    Smart_Data(const Region & region, const Microsecond & expiry, const Microsecond & period = 0, const unsigned char & mode = PRIVATE)
    : _unit(UNIT), _value(0), _error(ERROR), _coordinates(0), _time(0), _expiry(expiry), _device(REMOTE), _mode(static_cast<Mode>(mode & (~COMMANDED))), _thread(0), _responsive(0), _predictor(((Predictor_Common::Type)Predictor::TYPE != Predictor_Common::NONE) ? new Predictor(this, Predictor_Configuration(), REMOTE==REMOTE) : 0)
    {
        _interested = new Interested(this, region, UNIT, TSTP::SINGLE, 0, expiry, period, (((mode & PREDICTIVE) == PREDICTIVE) ? (Predictor_Common::Type)Predictor::TYPE : Predictor_Common::NONE), Predictor_Configuration());
        TSTP::attach(this, _interested);
    }

    ~Smart_Data() {
        if(_thread)
            delete _thread;
        if(_interested) {
            TSTP::detach(this, _interested);
            delete _interested;
        }
        if(_responsive) {
            TSTP::detach(this, _responsive);
            delete _responsive;
        }
    }

    DB_Record db_record() {
        Value v = this->operator Value();
        Time t = time();
        TSTP::Global_Coordinates c = location();

        DB_Record ret;
        ret.version = STATIC_VERSION;
        ret.unit = _unit;
        ret.x = c.x;
        ret.y = c.y;
        ret.z = c.z;
        ret.t = t;
        ret.confidence = 0;
        ret.dev = _device;

        if(EQUAL<DB_Record, SI_Record>::Result) {
            SI_Record * si_record = reinterpret_cast<SI_Record*>(&ret);
            si_record->error = error();
            si_record->value = v;
            if(Traits<Build>::MODEL == Traits<Build>::eMote3) {
                // NOTE: Some ARM processors has a half little-endian floating-point representation for double-precision numbers.
                // Both 32-bit words are stored in little-endian, but the most significant one first.
                unsigned long long ll =  *((unsigned long long *)&ret.value);
                ll = ((ll&0xFFFFFFFF)<<32) + (ll>>32);
                si_record->value = *((double*)&ll);
            }
        } else {
            assert(sizeof(Value) == Transducer::UNIT & 0xFFFF);
            Digital_Record<Transducer::UNIT & 0xFFFF> * digital_record = reinterpret_cast<Digital_Record<Transducer::UNIT & 0xFFFF>*>(&ret);
            digital_record->error = 0;
            memcpy(&digital_record->value, &v, sizeof(Value));
        }
        return ret;
    }

    DB_Series db_series(){
        DB_Series ret;

        ret.version = STATIC_VERSION;
        ret.unit = _unit;

        if(_interested) {
            TSTP::Global_Coordinates c = TSTP::absolute(_interested->region().center);
            ret.x = c.x;
            ret.y = c.y;
            ret.z = c.z;
            ret.r = _interested->region().radius;
            ret.t0 = TSTP::absolute(_interested->region().t0);
            ret.t1 = TSTP::absolute(_interested->region().t1);
        } else {
            TSTP::Global_Coordinates c = location();
            ret.x = c.x;
            ret.y = c.y;
            ret.z = c.z;
            ret.r = 0;
            ret.t0 = 0;
            ret.t1 = -1;
        }
        ret.dev = _device;

        return ret;
    }

    operator Value() {
        if(expired()) {
            if((_device != REMOTE) && (Transducer::POLLING)) { // Local data source
                Transducer::sense(_device, this); // read sensor
                _time = TSTP::now();
            } else if(_device == REMOTE && ((_mode & PREDICTIVE) == PREDICTIVE)) {
                if(_predictor){
                    _time = TSTP::now();
                    _value = _predictor->predict(_time);
                }
            } else {
                // Other data sources must have called update() timely
                db<Smart_Data>(WRN) << "Smart_Data::get(this=" << this << ",exp=" <<_time +  _expiry << ",val=" << _value << ") => expired!" << endl;
            }
        }
        Value ret = _value;
        if(((_mode & CUMULATIVE) == CUMULATIVE))
            _value = 0;
        return ret;
    }

    Smart_Data & operator=(const Value & v) {
        if(_device != REMOTE)
            Transducer::actuate(_device, this, v);
        if(_interested)
            _interested->command(v);

        if(_responsive && !_thread) {
            _time = TSTP::now();
            _responsive->value(_value);
            _responsive->time(_time);
            _responsive->respond(_time + _expiry);
        }
        return *this;
    }

    bool expired() const { return TSTP::now() > (_time + _expiry); }

    TSTP::Global_Coordinates location() const { return TSTP::absolute(_coordinates); }
    const Time time() const { return TSTP::absolute(_time); }
    const Error & error() const { return _error; }
    const Unit & unit() const { return _unit; }

    const Power_Mode & power() const { return Transducer::power(); }
    void power(const Power_Mode & mode) const { Transducer::power(mode); }

    friend Debug & operator<<(Debug & db, const Smart_Data & d) {
        db << "{";
        if(d._device != REMOTE) {
            switch(d._mode) {
            case PRIVATE:    db << "PRI."; break;
            case ADVERTISED: db << "ADV."; break;
            case COMMANDED:  db << "CMD."; break;
            }
            db << "[" << d._device << "]:";
        }
        if(d._thread) db << "ReTT";
        if(d._responsive) db << "ReED";
        if(d._interested) db << "In" << ((d._interested->period()) ? "TT" : "ED");
        db << ":u=" << d._unit << ",v=" << d._value << ",e=" << int(d._error) << ",c=" << d._coordinates << ",t=" << d._time << ",x=" << d._expiry << "}";
        return db;
    }

private:
    void update(TSTP::Observed * obs, int subject, TSTP::Buffer * buffer) {
        TSTP::Packet * packet = buffer->frame()->data<TSTP::Packet>();
        db<Smart_Data>(TRC) << "Smart_Data::update(obs=" << obs << ",cond=" << reinterpret_cast<void *>(subject) << ",data=" << packet << ")" << endl;
        switch(packet->type()) {
        case TSTP::INTEREST: {
            if(((_mode & ADVERTISED) == ADVERTISED)) {
                TSTP::Interest * interest = reinterpret_cast<TSTP::Interest *>(packet);
                db<Smart_Data>(INF) << "Smart_Data::update[I]:msg=" << interest << " => " << *interest << endl;
                _responsive->t0(interest->region().t0);
                _responsive->t1(interest->region().t1);
                _responsive->interest(*interest);
                if(interest->mode() == TSTP::Mode::DELETE) {
                    if(_thread) {
                        delete _thread; // FIXME: There is a bug when this Interest mode is received.
                        _thread = 0;
                    }
                    if(_predictor){
                        delete _predictor;
                        _predictor = 0;
                    }
                } else if(interest->period()) {


                    if(((_mode & PREDICTIVE) == PREDICTIVE) && interest->predictive() && interest->predictor() == Predictor::TYPE){
                        if(interest->has_config()){
                            if(!_predictor){
                                _predictor = new Predictor(this, *interest->predictor_config<Predictor_Configuration>(), _device==REMOTE);
                                _predictive = new Predictive(typename Predictor::Model(), _unit, _error, _expiry);
                                if(Predictor::LISTENER)
                                    _responsive->model_listener(true);
                            } else {
                                _predictor->configure(*interest->predictor_config<Predictor_Configuration>());
                            }
                        } else if (interest->has_model()) {
                            _predictor->update(*interest->predictor_config<typename Predictor::Model>(), true);
                        }
                        if(_predictive){
                            _predictive->t0(interest->region().t0);
                            _predictive->t1(interest->region().t1);
                        }
                    }

                    if(!_thread)
                        _thread = new Periodic_Thread(interest->period(), &updater, _device, interest->expiry(), this);
                    else {
                        if(interest->period() != _thread->period())
                            _thread->period(interest->period());
                    }
                } else {
                    Transducer::sense(_device, this);
                    _time = TSTP::now();

                    if(_predictor){
                        //TODO
                    } else {
                        _responsive->value(_value);
                        _responsive->time(_time);
                        _responsive->respond(_time + interest->expiry());
                    }
                }
            }
        } break;
        case TSTP::RESPONSE: {
            TSTP::Response * response = reinterpret_cast<TSTP::Response *>(packet);
            db<Smart_Data>(INF) << "Smart_Data:update[R]:msg=" << response << " => " << *response << endl;
            if(response->time() > _time) {
                if(((_mode & CUMULATIVE) == CUMULATIVE))
                    _value += response->value<Value>();
                else
                    _value = response->value<Value>();
                _error = response->error();
                _coordinates = response->origin();
                _time = response->time();
                db<Smart_Data>(INF) << "Smart_Data:update[R]:this=" << this << " => " << *this << endl;
                notify();
            }
        } break;
        case TSTP::COMMAND: {
            if(((_mode & COMMANDED) == COMMANDED)) {
                // TODO: Check if this command was already treated
                TSTP::Command * command = reinterpret_cast<TSTP::Command *>(packet);
                if(_device != REMOTE)
                    Transducer::actuate(_device, this, *(command->command<Value>()));
                _coordinates = command->origin();
            }
        } break;
        case TSTP::CONTROL: {
            switch(buffer->frame()->data<TSTP::Control>()->subtype()) {
            case TSTP::MODEL: {
                TSTP::Model * model = reinterpret_cast<TSTP::Model *>(packet);
                if(model->model<Model_Common>()->type() == Predictor::Model::TYPE) {
                    if(_predictor){
                        _time = model->time();
                        _error = model->error();
                        _coordinates = model->origin();
                        _predictor->update(*model->model<typename Predictor::Model>(), false);
                        _value = _predictor->predict(_time);
                        notify();
                    }
                }
            } break;
            default:
                break;
            }
        } break;
        default:
            break;
        }
    }

    // Event-driven update
    void update(typename Transducer::Observed * obs) {
        _time = TSTP::now();
        Transducer::sense(_device, this);
        db<Smart_Data>(TRC) << "Smart_Data::update(this=" << this << ",exp=" << _expiry << ") => " << _value << endl;
        db<Smart_Data>(TRC) << "Smart_Data::update:responsive=" << _responsive << " => " << *reinterpret_cast<TSTP::Response *>(_responsive) << endl;
        notify();
        if(_responsive && !_thread) {
            _responsive->value(_value);
            _responsive->time(_time);
            _responsive->respond(_time + _expiry);
        }
    }

    // Time-triggered update
    static int updater(unsigned int dev, Time_Offset expiry, Smart_Data * data) {
        do {
            Time t = TSTP::now();
            // TODO: The thread should be deleted or suspended when time is up
            if(t < data->_responsive->t1()) {
                Transducer::sense(dev, data);
                data->_time = t;

                if(((data->_mode & PREDICTIVE) == PREDICTIVE) && data->_predictor){
                    if(!data->_predictor->trickle(data->_time, data->_value)){
                        data->_predictive->model(data->_predictor->model());
                        data->_predictive->time(t);
                        data->_predictive->respond(t + expiry);
                    }
                } else {
                    data->_responsive->value(data->_value);
                    data->_responsive->time(t);
                    data->_responsive->respond(t + expiry);
                    data->notify();
                }
            }
        } while(Periodic_Thread::wait_next());

        return 0;
    }

    //TODO
    //void update(const typename Predictor::Model & model, Region * region) {
    //    _interested->predictor_update(model);
    //}

private:
    Unit _unit;
    Value _value;
    Error _error;
    Coordinates _coordinates;
    TSTP::Time _time;
    TSTP::Time _expiry;

    unsigned int _device;
    unsigned char _mode;
    Periodic_Thread * _thread;
    Interested * _interested;
    Responsive * _responsive;

    Predictive * _predictive;
    Predictor * _predictor;
};


// Smart Data Transform and Aggregation functions

class No_Transform
{
public:
    No_Transform() {}

    template<typename T, typename U>
    void apply(T * result, U * source) {
        typename U::Value v = *source;
        *result = v;
    }
};

template<typename T>
class Percent_Transform
{
public:
    Percent_Transform(typename T::Value min, typename T::Value max) : _min(min), _step((max - min) / 100) {}

    template<typename U>
    void apply(U * result, T * source) {
        typename T::Value v = *source;
        if(v < _min)
            *result = 0;
        else {
            v = (v - _min) / _step;
            if(v > 100)
                v = 100;

            *result = v;
        }
    }

private:
    typename T::Value _min;
    typename T::Value _step;
};

template<typename T>
class Inverse_Percent_Transform: private Percent_Transform<T>
{
public:
    Inverse_Percent_Transform(typename T::Value min, typename T::Value max) : Percent_Transform<T>(min, max) {}

    template<typename U>
    void apply(U * result, T * source) {
        typename U::Value r;
        Percent_Transform<T>::apply(&r, source);
        *result = 100 - r;
    }
};

class Sum_Transform
{
public:
    Sum_Transform() {}

    template<typename T, typename ...U>
    void apply(T * result, U * ... sources) {
        *result = sum<T::Value, U...>((*sources)...);
    }

private:
    template<typename T, typename U, typename ...V>
    T sum(const U & s0, const V & ... s) { return s0 + sum<T, V...>(s...); }

    template<typename T, typename U>
    T sum(const U & s) { return s; }
};

class Average_Transform: private Sum_Transform
{
public:
    Average_Transform() {}

    template<typename T, typename ...U>
    void apply(T * result, U * ... sources) {
        typename T::Value r;
        Sum_Transform::apply(&r, sources...);
        *result = r / sizeof...(U);
    }
};


// Smart Data Actuator

template<typename Destination, typename Transform, typename ...Sources>
class Actuator: public Smart_Data_Common::Observer
{
public:
    Actuator(Destination * d, Transform * a, Sources * ... s)
        : _destination(d), _transform(a), _sources{s...}
    {
        attach(s...);
    }
    ~Actuator() {
        unsigned int index = 0;
        detach((reinterpret_cast<Sources*>(_sources[index++]))...);
    }

    void update(Smart_Data_Common::Observed * obs) {
        unsigned int index = 0;
        _transform->apply(_destination, (reinterpret_cast<Sources*>(_sources[index++]))...);
    }

private:
    template<typename T, typename ...U>
    void attach(T * t, U * ... u) { t->attach(this); attach(u...); }
    template<typename T>
    void attach(T * t) { t->attach(this); }

    template<typename T, typename ...U>
    void detach(T * t, U * ... u) { t->detach(this); detach(u...); }
    template<typename T>
    void detach(T * t) { t->detach(this); }

private:
    Destination * _destination;
    Transform * _transform;
    void * _sources[sizeof...(Sources)];
};

__END_SYS

#endif

#include <transducer.h>

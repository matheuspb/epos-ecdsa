// EPOS Predictor Declarations

#ifndef __predictor_h
#define __predictor_h

__BEGIN_UTIL

class Model_Common
{
public:
    // Model Types
    enum Type {
        NONE       = 0,
        CONSTANT   = 1,
        LINEAR     = 2,
        S_CONSTANT = 3,
        S_LINEAR   = 4,
    };
protected:
    Model_Common(const unsigned char & type) : _type(type), _id(0) {}
public:
    unsigned char type() const { return _type; }
    void id(unsigned char id) { _id = id; }
    unsigned char id() const { return _id; }
private:
    unsigned char _type;
    unsigned char _id;
} __attribute__((packed));

template<typename M, unsigned int T=M::_TYPE>
class Model : public Model<void, T>, public M{
public:
    template<typename ... Tn>
    Model(Tn ... tn) : Model<void, T>(), M(tn...) {}
} __attribute__((packed));

template<unsigned int T>
class Model<void, T> : public Model_Common {
public:
    static const unsigned int TYPE = T;
    Model() : Model_Common(T) {}
} __attribute__((packed));

template<typename T, typename V>
class Constant {
    typedef T Time;
    typedef V Value;
public:
    static const unsigned int _TYPE = Model_Common::CONSTANT;
    Constant(const Value & v = 0) : _v(v) {}
    Value operator()(const Time & t) { return _v; }
    Value v() const { return _v; }
    void v(const Value & v)  { _v = v; }
private:
    Value _v;
} __attribute__((packed));

template<typename T, typename V>
class Linear {
    typedef T Time;
    typedef V Value;
public:
    static const unsigned int _TYPE = Model_Common::LINEAR;
    Linear(const Value & a = 0, const Value & b = 0, const Time & t0 = 0) : _a(a), _b(b), _t0(t0) {}
    Value operator()(const Time & t) { return (_a*(t-_t0) + _b); }
    Value a() const { return _a; }
    void a(const Value & a)  { _a = a; }
    Value b() const { return _b; }
    void b(const Value & b) { _b = b; }
    Time t0() const { return _t0; }
    void t0(const Time & t0) { _t0 = t0; }
private:
    Value _a;
    Value _b;
    Time _t0;
} __attribute__((packed));

template<typename M, unsigned int T, typename C>
class Spatial : public M  {
public:
    static const unsigned int _TYPE = T;
    template<typename ... Tn> Spatial(const C & c, Tn ... tn) : M(tn...), _c(c) {}
    template<typename ... Tn> Spatial(Tn ... tn) : M(tn...) {}
    C coordinates() const { return _c; }
    void coordinates(const C & c)  { _c = c; }
private:
    C _c;
} __attribute__((packed));


// For C++11
//template<typename T, typename V>
//using Constant_Model = Model<Constant<T, V>>;

template<typename T, typename V, typename M=Model<Constant<T, V>>>
struct Constant_Model : public M {
    template<typename ... Tn> Constant_Model(Tn... tn) : M(tn...) {};
};

template<typename T, typename V, typename M=Model<Linear<T, V>>>
struct Linear_Model : public M {
    template<typename ... Tn> Linear_Model(Tn... tn) : M(tn...) {};
};

template<typename T, typename V, typename C, typename M=Model<Spatial<Constant<T, V>, Model_Common::S_CONSTANT, C>>>
struct S_Constant_Model : public M {
    template<typename ... Tn> S_Constant_Model(Tn... tn) : M(tn...) {};
};

template<typename T, typename V, typename C, typename M=Model<Spatial<Linear<T, V>,   Model_Common::S_LINEAR,   C>>>
struct S_Linear_Model : public M {
    template<typename ... Tn> S_Linear_Model(Tn... tn) : M(tn...) {};
};

template<typename M=Model<void, Model_Common::NONE>>
struct Dummy_Model : public M {
    template<typename ... Tn> Dummy_Model(Tn... tn) : M(tn...) {};
    int operator()(const int & t) { assert(false); return 0; }
};

// Entry Types
enum Entry_Types { TEMPORAL, NON_TEMPORAL };
template<Entry_Types K, typename V, typename ...O> struct Entry : V::NEVER { };
// Entry for temporal predictors
template<typename T, typename V>
struct Entry<TEMPORAL, T, V>
{
    Entry(const T & time=0, const V & value=0) : _time(time), _value(value) {}
    V value() const { return _value; }
    V time() const { return _time; }
private:
    T _time;
    V _value;
};
// Entry for non-temporal predictors
template<typename V>
struct Entry<NON_TEMPORAL, V>
{
    Entry(const V & value=0) : _value(value) {}
    V value() const { return _value; }
private:
    V _value;
};


class Predictor_Common
{
public:
    // Predictor Types
    enum Type {
        NONE       = 0,
        LAST_VALUE = 1,
        DBP        = 2
    };
    static const bool LISTENER = false;
protected:
    Predictor_Common(const unsigned char & type) : _type(type) {}
public:
    unsigned char type() const { return _type; }
private:
    unsigned char _type;
};

class Dummy_Predictor : public Predictor_Common
{
public:
    static const unsigned int TYPE = Predictor_Common::NONE;
    typedef Dummy_Model<> Model;
    struct Configuration { friend OStream & operator<<(OStream & os, const Configuration & c) { return os; } };
    template<typename S>
    Dummy_Predictor(S * data, Configuration c, bool r) : Predictor_Common(TYPE) {}
    template<typename T> int predict(const T & time) const { return 0; }
    template<typename S, typename T, typename ...O> void predict(S * data, const T & t, const O & ...o) const {}
    template<typename C> void configure(const C & conf) { }
    template<typename T, typename V> bool trickle(const T & t, const V & v) { assert(false); return false; }
    Model & model() const { assert(false); return *_model; }
    void model(const Model & m) { assert(false); }
    void update(const Model & m, const bool & from_sink) { model(m); }
private:
    Model * _model;
};

template<typename S>
class Last_Value_Predictor : public Predictor_Common
{
    typedef typename S::Time Time;
    typedef typename S::Value Value;
public:
    static const unsigned int TYPE = Predictor_Common::LAST_VALUE;
    typedef Constant_Model<Time, Value> Model;

    struct Configuration
    {
        Configuration(Value _r=0, Value _a=0, Time _t=0) : r(_r), a(_a), t(_t) {
            OStream cout;
            cout << "New Configuration: r="<<_r<<", a="<<_a<<", t="<<_t<< endl;
        }

        template<typename Config>
        Configuration(const Config & conf) : Configuration(conf.r, conf.a, conf.t) {}

        friend OStream & operator<<(OStream & os, const Configuration & c) {
            os << "{r="<<c.r<<",a="<<c.a<<",t="<<c.t<<"}";
            return os;
        }

        Value r;
        Value a;
        Time t;
    } __attribute__((packed));

    Last_Value_Predictor(Value r=0, Value a=0, Time t=0) : Predictor_Common(TYPE), _rel_err(r), _abs_err(a), _t_err(t), _model(0), _miss_predicted(0) {
        OStream cout;
        cout << "New Last_Value 1: r="<<r<<", a="<<a<<", t=" << t << endl;
    }

    Last_Value_Predictor(S * data, const Configuration & config, bool remote=false) : Predictor_Common(TYPE), _rel_err(config.r), _abs_err(config.a), _t_err(config.t), _model(0), _miss_predicted(0) {
        OStream cout;
        cout << "New Last_Value 2: r="<<config.r<<", a="<<config.a<<", t=" << config.t << endl;
    }

    template<typename ...O>
    Value predict(const Time & t, const O & ...o) const {
        if(_model)
            return (*_model)(t, o...);
        else
            return 0;
    }

    template<typename ...O>
    void predict(S * data, const Time & t, const O & ...o) const {
        data->_time = t;
        data->_value = predict(t);
    }

    bool trickle(const Time & time, const Value & value) {
        if(!_model) {
            _model = new Model();
            _model->v(value);
            return false;
        } else {
            float predicted = predict(time);
            OStream cout;
            float max_acceptable_error = max( abso(((float)value * (float)_rel_err)/100.0f), (float)_abs_err );
            float error = abso( (float)value - predicted );
            cout << "real:" << value << "  pred:" << predicted << "  err:" << error << "  max:" << max_acceptable_error << " t_err:"<< _t_err << " miss:" << _miss_predicted << endl;

            if(error > max_acceptable_error){
                if(++_miss_predicted > _t_err){
                    _model->v(value);
                    _miss_predicted = 0;
                    return false;
                }
            } else {
                _miss_predicted = 0;
            }

//            Value predicted = (*_model)(time);
//            if(predicted != value){
//                _model->v(value);
//                return false;
//            }
        }
        return true;
    }


    Model & model() const { return *_model; }
    void model(const Model & m) {
        if(!_model)
            _model = new Model();
        (*_model) = m;
    }
    void update(const Model & m, const bool & from_sink) { model(m); }

    void configure(const Configuration & config) {
        _rel_err = config.r;
        _abs_err = config.a;
        _t_err = config.t;
    }

private:
    template<typename T>
    T abso(const T & a){ return (a < 0) ? -a : a; }

    template<typename T>
    const T & max(const T & a, const T & b) { return (a >= b) ? a : b; }

    template<typename T>
    const T & min(const T & a, const T & b) { return (a <= b) ? a : b; }

private:
    Value _rel_err;
    Value _abs_err;
    Time _t_err;
    Model * _model;
    unsigned int _miss_predicted;
};

template<typename E, unsigned int W=10>
class Historical_Predictor : public Predictor_Common
{
public:
    Historical_Predictor(const unsigned char & type, unsigned int size = W) : Predictor_Common(type), history(new Cirular_Queue<E,W>(size)) {}

private:
    template<typename El, unsigned int SIZE>
    class Cirular_Queue
    {
        typedef El Element;
    public:
        Cirular_Queue(unsigned int size_limit = SIZE) : _size(0), _size_limit(size_limit <= SIZE ? size_limit : SIZE), _head(-1), _tail(-1) { }

        const Element & insert(const Element & el){
            if(full()) {
                _head = (_head + 1) % _size_limit;
                _tail = (_tail + 1) % _size_limit;
            } else {
                if(empty())
                    _head = _tail = 0;
                else
                    _tail = (_tail + 1) % _size_limit;
                _size++;
            }
            _queue[_tail] = el;
            return el;
        }

        unsigned int size() { return _size; }
        unsigned int max_size() { return _size_limit; }

        const Element & head() { return _queue[_head]; }
        const Element & tail() { return _queue[_tail]; }

        const Element & operator[](unsigned int index) {
            return _queue[(_head + index) % _size_limit];
        }

        bool empty() { return (_head == static_cast<unsigned int>(-1) && _tail == static_cast<unsigned int>(-1)); }
        bool full()  { return (_tail + 1) % _size_limit == _head ? true : false; }

        void clear() {
            _size = 0;
            _head = _tail = -1;
        }
    private:
        Element _queue[SIZE];
        unsigned int _size;
        unsigned int _size_limit;
        unsigned int _head;
        unsigned int _tail;
    };

protected:
    template<typename ...O>
    void store(const O & ...o) { history->insert(E(o...)); }

    Cirular_Queue<E,W> * history;
};

template<typename S, unsigned int W_MAX=100>
class DBP : public Historical_Predictor<Entry<Entry_Types::TEMPORAL, typename S::Time, typename S::Value>, W_MAX>
{
    typedef typename S::Time Time;
    typedef typename S::Value Value;
    typedef Historical_Predictor<Entry<Entry_Types::TEMPORAL, Time, Value>, W_MAX> Base;
    using Base::history;
    using Base::store;

public:
    static const unsigned int TYPE = Predictor_Common::DBP;

    typedef Linear_Model<Time, Value> Model;

    template<unsigned int _W, unsigned int _L, int _R=0, int _A=0 , Time _T=0>
    struct Configuration
    {
        //W, // window size
        //L, // number of edge points to calculate the coefficients.
        //R, // relative tolerance
        //A, // absolute tolerance
        //T, // time tolerance

        Configuration(Value _r=_R, Value _a=_A, Time _t=_T, unsigned int _w=_W, unsigned int _l=_L) : w(_w), l(_l), r(_r), a(_a), t(_t) {
            OStream cout;
            cout << "New Configuration: w="<<_w<<", l="<<_l<<", r="<<_r<<", a="<<_a<<", t="<<_t<< endl;
        }

        template<typename Config>
        Configuration(const Config & conf) : Configuration(conf.r, conf.a, conf.t, conf.w, conf.l) {}

        //Configuration() : Configuration(_W, _L, _R, _A, _T) {}

        friend OStream & operator<<(OStream & os, const Configuration & c) {
            os << "{w="<<c.w<<",l="<<c.l<<",r="<<c.r<<",a="<<c.a<<",t="<<c.t<<"}";
            return os;
        }

        unsigned int w;
        unsigned int l;
        Value r;
        Value a;
        Time t;
    } __attribute__((packed));

public:
    DBP(unsigned int w, unsigned int l, Value r=0, Value a=0, Time t=0) : Base(TYPE, w), _l(l), _rel_err(r), _abs_err(a), _t_err(t), _model(0), _miss_predicted(0) {
        OStream cout;
        cout << "New DBP 1: w="<<w<<", l="<<l<<", r="<<r<<", a="<<a<<", t=" << t << endl;
    }

    template<typename Config>
    DBP(S * data, const Config & config, bool remote=false) : Base(TYPE, config.w), _l(config.l), _rel_err(config.r), _abs_err(config.a), _t_err(config.t), _model(0), _miss_predicted(0) {
        OStream cout;
        cout << "New DBP 2: w="<<config.w<<", l="<<config.l<<", r="<<config.r<<", a="<<config.a<<", t=" << config.t << endl;
    }

    template<typename ...O>
    Value predict(const Time & t, const O & ...o) const {
        if(_model)
            return (*_model)(t, o...);
        else if(!history->empty())
            return (Value)history->tail().value();
        else
            return 0;
    }

    template<typename ...O>
    void predict(S * data, const Time & t, const O & ...o) const {
        data->_time = t;
        data->_value = predict(t);
    }

    void update(const Time & t, const Value & v) { store(t, v); }

    bool trickle(const Time & time, const Value & value) {
        update(time, value);

        if(!_model) {
            if(history->full()){
                build_model(time, value);
                return false;
            }
        } else {
            float predicted = predict(time);
            OStream cout;
            float max_acceptable_error = max( abso(((float)value * (float)_rel_err)/100.0f), (float)_abs_err );
            float error = abso( (float)value - predicted );
            cout << "real:" << value << "  pred:" << predicted << "  err:" << error << "  max:" << max_acceptable_error << " t_err:"<< _t_err << " miss:" << _miss_predicted << endl;

            if(error > max_acceptable_error){
                if(++_miss_predicted > _t_err){
                    build_model(time, value);
                    _miss_predicted = 0;
                    return false;
                }
            } else {
                _miss_predicted = 0;
            }
        }

        return true;
    }

    Model & model() const { return *_model; }
    void model(const Model & m) {
        if(!_model)
            _model = new Model();
        (*_model) = m;
    }
    void update(const Model & m, const bool & from_sink) { model(m); }

    template<typename C>
    void configure(const C & config) {
        _l = config.l;
        _rel_err = config.r;
        _abs_err = config.a;
        _t_err = config.t;
    }

private:
    template<typename T>
    T abso(const T & a){ return (a < 0) ? -a : a; }

    template<typename T>
    const T & max(const T & a, const T & b) { return (a >= b) ? a : b; }

    template<typename T>
    const T & min(const T & a, const T & b) { return (a <= b) ? a : b; }

protected:
    void build_model(const Time & t, const Value & v){
        assert(history->full());

        if(!_model)
            _model = new Model();

        float avg_oldest = 0;
        float avg_recent = 0;

        _model->t0((*history)[0].time());

        for(unsigned int i = 0; i < _l; i++)
            avg_oldest += (*history)[i].value();
        avg_oldest /= _l;

        for(unsigned int i = 0; i < _l; i++)
            avg_recent += (*history)[history->size()-1-i].value();
        avg_recent /= _l;

        float t_oldest = ( (*history)[0].time() + (*history)[_l-1].time() )/2;
        float t_recent = ( (*history)[history->size()-1].time() + (*history)[history->size()-1-(_l-1)].time() )/2;

        _model->a((avg_recent - avg_oldest)/(t_recent - t_oldest));
        _model->b(avg_oldest);
        _model->t0(t_oldest);

        float predicted = predict(t);
        float error = v - predicted;
        _model->b(avg_oldest+error);
    }

private:
    unsigned int _l;
    Value _rel_err;
    Value _abs_err;
    Time _t_err;
    Model * _model;
    unsigned int _miss_predicted;
};

__END_UTIL

#endif

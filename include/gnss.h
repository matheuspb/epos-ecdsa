// EPOS GNSS Mediator Common package

#ifndef __gnss_h
#define __gnss_h

#include <system/config.h>
#include <tstp.h>

__BEGIN_SYS

class GNSS_Common
{
public:
    typedef struct {
        float lat;
        float lon;
        float alt;
    } Coordinates;
    
    GNSS_Common();
    
    static TSTP::Global_Coordinates llh_to_xyz(const Coordinates &llh);
    
    void parse_msg();
    
    bool is_ready();
    
    Coordinates get_llh(void) {
        return _llh;
    }

    friend OStream & operator<<(OStream & os, const GNSS_Common & g) {
        os << "{lat=" << g._llh.lat << ",lon=" << g._llh.lon << ",alt=" << g._llh.alt << "}";
        return os;
    }
    
private:
    virtual char get_byte() = 0;
    
    void parse_GGA(void);
    
    void read_packet();
    float read_float(int *ptr);
    long read_integer(int *ptr, int sz = 0xFFFF);
    
private:
    char _packet[81]; // Max packet size is 81 without $ and <LF>
    int _ck;
    Coordinates _llh;
    
    bool _new_data;
};

__END_SYS

#ifdef __GNSS_H
#include __GNSS_H
#endif

#endif

// EPOS GNSS Mediator Common package

#include <system/config.h>
#include <gnss.h>
#include <utility/math.h>

__BEGIN_SYS

GNSS_Common::GNSS_Common() {
    _llh.lat = 0;
    _llh.lon = 0;
    _llh.alt = -6378.137; // Earth center
    _new_data = false;
}

TSTP::Global_Coordinates GNSS_Common::llh_to_xyz(const Coordinates &llh) {
    float lat, lon, alt;
    lat = llh.lat * M_PI / 180.0;
    lon = llh.lon * M_PI / 180.0;
    alt = llh.alt / 1000; // convert to km
    
    float x, y, z;
    float slat, slon, clat, clon;
    slat = sin(lat);
    slon = sin(lon);
    clat = cos(lat);
    clon = cos(lon);
    
    float rn = 6378.137/sqrt<float>(1 - (0.006694 * sin(lat)*sin(lat)));
    float rnalt = alt + rn;
    x = clat * clon * rnalt;
    y = clat * slon * rnalt;
    z = ((0.993306)*rn + alt) * slat;
    
    x *= 100000.0;
    y *= 100000.0;
    z *= 100000.0;
    return TSTP::Global_Coordinates(x, y, z);
}
    
void GNSS_Common::parse_msg() {
    read_packet();
    if (_packet[2] == 'G' && _packet[3] == 'G' && _packet[4] == 'A') {
        parse_GGA();
    }
}
    
bool GNSS_Common::is_ready() {
    bool tmp;
    tmp =_new_data;
    _new_data = false;
    return tmp;
}
    
void GNSS_Common::read_packet() {
    char c = 0;
    int i;
    bool end;
    while (c != '$') {
        c = get_byte();
        if (c > 128){
            _packet[2] = 0;
            _packet[3] = 0;
            _packet[4] = 0;
            return;
        }
    }
    end = false;
    _ck = 0;
    db<GNSS_Common>(TRC) << "GNSS_Common::read_packet() => ";
    for (i = 0; i < 81; i++) {
        c = get_byte();
        if (c == '*')
            end = true;
        if (!end)
            _ck ^= c;
        if (c == 10) // <LF>
            break;
        _packet[i] = c;
        db<GNSS_Common>(TRC) << c;
    }
    db<GNSS_Common>(TRC) << endl;
    return;
}

void GNSS_Common::parse_GGA(void) {
    int i;
    int ck;
    float lat, lon, alt;
    int lat_gr, lon_gr;
    int quality, n_sat;
    lat = 0;
    lon = 0;
    alt = 0;
    lat_gr = 0;
    lon_gr = 0;
    i = 6;
    db<GNSS_Common>(TRC) << "GNSS_Common::parse_GGA() => ";
    while (_packet[i++] != ',')
        ; // ignore day hour
        
    lat_gr = read_integer(&i, 2);
    db<GNSS_Common>(TRC) << "lat_gr=" << lat_gr << " ";
    if (_packet[i-1] != ',')
        lat = read_float(&i);
    if (_packet[i] == 'S') {
        lat = 0-lat;
        lat_gr = 0-lat_gr;
    }
    while (_packet[i++] != ',')
        ;
    
    lon_gr = read_integer(&i, 3);
    db<GNSS_Common>(TRC) << "lon_gr=" << lon_gr << " ";
    if (_packet[i-1] != ',')
        lon = read_float(&i);
    if (_packet[i] == 'W') {
        lon = 0-lon;
        lon_gr = 0-lon_gr;
    }
    while (_packet[i++] != ',')
        ;
    
    quality = read_integer(&i);
    db<GNSS_Common>(TRC) << "q=" << quality << " ";
    
    n_sat = read_integer(&i);
    db<GNSS_Common>(TRC) << "n=" << n_sat << " ";
    
    while (_packet[i++] != ',')
        ; // ignore Horizontal Dilution of precision
    
    alt = read_float(&i);
    if (_packet[i] == 'M') { // M for meters, other codes unknow
        // M for meters, other codes unknow
    }
    while (_packet[i++] != ',')
        ;
    
    
    while (_packet[i++] != ',')
        ; // Ignore diff sea-wgs84
    if (_packet[i] == 'M') { // M for meters, other codes unknow
        // M for meters, other codes unknow
    }
    while (_packet[i++] != ',')
        ;
    
    while (_packet[i++] != ',')
        ; // ignore Age of differential GPS data
    
    while (_packet[i++] != '*' && _packet[i-1] != 13)
        ; // ignore Differential reference station ID
    
    if (_packet[i-1] == '*')
        ck = read_integer(&i);
    else
        ck = _ck;
    
    // Must implement correct checksum
    if (quality > 0/* && ck == _ck*/) {
        lat /= 60.0;
        lon /= 60.0;
        _llh.lat = lat + lat_gr;
        _llh.lon = lon + lon_gr;
        _llh.alt = alt;
        db<GNSS_Common>(TRC) << "llh=" << _llh.lat << "," << _llh.lon;
        
        _new_data = true;
    }
    db<GNSS_Common>(TRC) << endl;
}

// Reads a floating point value from NMEA 0183 packet
float GNSS_Common::read_float(int *ptr) {
    char c;
    int i = *ptr;
    float num = 0;
    while ((c = _packet[i++]) != ',' && c != '.') {
        if (c >= '0' && c <= '9') {
            num = (num*10) + (c - '0');
        }
    }
    if (c == '.') {
        float factor = 0.1;
        while ((c = _packet[i++]) != ',') {
            if (c >= '0' && c <= '9') {
                num += factor*(c - '0');
                factor /= 10;
            }
        }
    }
    *ptr = i;
    return num;
}

// Reads a value from NMEA 0183 packet, limited to 'sz' digits
long GNSS_Common::read_integer(int *ptr, int sz) {
    char c;
    int i = *ptr;
    long num = 0;
    while ((c = _packet[i++]) != ',') {
        if (c >= '0' && c <= '9') {
            num = (num*10) + (c - '0');
        }
        if (i >= (*ptr+sz)) {
            break;
        }
    }
    *ptr = i;
    return num;
}

__END_SYS

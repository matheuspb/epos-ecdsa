// EPOS Trustful SpaceTime Protocol Initialization

#include <system/config.h>
#ifndef __no_networking__

#include <tstp.h>

__BEGIN_SYS

TSTP::TSTP()
{
    db<TSTP>(TRC) << "TSTP::TSTP()" << endl;
}

// TODO: we need a better way to define static locations
void TSTP::Locator::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Locator::bootstrap()" << endl;
    _confidence = 100;

    // This is used if your machine ID is unlisted below
    if(Traits<TSTP>::sink) {
        _here = TSTP::sink();
    }
    else {
        _here = Coordinates(10,10,0); // Adjust this value to the coordinates of the sensor
    }

    // You can edit the values below to define coordinates based on the machine ID
    if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x00\x00\x00\x00", 8)) // Adjust this value to the ID of the mote
        _here = TSTP::sink();
    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x00\x00\x00\x00", 8)) // Adjust this value to the ID of the mote
        _here = Coordinates(10,10,0); // Adjust this value to the coordinates of the sensor

    // To get global coordinates, see http://epos.lisha.ufsc.br/EPOS+2+User+Guide#Coordinates

    // UFSC - HU mesh 1
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //    _here = TSTP::sink();
    //    TSTP::coordinates(Global_Coordinates(374677500, -423772800, -293700300));
    // }
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x5e\x0e\x16\x06", 8)) // Water flow sensor 1
    //    _here = Coordinates(100,100,100);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x20\x0f\x16\x06", 8)) // Water flow sensor 2
    //    _here = Coordinates(-4900,-200,-6100);
    
    // UFSC - HU mesh 2
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x31\x0e\x16\x06", 8)) { // Sink
    //    _here = TSTP::sink();
    //    TSTP::coordinates(Global_Coordinates(374690300, -423766300, -293692900));
    // }
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xb3\x0e\x16\x06", 8)) // Water flow sensor 1
    //    _here = Coordinates(5500,-100,7000);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xb0\x0e\x16\x06", 8)) // Water flow sensor 2
    //    _here = Coordinates(4100,5600,-3000);

    //UFSC - Aplicacao (Carvoeira)
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374619700, -423795900, -293741100)); 
    //  }

    //UFSC - Exutorio
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374672400, -423762500, -293721000)); 
    //  }

    //UFSC - FAPEU
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374664300, -423764000, -293729500)); 
    //  }

    //UFSC - BU
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374665000, -423767000, -293725000));
    //  }

    //UFSC - CDS
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374636700, -423757700, -293773100)); 
    //  }

    //UFSC - CCJ
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374642900, -423784200, -293727600)); 
    //  }

    //UFSC - Arquitetura
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374656900, -423750400, -293758500)); 
    //  }

    //UFSC - Elefante
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374653600, -423759400, -293749600)); 
    //  }

    //Corrego Grande - Corrego Grande
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374795300, -423642000, -293737500)); 
    //  }

    //Corrego Grande - ESMESC
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374874100, -423624800, -293662600)); 
    //  }

    //Corrego Grande - Ana Davila
    // if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x59\x0e\x16\x06", 8)) { // Sink
    //     _here = TSTP::sink();
        // TSTP::coordinates(Global_Coordinates(374880900, -423611900, -293673500)); 
    //  }

    // LISHA Testbed
    // if(Traits<TSTP>::sink) { // Sink
    //    _here = TSTP::sink();
    //    TSTP::coordinates(Global_Coordinates(741869040, 679816341, 25300));
    // }
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x7f\x0e\x16\x06", 8)) // Dummy 0
    //    _here = Coordinates(10,5,0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x69\x0e\x16\x06", 8)) // Dummy 1
    //    _here = Coordinates(10,10,0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xca\x0e\x16\x06", 8)) // Dummy 2
    //    _here = Coordinates(5,15,0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x67\x83\x0d\x06", 8)) // Dummy 3
    //    _here = Coordinates(0,15,0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xec\x82\x0d\x06", 8)) // Dummy 4
    //    _here = Coordinates(-5,10,0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x97\x0e\x16\x06", 8)) // Dummy 5
    //    _here = Coordinates(-5,5,0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x11\x83\x0d\x06", 8)) // Outlet 0 (B0)
    //    _here = Coordinates(-270, -330, -15);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x74\x82\x0d\x06", 8)) // Outlet 1 (B1)
    //    _here = Coordinates(-735, -110, -15);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x5e\x83\x0d\x06", 8)) // Lights 1 (A1)
    //    _here = Coordinates(-425, -250, 220);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x0b\x0f\x16\x06", 8)) // Luminosity sensor
    //    _here = Coordinates(-720, -90, 0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x1a\x84\x0d\x06", 8)) // Router 1 (corridor, green)
    //    _here = Coordinates(-225, 40, 0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x65\x84\x0d\x06", 8)) // Router 2 (main door)
    //    _here = Coordinates(-210, 120, 140);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xb7\x82\x0d\x06", 8)) // Router 3 (Guto's door)
    //    _here = Coordinates(-270, -110, 160);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x5e\x0e\x16\x06", 8)) // Door
    //    _here = Coordinates(-200, 150, 200);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x0d\x3e\x3a\x06", 8)) // Presence
    //    _here = Coordinates(-720, -100, 0);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xc5\x3e\x3a\x06", 8)) // Perimeter sensor
    //    _here = Coordinates(-160, 452, -73);
    // else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x74\xb0\x0d\x06", 8)) // Temperature
    //    _here = Coordinates(-200, 100, 0);
    // else
    //    _confidence = 0;

    // SSB
    //if(Traits<TSTP>::sink) {
    //    _here = TSTP::sink();
    //    TSTP::coordinates(Global_Coordinates(375810800, -423903200, -292069200));
    //} else {
    //    // Student's room 1
    //    //
    //    //\x00\x4b\x12\x00\xf6\x82\x0d\x06
    //    /*
    //    if(!memcmp(Machine::id(), "", 8)) // Lights 0
    //        _here = Coordinates(430, 50, 250);
    //    else*/ if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x1a\x83\x0d\x06", 8)) // Lights 1
    //        _here = Coordinates(150, 50, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xcb\x82\x0d\x06", 8)) // Lights 2
    //        _here = Coordinates(-140, 50, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x04\x84\x0d\x06", 8)) // Lights 3
    //        _here = Coordinates(430, 300, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x1e\x83\x0d\x06", 8)) // Lights 4
    //        _here = Coordinates(150, 300, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x98\x83\x0d\x06", 8)) // Lights 5
    //        _here = Coordinates(-140, 300, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xd3\x82\x0d\x06", 8)) // Lights 7
    //        _here = Coordinates(150, 500, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xfa\x82\x0d\x06", 8)) // Lights 8
    //        _here = Coordinates(-140, 500, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x84\x82\x0d\x06", 8)) // NOT SURE Outlet 3
    //        _here = Coordinates(-140, -10, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x0b\x83\x0d\x06", 8)) // Outlet 4
    //        _here = Coordinates(-250, -10, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x4c\x83\x0d\x06", 8)) // Outlet 5
    //        _here = Coordinates(-260, 200, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x98\x82\x0d\x06", 8)) // Outlet 6
    //        _here = Coordinates(-260, 400, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x8b\x82\x0d\x06", 8)) // NOT SURE Outlet 9
    //        _here = Coordinates(5, 570, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x90\x82\x0d\x06", 8)) // NOT SURE Outlet 11
    //        _here = Coordinates(185, 570, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x03\x83\x0d\x06", 8)) // Outlet 12
    //        _here = Coordinates(345, 570, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x22\x83\x0d\x06", 8)) // Lux 0
    //        _here = Coordinates(15, 0, 0);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x56\xb0\x0d\x06", 8)) // Temperature 0
    //    //else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xa4\x82\x0d\x06", 8)) // Temperature 0
    //        _here = Coordinates(10, 0, 0);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x51\x82\x0d\x06", 8)) // Air Conditioner 0
    //        _here = Coordinates(430, -10, 220);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x47\x83\x0d\x06", 8)) // Air Conditioner 1
    //        _here = Coordinates(150, -10, 220);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x8e\x83\x0d\x06", 8)) // Water 0
    //        _here = Coordinates(1140, -688, 350);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x31\x84\x0d\x06", 8)) // Water 1
    //        _here = Coordinates(1090, -688, 350);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x7f\x82\x0d\x06", 8)) // Water 2 // NOT OK
    //        _here = Coordinates(-920, -870, 350);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x1b\xb0\x0d\x06", 8)) // Water 3 // OK
    //        _here = Coordinates(-970, -970, 350);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xdc\xaf\x0d\x06", 8)) // Water 4 // OK
    //        _here = Coordinates(-1020, -870, 350);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x02\x0b\x0d\x06", 8)) // Router 0 
    //        _here = Coordinates(610, 1700, 550);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xe7\x83\x0d\x06", 8)) // Water 5
    //        _here = Coordinates(710, 2800, 350);
    //    //else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xXX\xXX\xXX\xXX", 8)) // Rain 0 // TODO
    //    //    _here = Coordinates(2727, 3500, 0);

    //    // Student's room 2
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xd5\x3d\x3a\x06", 8)) // Lights 9
    //        _here = Coordinates(-1368, -613, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x75\x3a\x3a\x06", 8)) // Lights 10
    //        _here = Coordinates(-1510, -613, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xae\xaa\x0d\x06", 8)) // Lights 11
    //        _here = Coordinates(-1652, -613, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x00\x00\x00\x00", 8)) // Lights 12
    //        _here = Coordinates(-1368, -763, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x00\x00\x00\x00", 8)) // Lights 13
    //        _here = Coordinates(-1510, -763, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x0c\x39\x3a\x06", 8)) // Lights 14
    //        _here = Coordinates(-1652, -763, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x00\x00\x00\x00", 8)) // Lights 15
    //        _here = Coordinates(-1368, -914, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x00\x00\x00\x00", 8)) // Lights 16
    //        _here = Coordinates(-1510, -914, 250);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x8a\x38\x3a\x06", 8)) // Lights 17
    //        _here = Coordinates(-1652, -914, 250);

    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xa1\xaf\x0d\x06", 8)) // Outlet 13 (Door)
    //        _here = Coordinates(-1268, -823, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\x1c\xb0\x0d\x06", 8)) // Outlet 14
    //        _here = Coordinates(-1752, -603, -40);
    //    else if(!memcmp(Machine::id(), "\x00\x4b\x12\x00\xbe\xaf\x0d\x06", 8)) // Outlet 15
    //        _here = Coordinates(-1752, -613, -40);

    //    else
    //        _confidence = 0;
    //}

    if(Traits<Radio>::promiscuous) {
        _here = Coordinates(12,12,12);
        _confidence = 100;
    }

#if defined(__GXCOOR) && defined(__GYCOOR) && defined(__GZCOOR)
    TSTP::coordinates(Global_Coordinates(__GXCOOR, __GYCOOR, __GZCOOR));
#endif
#if defined(__XCOOR) && defined(__YCOOR) && defined(__ZCOOR)
    _here = Coordinates(__XCOOR, __YCOOR, __ZCOOR);
    _confidence = 100;
#endif

    TSTP::_nic->attach(this, NIC::TSTP);

    // Wait for spatial localization
    while(_confidence < 80)
        Thread::self()->yield();
}

void TSTP::Timekeeper::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Timekeeper::bootstrap()" << endl;

    if(here() == sink())
        _next_sync = -1ull; // Just so that the sink will always have synchronized() return true

    TSTP::_nic->attach(this, NIC::TSTP);

    if((TSTP::here() != TSTP::sink()) && (!Traits<Radio>::promiscuous)) { // TODO
        _life_keeper = new (SYSTEM) Alarm(SYNC_PERIOD, _life_keeper_handler, Alarm::INFINITE);
        // Wait for time synchronization
        while(sync_required())
            Thread::self()->yield();
    }
}

void TSTP::Router::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Router::bootstrap()" << endl;
    TSTP::_nic->attach(this, NIC::TSTP);
}

void TSTP::Security::bootstrap()
{
    db<TSTP>(TRC) << "TSTP::Security::bootstrap()" << endl;

    TSTP::_nic->attach(this, NIC::TSTP);

    /*
    // TODO
    if((TSTP::here() != TSTP::sink()) && (!Traits<Radio>::promiscuous)) {
        Peer * peer = new (SYSTEM) Peer(_id, Region(TSTP::sink(), 0, 0, -1));
        _pending_peers.insert(peer->link());

        // Wait for key establishment
        while(_trusted_peers.size() == 0)
            Thread::self()->yield();
    }
    */
}

template<unsigned int UNIT>
void TSTP::init(const NIC & nic)
{
    db<Init, TSTP>(TRC) << "TSTP::init(u=" << UNIT << ")" << endl;

    _nic = new (SYSTEM) NIC(nic);
    TSTP::Locator * locator = new (SYSTEM) TSTP::Locator;
    TSTP::Timekeeper * timekeeper = new (SYSTEM) TSTP::Timekeeper;
    TSTP::Router * router = new (SYSTEM) TSTP::Router;
    TSTP::Security * security = new (SYSTEM) TSTP::Security;
    TSTP * tstp = new (SYSTEM) TSTP;

    locator->bootstrap();
    timekeeper->bootstrap();
    router->bootstrap();
    security->bootstrap();

    _nic->attach(tstp, NIC::TSTP);
}

template void TSTP::init<0>(const NIC & nic);
template void TSTP::init<1>(const NIC & nic);
template void TSTP::init<2>(const NIC & nic);
template void TSTP::init<3>(const NIC & nic);
template void TSTP::init<4>(const NIC & nic);
template void TSTP::init<5>(const NIC & nic);
template void TSTP::init<6>(const NIC & nic);
template void TSTP::init<7>(const NIC & nic);

__END_SYS

#endif

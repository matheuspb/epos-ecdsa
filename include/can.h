// EPOS CAN Controller Mediator Common package

#ifndef __can_h
#define __can_h

#include <system/config.h>

__BEGIN_SYS

class CAN_Common
{
protected:
    CAN_Common() {}

public:
    enum Filter {
        LISTEN_ONLY,
        NORMAL,
    };
};

__END_SYS

#ifdef __CAN_H
#include __CAN_H
#endif

#endif

// EPOS ARM Cortex AES Mediator Declarations

#ifndef __cortex_aes_h
#define __cortex_aes_h

#include <aes.h>

__BEGIN_SYS

template<unsigned int KEY_SIZE>
class AES: public AES_Common::AES<KEY_SIZE> {};

__END_SYS

#endif

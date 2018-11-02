// EPOS PC AES Mediator Declarations

#ifndef __pc_aes_h
#define __pc_aes_h

#include <aes.h>

__BEGIN_SYS

template<unsigned int KEY_SIZE>
class AES: public AES_Common::AES<KEY_SIZE> {};

__END_SYS

#endif

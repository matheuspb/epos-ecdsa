#ifndef __cortex_rsa_h
#define __cortex_rsa_h


#include <rsa.h>
#include <utility/rsa.h>
#include <system/config.h>
#include <utility/keypair.h>

__BEGIN_SYS

class RSA_Cortex : public RSA_Common
{
//	typedef HWBignum Number;
//
public:
	RSA_Cortex() {};
	static Number sign(Private_Key* pk, Number& message) {
		return message.exp_mod(*pk->_ks_sk, *pk->modulo);
	};

	static bool verify(Public_Key* pk, Number& message, Number& plaintext) {
		Number deciph = message.exp_mod(*pk->_ks_pk, *pk->modulo);
		if(deciph == plaintext)
			return true;
		return false;
	};

	static Number decrypt(Public_Key* pk, Number& message) {
			return message.exp_mod(*pk->_ks_pk, *pk->modulo);
	};

};

class RSA: public IF<Traits<Build>::MODEL == Traits<Build>::eMote3, RSA_Cortex, RSA_Utility>::Result
{

private:
    typedef IF<Traits<Build>::MODEL == Traits<Build>::eMote3, RSA_Cortex, RSA_Utility>::Result Engine;

public:
    RSA(): Engine()
    {

    }

};

__END_SYS


#endif

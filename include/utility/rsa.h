// EPOS RSA Utility 

#ifndef __utility_rsa_h
#define __utility_rsa_h

#include <system/config.h>
#include <utility/ostream.h>
#include <utility/malloc.h>

__BEGIN_UTIL

typedef unsigned long long Value;

struct Util_Private_Key {
	Value modulo;
	Value _ks_sk;
	Util_Private_Key(Value n, Value sk) :
		modulo(n), _ks_sk(sk){}
	Util_Private_Key(){}
};

struct Util_Public_Key {
	Value modulo;
	Value _ks_pk;
	Util_Public_Key(Value n, Value pk) :
				modulo(n), _ks_pk(pk){}
	Util_Public_Key(){}
};

class RSA_Utility
{

public:

	RSA_Utility() { OStream cout; cout << "UTILITY " << endl;};

	Value sign(Util_Private_Key pk, Value message) const
	{
		return mod_exp(message, pk._ks_sk, pk.modulo);
	};

	Value verify(Util_Public_Key pk, Value message) const
	{
		return mod_exp(message, pk._ks_pk, pk.modulo);;
	};

private:

	//Método auxiliar para o calculo das funçoes de assinar e verificar
	static Value mod_exp(Value base, Value exponent, const Value n)
	{
		int i = 1;
		Value y = 1;
		while(exponent > 1)
		{
			if((exponent % 2) == 1){
				exponent -= 1;
				y *= base;
				y %= n;
			}
			base *= base;
			base %= n;
			exponent /= 2;
			i++;
		}

		return (base * y) % n;
	}

};

__END_UTIL

#endif /*__utility_rsa_h*/

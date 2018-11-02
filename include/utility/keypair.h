#ifndef __keystore_h__
#define __keystore_h__

#include <system/config.h>
#include <utility/ostream.h>
#include <utility/malloc.h>

#include <hw_types.h>
#include <stdint.h>
#include <pka.h>
#include <utility/hwbignum.h>

__BEGIN_SYS
typedef HWBignum Number;

struct Private_Key {
	Number* modulo;
	Number* _ks_sk;
	Private_Key(Number& n, Number& sk) :
		modulo(&n), _ks_sk(&sk){}
	Private_Key(){}
};

struct Public_Key {
	Number* modulo;
	Number* _ks_pk;
	Public_Key(Number& n, Number& pk) :
				modulo(&n), _ks_pk(&pk){}
	Public_Key(){}
};

class KeyPair {
private:

public:


	KeyPair() {}
	KeyPair(Number& n, Number& sk, Number& pk) :
	n(n),
	d(sk),
	e(pk) {}

	Public_Key* getPublicKey() {
		return &Public_Key(n, e);
	}

	Private_Key* getPrivateKey() {
		return &Private_Key(n, d);
	}
	enum size {
	        _64BITS = 1,
	        _128BITS = 2,
	        _192BITS = 3,
			_256BITS = 4,
			_320BITS = 5,
			_384BITS = 6,
			_448BITS = 7,
			_512BITS = 8
	    };


	void generateKeyPair(uint32_t size) {
		OStream cout;
		Number phiN = Number::zero;
		{
			Number p = Number(0x0, size*4);
			bool isprime = false;

			while (!isprime) {
				p.random_same_size();
				isprime = prob_prime(p, 3);
			}
			isprime = false;
			Number q = Number(0x0, size*4);
			while (!isprime) {
				q.random_same_size();
				isprime = prob_prime(q, 3);
			}

			n = q*p;

			{
				Number p1 = p - HWBignum::one;
				Number q1 = q - HWBignum::one;
				phiN = p1 * q1;
			}
		}

		e = Number(65537);
		d = e.inv_mod(phiN);


		coutt << "Key generator end." << endl;
		coutt << "Public value:" << endl;
		e.printResult(coutt);
		coutt << "Modular value:" << endl;
		n.printResult(coutt);
		coutt << "Private value:" << endl;
		d.printResult(coutt);

	}


	static Number rand_betwen(Number& n1, Number& n2) {
		Number _a = Number(0x0, n2.getLen()*4);
		_a.random_same_size();
		return _a;
	}

	static bool prob_prime(Number& n, int rounds) {
		Number v2 = Number(2);
		Number v3 = Number(3);

		if(n == Number::one || n == v2 || n == v3) return true;

		if (n.is_even()) return false;

		uint32_t s = 0;

		{
			Number m = n - Number::one;
			while(m.is_even()){
				++s;
				m = m >> (const uint32_t)1;
			}
		}
		Number d = Number::zero;
		{
			Number temp = Number::one;
			Number dividend= n - Number::one;

			Number divisor  = temp << s;
			d = (dividend) / (divisor);
		}

		Number up_limit = n - v2;
		Number temp2 = (n - Number::one);
		for(uint32_t i = 0; i < rounds; ++i) {

			Number a = rand_betwen(v2, up_limit);
			Number x = a.exp_mod(d, n);
			bool aa = x == Number::one;

			bool bb = x == temp2;
			if( aa || bb ) continue;
			for (uint8_t r = 0; r < (s - 1); ++r) {
				x = x.exp_mod(v2, n);
				if(x == Number::one) return false;

				if(x == temp2) break;
			}

			if(x != temp2) return false;
		}
		return true;
	}
private:

	Number e;
	Number d;
	Number n;
};

__END_SYS

#endif /* __keystore_h__ */

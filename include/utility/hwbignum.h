#ifndef __hwbignum_h__
#define __hwbignum_h__

#include <system/config.h>
#include <utility/ostream.h>
#include <utility/malloc.h>

#include <sha-init.h>
#include <sha-int-cfg.h>
#include <hw_types.h>
#include <stdint.h>
#include <pka.h>
#include <utility/bignum.h>

__BEGIN_UTIL
OStream coutt;

template <typename> class ECPoint;

class HWBignum{
    template <typename> friend class ECPoint;

public:
	typedef uint8_t tPKAStatus;

	unsigned int getLen() {
		return _len;
	}

	HWBignum() {
		_data = new uint32_t[1];
		_len = 1;
		_data[0] = 0;
		_signal = 0;
	}

	HWBignum(uint32_t n) {
		_data = new uint32_t[1];
		_len = 1;
		_data[0] = n;
		_signal = 0;
	}

	HWBignum(const uint32_t * bytes, unsigned int len) {
		_len = len / 4;
		_data = new uint32_t[_len];
		_signal = 0;
		for(uint32_t i = 0, j = 0; i < _len; i++) {
		    _data[i] = 0;
		    for(unsigned int k = 0; k < sizeof(uint32_t) && j < len; k++, j++)
		        _data[i] += (reinterpret_cast<const unsigned char *>(bytes)[j] << (8 * k));
		}
	}
    HWBignum(const HWBignum & b) {
    	_len = b._len;
    	_data = new uint32_t[_len];
    	_signal = b._signal;
        for(uint32_t i = 0; i < _len; i++) {
			_data[i] = 0;
        	_data[i] = b._data[i];
        }
    }

    HWBignum& operator=(const HWBignum & b) {
    	this->_len = b._len;
    	if(this->_data) delete this->_data;
    	this->_data = new uint32_t[this->_len];
    	this->_signal = b._signal;
        for(uint32_t i = 0; i < this->_len; i++) {
			this->_data[i] = 0;
        	this->_data[i] = b._data[i];
        }
        return *this;
    }

    HWBignum& operator=(HWBignum & b) {
    	_len = b._len;
    	if(_data) delete _data;
    	_data = new uint32_t[_len];
    	_signal = b._signal;
        for(uint32_t i = 0; i < _len; i++) {
			_data[i] = 0;
        	_data[i] = b._data[i];
        }

        return *this;
    }


	virtual ~HWBignum() {
		if(_data) delete[] _data;
	}

	virtual bool operator==(const HWBignum & b) const {
		return cmp(this, &b) == 0;
	}
	virtual bool operator!=(const HWBignum & b) const {
		return cmp(this, &b) != 0;
	}
	virtual bool operator>=(const HWBignum & b) const {
		int res = cmp(this, &b);
		return res == 1 || res == 0;
	}
	virtual bool operator<=(const HWBignum & b) const {
		int res = cmp(this, &b);
		return res == -1 || res == 0;
	}
	virtual bool operator>(const HWBignum & b) const {
		return cmp(this, &b) == 1;
	}
	virtual bool operator<(const HWBignum & b) const {
		return cmp(this, &b) == -1;
	}
	HWBignum operator<<(const uint32_t &value)__attribute__((noinline)) {
		uint32_t i = value;
		if(i > 31) {
			for(; i > 31; i-=31) {
				shift(31, 0);
			}
		}
		return shift(value, 0);
	}
	HWBignum operator>>(const uint32_t &value)__attribute__((noinline)) {
		return shift(value, 1);
	}
	virtual void operator*=(const HWBignum & b) __attribute__((noinline)) {
		HWBignum result = (*this) * b;
		delete this->_data;
		*this = result;
	}
	virtual void operator+=(const HWBignum &b)__attribute__((noinline)) {
		HWBignum result = (*this) + b;
		delete this->_data;
		*this = result;
	}
	virtual void operator-=(const HWBignum &b)__attribute__((noinline)) {
		HWBignum result = (*this) - b;
		delete this->_data;
		*this = result;
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 0 < A_Len
	 * B_Len ≤ Max_len
	 *
	 */
	HWBignum operator+(const HWBignum &b) const {
		// Add operation with both numbers positiv
		if(_signal == b._signal) {
			HWBignum result;
			tPKAStatus opStatus = 0;
			uint32_t res_addr = 0;
			int res_size;
			res_size = b._len;


			if(*this > b) {
				res_size = _len;
			}
			result = HWBignum(0x0, (res_size + 1)*4);

			opStatus = add_start((uint32_t*)_data, _len,
					(uint32_t*)b._data, b._len, &res_addr);
			result._signal = _signal;
			do {
				opStatus = add_result((uint32_t*)result._data, (uint32_t*)&result._len, res_addr);
				if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
				}
			}
			while (opStatus == PKA_STATUS_OPERATION_INPRG);
			if(opStatus == 4) return HWBignum(0);
			return result;
		} else {
			if(!_signal) {
				HWBignum temp_b = b;
				temp_b._signal = 0;
				return *this-temp_b;
			} else {
				HWBignum temp_a = *this;
				temp_a._signal = 0;
				temp_a = temp_a-b;
				temp_a._signal = 1;
				return temp_a;
			}
		}
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 0 < A_Len
	 * B_Len ≤ Max_len
	 * Result must be positive (A ≥ B)
	 *
	 */
	HWBignum operator-(const HWBignum &b) const {
		uint32_t res_addr = 0;
		tPKAStatus opStatus = 0;

		HWBignum result;

		if(_signal == b._signal) {
			if(*this >= b) {

				result = HWBignum(0x0, _len*4);
				result._signal = 0;
				sub_start((uint32_t*)_data, _len,
							(uint32_t*)b._data, b._len, &res_addr);
			} else {

				result = HWBignum(0x0, b._len*4);
				result._signal = 1;
				sub_start((uint32_t*)b._data, b._len,
							(uint32_t*)_data, _len, &res_addr);
			}
			do {
				opStatus = sub_result((uint32_t*)result._data, (uint32_t*)&result._len, res_addr);
				if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
				}
			}
			while (opStatus == PKA_STATUS_OPERATION_INPRG);

			if(opStatus == 4) {
				return HWBignum(0);
			}
			return result;
		} else {
			if(*this >= b) {
				HWBignum temp = b;
				temp.setSignal(0);
				return *this + temp;

			} else {
				HWBignum temp = *this;
				temp.setSignal(0);
				temp = temp + b;
				temp.setSignal(1);
				return temp;
			}
		}
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 0 < A_Len
	 * B_Len ≤ Max_len
	 *
	 */
	HWBignum operator*(const HWBignum &b) const __attribute__((noinline)) {
		if(*this == HWBignum::zero) return HWBignum(0);

		int res_size = _len + b._len + 6;
		HWBignum result = HWBignum(0x0, res_size*4);
		uint32_t res_addr = 0;

		tPKAStatus opStatus = 0;

		do {
			opStatus = mul_start((uint32_t*)_data, _len,
					(uint32_t*)b._data, b._len, &res_addr);
		}while (opStatus == PKA_STATUS_OPERATION_INPRG);

		do {
			opStatus = mul_result((uint32_t*)result._data, (uint32_t*)&result._len, res_addr);
			if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
			}
		}
		while (opStatus == PKA_STATUS_OPERATION_INPRG);

		if(_signal != b._signal) {
			result._signal = 1;
		} else {
			result._signal = 0;
		}

		return result;
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 1 < B_len ≤ A_Len ≤ Max_Len
	 * Most significant 32-bit word of B cannot be zero
	 *
	 */
	HWBignum operator/(const HWBignum &b)__attribute__((noinline)) {
		if(cmp_abs(this, &b)== -1) {
			coutt << "DIV ERROR, B > A" << endl;
			return HWBignum(0);
		}

		uint32_t* mult_value = new uint32_t[2];
		mult_value[0] = 0x00000000;
		mult_value[1] = 0x00000001;

		HWBignum word_mult= HWBignum(mult_value, 2*4);

		HWBignum temp_a = word_mult * (*this);

		HWBignum temp_b = word_mult * b;

		HWBignum c = HWBignum(new uint32_t[temp_b._len+1], (temp_b._len+1)*4);


		HWBignum d = HWBignum(0x0,
				(temp_a._len - temp_b._len + 1)*4);

		uint32_t res_addr = 0;
		div_start((uint32_t*)temp_a._data, temp_a._len,
				(uint32_t*)temp_b._data, temp_b._len,
				(uint32_t*)c._data, c._len,
				&res_addr);

		tPKAStatus opStatus = 0;
		do {
			opStatus = div_result((uint32_t*)d._data, (uint32_t*)&d._len, res_addr);
			if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
			}
		}
		while (opStatus == PKA_STATUS_OPERATION_INPRG);
		if(opStatus == PKA_STATUS_RESULT_0) {
			return HWBignum(0);
		}

		if(_signal != b._signal) {
			d._signal = 1;
		} else {
			d._signal = 0;
		}

		return d;
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 1 < B_len ≤ A_Len ≤ Max_Len
	 * Most significant 32-bit word of B cannot be zero
	 *
	 */
	HWBignum operator%(const HWBignum &b) {
		if(!_signal ) {
			HWBignum result = HWBignum(0x0, (b._len+1)*4);
			uint32_t res_addr = 0;
			mod_start((uint32_t*)_data, _len,
					(uint32_t*)b._data, b._len,
					&res_addr);

			tPKAStatus opStatus = 0;
			do {
				opStatus = mod_result((uint32_t*)result._data, (uint32_t*)&result._len, res_addr);
				if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
				}
			}
			while (opStatus == PKA_STATUS_OPERATION_INPRG);

			return result;
		} else {
			HWBignum result = HWBignum(0x0, (b._len+1)*4);
			uint32_t res_addr = 0;
			mod_start((uint32_t*)_data, _len,
					(uint32_t*)b._data, b._len,
					&res_addr);

			tPKAStatus opStatus = 0;
			do {
				opStatus = mod_result((uint32_t*)result._data, (uint32_t*)&result._len, res_addr);
				if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
				}
			}
			while (opStatus == PKA_STATUS_OPERATION_INPRG);

			return (HWBignum)b - result;
		}
	}

	HWBignum exp_mod(HWBignum& b, HWBignum& c) {
		HWBignum temp_a = *this;
		HWBignum temp_c = c;
		temp_a.append(c._len - _len + 1) ;
		temp_c.append(1);

		HWBignum d = HWBignum(0x0, 5*(temp_c._len+2)*4);
		uint32_t res_addr = 0;
		tPKAStatus opStatus;

		opStatus = exp_mod_start((uint32_t*)b._data, b._len,
				(uint32_t*)temp_c._data, c._len,
				(uint32_t*)temp_a._data, c._len,
				&res_addr);
		if(opStatus != PKA_STATUS_SUCCESS) {
			coutt << "erro no start numero: " << opStatus << endl;
		}
		do {
			opStatus = exp_mod_result((uint32_t*)d._data, (uint32_t*)&d._len, res_addr);
			if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
			}
		}
		while (opStatus == PKA_STATUS_OPERATION_INPRG);

		return d;
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 0 < A_Len ≤ Max_len
	 * B has A_Len as length, B_Len ignored
	 *
	 * return:
	 * -1 if a < b
	 *  0 if a == b
	 *  1 if a > b
	 *  10 in case an error occurred
	 */
	static int cmp(const HWBignum* a, const HWBignum * b) {
		if(a->_signal > b->_signal) {
			return -1;
		} else if (a->_signal < b->_signal) {
			return 1;
		}


		if(!a->_signal) {
			if(a->_len > b->_len) {
				return 1;
			} else if (a->_len < b->_len){
				return -1;
			}

			uint32_t res_addr = 0;
			cmp_start((uint32_t*)a->_data,
					(uint32_t*)b->_data, a->_len);
		} else {
			if(a->_len < b->_len) {
				return 1;
			} else if (a->_len > b->_len){
				return -1;
			}

			uint32_t res_addr = 0;
			cmp_start((uint32_t*)b->_data,
					(uint32_t*)a->_data, b->_len);
		}


		tPKAStatus opStatus = 0;
		do {
			opStatus = cmp_result();
			if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
			}
		}
		while (opStatus == PKA_STATUS_OPERATION_INPRG);

		switch(opStatus) {
			case PKA_STATUS_SUCCESS:
			return 0;
			case PKA_STATUS_A_GR_B:
			return 1;
			case PKA_STATUS_A_LT_B:
			return -1;
			default:
			return 10;
		}

		return 10;
	}

	bool is_zero() {
		return _data[_len] == 0x0;

	}

	int cmp_abs(const HWBignum* a, const HWBignum * b) {

		if(a->_len > b->_len) {
			return 1;
		} else if (a->_len < b->_len){
			return -1;
		}

		uint32_t res_addr = 0;
		cmp_start((uint32_t*)a->_data,
				(uint32_t*)b->_data, a->_len);

		tPKAStatus opStatus = 0;
		do {
			opStatus = cmp_result();
			if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
			}
		}
		while (opStatus == PKA_STATUS_OPERATION_INPRG);

		switch(opStatus) {
			case PKA_STATUS_SUCCESS:
			return 0;
			case PKA_STATUS_A_GR_B:
			return 1;
			case PKA_STATUS_A_LT_B:
			return -1;
			default:
			return 10;
		}

		return 10;
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 0 < A_len ≤ Max_Len
	 * 0 < B_Len ≤ Max_len
	 * B must be odd
	 * B may not have value 1 (UNDEFINED RESULT!!)
	 * The Highest word of the Modulus vector may not be zero
	 *
	 */
	HWBignum inv_mod(HWBignum& b) const {
		/*HWBignum d_old = HWBignum::zero;
		HWBignum d_new = HWBignum::one;
		HWBignum r_new = *this;
		HWBignum r_old = b;

		while(r_new > HWBignum::zero) {
			HWBignum temp;
			HWBignum a = r_old / r_new;

			temp = d_old;
			d_old = d_new;
			d_new = a * d_new;

			d_new = temp - d_new;

			temp = r_old;
			r_old = r_new;
			r_new = a * r_new;

			r_new = temp - r_new;
		}

		if(r_old == HWBignum::one) {
			return d_old % b;
		}
		return HWBignum(0);*/
        HWBignum result = HWBignum(0x0, b._len*4);
        uint32_t res_addr = 0;
        inv_mod_start(_data, _len, b._data, b._len, &res_addr);

        tPKAStatus opStatus = 0;
        do {
            opStatus = inv_mod_result(result._data, (uint32_t*) &result._len, res_addr);
        } while (opStatus == PKA_STATUS_OPERATION_INPRG);

        return result;
	}

	/*************************************************************
	 *
	 * Important note!!
	 *
	 * Operation Restrictions:
	 * 0 < A_len ≤ Max_Len
	 *
	 * Shift Direction:
	 * << = 0
	 * >> = 1
	 *
	 */
	HWBignum shift(const uint8_t s_value, const uint8_t direction) {
		if(*this == HWBignum::zero) return HWBignum(0);
		uint32_t res_size = this->_len;
		if ( direction == 0 && s_value > 0) {
			res_size = this->_len + 1;
		}

		HWBignum result = HWBignum(0x0, res_size*4);
		uint32_t res_addr = 0;

		shift_start((uint32_t*)_data, _len,
				s_value, direction,
				&res_addr);
		tPKAStatus opStatus = 0;
		do {
			opStatus = shift_result((uint32_t*)result._data, (uint32_t*)&result._len, res_addr);
			if(opStatus != PKA_STATUS_SUCCESS && opStatus != PKA_STATUS_OPERATION_INPRG) {
			}
		}
		while (opStatus == PKA_STATUS_OPERATION_INPRG);
		if(opStatus == 4) return HWBignum(0);
		return result;
	}

    void random_same_size() { // Sets _data to a random number smaller than _mod

        for(int i = 0; i < _len; i++) {
        	_data[i] = Random::random();
        }
    }

    void random_lesser_than(HWBignum& n) { // Sets _data to a random number smaller than _mod
    	random_same_size();
    	_data[_len] = Random::random() % n._data[n._len];
    }

    void printResult(OStream & cout)
 	{

 		unsigned char* Message_Digest = (unsigned char *)_data;
 		const char hexdigits[ ] = "0123456789ABCDEF";
 		int i;
 		cout << (_signal?"-":"");
 		for (i = (_len)*4-1; i >= 0; --i) {
 		  cout << (hexdigits[(Message_Digest[i] >> 4) & 0xF]);
 		  cout << (hexdigits[Message_Digest[i] & 0xF]);
 		}
 		cout << endl;
 	}

    bool is_even() { return !(_data[0] % 2); }

    friend OStream &operator<<(OStream & out, const HWBignum & b){
        out << '[';
        for(unsigned int i = 0; i < b._len; i++) {
            out << (uint32_t) b._data[i];
            if(i < b._len - 1)
                out << ", ";
        }
        out << "]";
        return out;
    }

    friend Debug &operator<<(Debug & out, const HWBignum & b) {
        out << '[';
        for(unsigned int i = 0; i < b._len; i++) {
            out << (uint32_t) b._data[i];
            if(i < b._len - 1)
                out << ", ";
        }
        out << "]";
        return out;
    }

    uint8_t getSignal() {
    	return _signal;
    }
    void setSignal(uint8_t sig) {
    	_signal = sig;
    }

	void append(uint8_t size)
	{
		uint32_t* n_data = new uint32_t[_len+size];

		for(uint8_t i = 0; i < _len; i++) {
			n_data[i] = 0;
			n_data[i] = _data[i];
		}
		for(uint8_t i = _len; i < _len+size; i++) {
			n_data[i] = 0;
		}

		if(_data) delete _data;
		_data = n_data;
	}
private:



	static tPKAStatus
	exp_mod_start(const uint32_t* pui32Xpoent, const uint32_t ui8XpoentSize,
			const uint32_t* pui32mod, const uint32_t ui8modSize,
			const uint32_t* pui32Base, const uint32_t ui8BaseSize,
			uint32_t* pui32ResultVector)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32Xpoent);
		assert(NULL != pui32mod);
		assert(NULL != pui32Base);
		assert(NULL != pui32ResultVector);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}
		HWREG( (PKA_APTR) ) = offset >> 2;

		for(i = 0; i < ui8XpoentSize; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = *pui32Xpoent;
			pui32Xpoent++;
		}

		offset += 4 * (i + (ui8XpoentSize % 2));

		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8modSize; i++)
		{
			HWREG( (PKA_RAM_BASE + offset + 4*i) ) = *pui32mod;
			pui32mod++;
		}

		offset += 4 * (i + (ui8modSize % 2 + 2));

		HWREG( (PKA_CPTR) ) = offset >> 2;

		for(i = 0; i < ui8BaseSize; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = *pui32Base;
			pui32Base++;
		}

		HWREG(PKA_DPTR) = offset >> 2;

		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_DPTR) ) = offset >> 2;

		HWREG( (PKA_ALENGTH) ) = ui8XpoentSize;
		HWREG( (PKA_BLENGTH) ) = ui8modSize;

		HWREG( (PKA_FUNCTION) ) = (0x0000C000);

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	exp_mod_result(uint32_t* pui32ResultBuf, uint32_t* pui32Len,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert(NULL != pui32Len);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_MSW);

		if(regMSWVal & PKA_MSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

//		cout << "regMSWVal: " << regMSWVal << endl;
//		cout << "PKA_MSW_MSW_ADDRESS_M: " << PKA_MSW_MSW_ADDRESS_M << endl;
//		cout << "ui32ResVectorLoc: " << ui32ResVectorLoc << endl;
//		cout << "PKA_RAM_BASE: " << PKA_RAM_BASE << endl;
//		cout << "((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) - ((ui32ResVectorLoc - PKA_RAM_BASE) >> 2)" << endl;
		len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*pui32Len < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

//		cout << "len real: " << len << endl;
//		cout << "len antigo: " << *pui32Len << endl;

		*pui32Len = len;

		for(i = 0; i < *pui32Len; i++)
		{
			pui32ResultBuf[i]= HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	mod_start(uint32_t* pui32BNum, uint8_t ui8BNSize,
			uint32_t* pui32Modulus, uint8_t ui8ModSize,
			uint32_t* pui32ResultVector)
	{
		uint8_t extraBuf;
		uint32_t offset;
		int i;

		assert(NULL != pui32BNum);
		assert(NULL != pui32Modulus);
		assert(NULL != pui32ResultVector);

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		extraBuf = 2 + ui8ModSize % 2;

		offset = 0;

		HWREG( (PKA_APTR) ) = offset >>2;

		for(i = 0; i < ui8BNSize; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = pui32BNum[i];
		}

		offset += 4 * (i + ui8BNSize % 2);

		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8ModSize; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = pui32Modulus[i];
		}

		offset += 4 * (i + extraBuf);

		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_CPTR) ) = offset >> 2;

		HWREG( (PKA_ALENGTH) ) = ui8BNSize;

		HWREG( (PKA_BLENGTH) ) = ui8ModSize;

		HWREG( (PKA_FUNCTION) ) = (PKA_FUNCTION_RUN | PKA_FUNCTION_MODULO);

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	mod_result(uint32_t* pui32ResultBuf,uint32_t* ui8Size,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_DIVMSW);

		if(regMSWVal & PKA_DIVMSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

		len = ((regMSWVal & PKA_DIVMSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*ui8Size < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

		*ui8Size = len;

		for(i = 0; i < len; i++)
		{
			pui32ResultBuf[i]= HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	cmp_start(uint32_t* pui32BNum1, uint32_t* pui32BNum2, uint8_t ui8Size)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32BNum1);
		assert(NULL != pui32BNum2);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		HWREG( (PKA_APTR) ) = offset >> 2;

		for(i = 0; i < ui8Size; i++)
		{
			HWREG( (PKA_RAM_BASE + offset + 4*i) ) = pui32BNum1[i];
		}

		offset += 4 * (i + ui8Size % 2);

		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8Size; i++)
		{
			HWREG( (PKA_RAM_BASE + offset + 4*i) ) = pui32BNum2[i];
		}

		HWREG( (PKA_ALENGTH) ) = ui8Size;

		HWREG( (PKA_FUNCTION) ) = (PKA_FUNCTION_RUN | PKA_FUNCTION_COMPARE);

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	cmp_result(void)
	{
		tPKAStatus status;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			status = PKA_STATUS_OPERATION_INPRG;
			return (status);
		}

		switch(HWREG(PKA_COMPARE))
		{
			case PKA_COMPARE_A_EQUALS_B:
			status = PKA_STATUS_SUCCESS;
			break;

			case PKA_COMPARE_A_GREATER_THAN_B:
			status = PKA_STATUS_A_GR_B;
			break;

			case PKA_COMPARE_A_LESS_THAN_B:
			status = PKA_STATUS_A_LT_B;
			break;

			default:
			status = PKA_STATUS_FAILURE;
			break;
		}

		return (status);
	}

	static tPKAStatus
	inv_mod_start(uint32_t* pui32BNum, uint8_t ui8BNSize,
			uint32_t* pui32Modulus, uint8_t ui8Size,
			uint32_t* pui32ResultVector)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32BNum);
		assert(NULL != pui32Modulus);
		assert(NULL != pui32ResultVector);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		HWREG( (PKA_APTR) ) = offset >>2;

		for(i = 0; i < ui8BNSize; i++)
		{
			HWREG( (PKA_RAM_BASE + offset + 4*i) ) = pui32BNum[i];
		}

		offset += 4 * (i + ui8BNSize % 2);

		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8Size; i++)
		{
			HWREG( (PKA_RAM_BASE + offset + 4*i) ) = pui32Modulus[i];
		}

		offset += 4 * (i + ui8Size % 2);

		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_DPTR) ) = offset >> 2;

		HWREG( (PKA_ALENGTH) ) = ui8BNSize;
		HWREG( (PKA_BLENGTH) ) = ui8Size;

		HWREG( (PKA_FUNCTION) ) = 0x0000F000;

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	inv_mod_result(uint32_t* pui32ResultBuf, uint32_t* ui8Size,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_MSW);

		if(regMSWVal & PKA_MSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

		len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*ui8Size < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

		*ui8Size = len;

		for(i = 0; i < len; i++)
		{
			pui32ResultBuf[i]= HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	mul_start(uint32_t* pui32Xplicand, uint8_t ui8XplicandSize,
			uint32_t* pui32Xplier, uint8_t ui8XplierSize,
			uint32_t* pui32ResultVector)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32Xplicand);
		assert(NULL != pui32Xplier);
		assert(NULL != pui32ResultVector);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		HWREG( (PKA_APTR) ) = offset >> 2;

		for(i = 0; i < ui8XplicandSize; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = *pui32Xplicand;
			pui32Xplicand++;
		}

		offset += 4 * (i + (ui8XplicandSize % 2));

		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8XplierSize; i++)
		{
			HWREG( (PKA_RAM_BASE + offset + 4*i) ) = *pui32Xplier;
			pui32Xplier++;
		}

		offset += 4 * (i + (ui8XplierSize % 2));

		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_CPTR) ) = offset >> 2;

		HWREG( (PKA_ALENGTH) ) = ui8XplicandSize;
		HWREG( (PKA_BLENGTH) ) = ui8XplierSize;

		HWREG( (PKA_FUNCTION) ) = (PKA_FUNCTION_RUN | PKA_FUNCTION_MULTIPLY);

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	mul_result(uint32_t* pui32ResultBuf, uint32_t* pui32Len,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert(NULL != pui32Len);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_MSW);

		if(regMSWVal & PKA_MSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

		len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*pui32Len < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

		*pui32Len = len;

		for(i = 0; i < *pui32Len; i++)
		{
			pui32ResultBuf[i]= HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	div_start(uint32_t* pui32Dvidend, uint8_t ui8DvidendSize,
			uint32_t* pui32Dvisor, uint8_t ui8DvisorSize,
			uint32_t* pui32Mod, uint8_t ui8ModSize,
			uint32_t* pui32ResultVector)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32Dvidend);
		assert(NULL != pui32Dvisor);
		assert(NULL != pui32ResultVector);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

/////////////////////////
		HWREG( (PKA_APTR) ) = offset >> 2;

		for(i = 0; i < ui8DvidendSize; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = *pui32Dvidend;
			pui32Dvidend++;
		}

		offset += 4 * (i + (ui8DvidendSize % 2));

		/////////////////////////
		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8DvisorSize; i++)
		{
			HWREG( (PKA_RAM_BASE + offset + 4*i) ) = *pui32Dvisor;
			pui32Dvisor++;
		}

		offset += 4 * (i + (ui8DvisorSize % 2));

		/////////////////////////
		HWREG( (PKA_CPTR) ) = offset >> 2;

		for(i = 0; i < ui8ModSize; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = *pui32Mod;
			pui32Mod++;
		}

		offset += 4 * (i + (ui8ModSize % 2));

		/////////////////////////
		HWREG( (PKA_DPTR) ) = offset >> 2;

		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_ALENGTH) ) = ui8DvidendSize;
		HWREG( (PKA_BLENGTH) ) = ui8DvisorSize;


		HWREG( (PKA_FUNCTION) ) = (PKA_FUNCTION_RUN | PKA_FUNCTION_DIVIDE);

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	div_result(uint32_t* pui32ResultBuf, uint32_t* pui32Len,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert(NULL != pui32Len);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_MSW);

		if(regMSWVal & PKA_MSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

		len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*pui32Len < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

		*pui32Len = len;

		for(i = 0; i < len; i++)
		{
			pui32ResultBuf[i]= HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	add_start(uint32_t* pui32BN1, uint8_t ui8BN1Size,
			uint32_t* pui32BN2, uint8_t ui8BN2Size,
			uint32_t* pui32ResultVector)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32BN1);
		assert(NULL != pui32BN2);
		assert(NULL != pui32ResultVector);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		HWREG( (PKA_APTR) ) = offset >> 2;

		for(i = 0; i < ui8BN1Size; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = pui32BN1[i];
		}

		offset += 4 * (i + (ui8BN1Size % 2));

		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8BN2Size; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = pui32BN2[i];
		}

		offset += 4 * (i + (ui8BN2Size % 2));

		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_CPTR) ) = offset >> 2;

		HWREG( (PKA_ALENGTH) ) = ui8BN1Size;
		HWREG( (PKA_BLENGTH) ) = ui8BN2Size;

		HWREG( (PKA_FUNCTION) ) = (PKA_FUNCTION_RUN | PKA_FUNCTION_ADD);

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	add_result(uint32_t* pui32ResultBuf, uint32_t* pui32Len,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert(NULL != pui32Len);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_MSW);

		if(regMSWVal & PKA_MSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

		len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*pui32Len < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

		*pui32Len = len;

		for(i = 0; i < *pui32Len; i++)
		{
			pui32ResultBuf[i] = HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	sub_start(uint32_t* pui32BN1, uint8_t ui8BN1Size,
			uint32_t* pui32BN2, uint8_t ui8BN2Size,
			uint32_t* pui32ResultVector)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32BN1);
		assert(NULL != pui32BN2);
		assert(NULL != pui32ResultVector);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		HWREG( (PKA_APTR) ) = offset >> 2;

		for(i = 0; i < ui8BN1Size; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = pui32BN1[i];
		}

		offset += 4 * (i + (ui8BN1Size % 2));

		HWREG( (PKA_BPTR) ) = offset >> 2;

		for(i = 0; i < ui8BN2Size; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = pui32BN2[i];
		}

		offset += 4 * (i + (ui8BN2Size % 2));

		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_CPTR) ) = offset >> 2;

		HWREG( (PKA_ALENGTH) ) = ui8BN1Size;
		HWREG( (PKA_BLENGTH) ) = ui8BN2Size;

		HWREG( (PKA_FUNCTION) ) = (PKA_FUNCTION_RUN | PKA_FUNCTION_SUBTRACT);

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	sub_result(uint32_t* pui32ResultBuf, uint32_t* pui32Len,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert(NULL != pui32Len);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_MSW);

		if(regMSWVal & PKA_MSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

		len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*pui32Len < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

		*pui32Len = len;

		for(i = 0; i < *pui32Len; i++)
		{
			pui32ResultBuf[i] = HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	shift_start(uint32_t* pui32BN1, uint8_t ui8BN1Size,
			uint32_t ui8ShiftValue, uint8_t ui8ShiftDirection,
			uint32_t* pui32ResultVector)
	{
		uint32_t offset;
		int i;

		assert(NULL != pui32BN1);
		assert(NULL != pui32BN2);
		assert(NULL != pui32ResultVector);

		offset = 0;

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		HWREG( (PKA_APTR) ) = offset >> 2;

		for(i = 0; i < ui8BN1Size; i++)
		{
			HWREG((PKA_RAM_BASE + offset + 4*i)) = pui32BN1[i];
		}

		offset += 4 * (i + (ui8BN1Size % 2));


		*pui32ResultVector = PKA_RAM_BASE + offset;

		HWREG( (PKA_CPTR) ) = offset >> 2;

		HWREG( (PKA_ALENGTH) ) = ui8BN1Size;

		HWREG( PKA_SHIFT ) = ui8ShiftValue;

		uint32_t pkaFunction = PKA_FUNCTION_RSHIFT;
		if(ui8ShiftDirection == 0) {
			pkaFunction = PKA_FUNCTION_LSHIFT;
		}

		HWREG( (PKA_FUNCTION) ) = (PKA_FUNCTION_RUN | pkaFunction);


		return (PKA_STATUS_SUCCESS);
	}

	static tPKAStatus
	shift_result(uint32_t* pui32ResultBuf, uint32_t* pui32Len,
			uint32_t ui32ResVectorLoc)
	{
		uint32_t regMSWVal;
		uint32_t len;
		int i;

		assert(NULL != pui32ResultBuf);
		assert(NULL != pui32Len);
		assert((ui32ResVectorLoc > PKA_RAM_BASE) &&
				(ui32ResVectorLoc < (PKA_RAM_BASE + PKA_RAM_SIZE)));

		if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0)
		{
			return (PKA_STATUS_OPERATION_INPRG);
		}

		regMSWVal = HWREG(PKA_MSW);

		if(regMSWVal & PKA_MSW_RESULT_IS_ZERO)
		{
			return (PKA_STATUS_RESULT_0);
		}

		len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1) -
		((ui32ResVectorLoc - PKA_RAM_BASE) >> 2);

		if(*pui32Len < len)
		{
			return (PKA_STATUS_BUF_UNDERFLOW);
		}

		*pui32Len = len;

		for(i = 0; i < *pui32Len; i++)
		{
			pui32ResultBuf[i] = HWREG( (ui32ResVectorLoc + 4*i) );
		}

		return (PKA_STATUS_SUCCESS);
	}
//
	static int static_init(){
		SysCtrlPeripheralReset(SYS_CTRL_PERIPH_PKA);
		SysCtrlPeripheralReset(SYS_CTRL_PERIPH_PKA);
		SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_PKA);
		IntAltMapEnable();
		return 0;

	}
private:
    uint32_t* _data;
    unsigned int _len;		    //<< Length in bytes

    uint8_t _signal;			//<< 0 if positive
    							//<< 1 if negative
public:
    static HWBignum zero;
    static HWBignum one;
    static int __inithardware;
};

HWBignum HWBignum::zero = HWBignum(0);
HWBignum HWBignum::one = HWBignum(1);
int HWBignum::__inithardware = HWBignum::static_init();


__END_UTIL

#endif /* __hwbignum_h__ */


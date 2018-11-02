#ifndef __ecdsa_h
#define __ecdsa_h

#include <stdint.h>
#include <pka.h>
#include <utility/hwbignum.h>

__BEGIN_UTIL

struct SecP256Info {
    static const uint8_t size = 8;
    static const uint32_t prime[];
    static const uint32_t n[];
    static const uint32_t a[];
    static const uint32_t b[];
    static const uint32_t x[];
    static const uint32_t y[];
};

const uint32_t SecP256Info::prime[] = {
    0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF };
const uint32_t SecP256Info::n[] = {
    0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD,
    0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF };
const uint32_t SecP256Info::a[] = {
    0xFFFFFFFC, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0x00000000, 0x00000001, 0xFFFFFFFF };
const uint32_t SecP256Info::b[] = {
    0x27D2604B, 0x3BCE3C3E, 0xCC53B0F6, 0x651D06B0,
    0x769886BC, 0xB3EBBD55, 0xAA3A93E7, 0x5AC635D8 };
const uint32_t SecP256Info::x[] = {
    0xD898C296, 0xF4A13945, 0x2DEB33A0, 0x77037D81,
    0x63A440F2, 0xF8BCE6E5, 0xE12C4247, 0x6B17D1F2 };
const uint32_t SecP256Info::y[] = {
    0x37BF51F5, 0xCBB64068, 0x6B315ECE, 0x2BCE3357,
    0x7C0F9E16, 0x8EE7EB4A, 0xFE1A7F9B, 0x4FE342E2 };

template<typename Curve>
class ECPoint {

public:
    typedef uint8_t tPKAStatus;
    typedef HWBignum Scalar;

    ECPoint() {
        x = new uint32_t[Curve::size];
        y = new uint32_t[Curve::size];
        for (uint8_t i = 0; i < Curve::size; i++) {
            x[i] = Curve::x[i];
            y[i] = Curve::y[i];
        }
    }

    ECPoint(const ECPoint& other) {
        for (uint8_t i = 0; i < Curve::size; i++) {
            x[i] = other.x[i];
            y[i] = other.y[i];
        }
    }

    ECPoint& operator=(const ECPoint& other) {
        for (uint8_t i = 0; i < Curve::size; i++) {
            x[i] = other.x[i];
            y[i] = other.y[i];
        }
        return *this;
    }

    ECPoint& operator*=(const Scalar& x) {
        uint32_t res_addr = 0;
        ecc_mul_start(x, res_addr);
        tPKAStatus op_status = 0;
        do {
            op_status = ecc_mul_result(res_addr);
        } while (op_status == PKA_STATUS_OPERATION_INPRG);
        return *this;
    }

    ECPoint operator+(const ECPoint& p) {
        uint32_t res_addr = 0;
        ecc_add_start(p, res_addr);
        tPKAStatus op_status = 0;
        ECPoint result;
        do {
            op_status = ecc_add_result(result, res_addr);
        } while (op_status == PKA_STATUS_OPERATION_INPRG);
        return result;
    }

    bool operator==(const ECPoint& other) {
        for (uint8_t i = 0; i < Curve::size; i++) {
            if (x[i] != other.x[i])
                return false;

            // Is this needed???
            if (y[i] != other.y[i])
                return false;
        }
        return true;
    }

    friend OStream& operator<<(OStream& out, const ECPoint& p) {
        out << "x = [";
        for (uint8_t i = 0; i < Curve::size; i++) {
            out << p.x[i];
            out << ", ";
        }
        out << "], y = [";
        for (uint8_t i = 0; i < Curve::size; i++) {
            out << p.y[i];
            out << ", ";
        }
        out << "]";
    }

private:

    tPKAStatus ecc_mul_start(const Scalar& scalar, uint32_t& result_vector) {
        uint8_t extraBuf;
        uint32_t offset;
        int i;

        offset = 0;

        /* Make sure no PKA operation is in progress. */
        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        /* Calculate the extra buffer requirement. */
        extraBuf = 2 + Curve::size % 2;

        /* Update the A ptr with the offset address of the PKA RAM location
         * where the scalar will be stored. */
        HWREG(PKA_APTR) = offset >> 2;

        /* Load the scalar in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = scalar._data[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + (Curve::size % 2));

        /* Update the B ptr with the offset address of the PKA RAM location
         * where the curve parameters will be stored. */
        HWREG(PKA_BPTR) = offset >> 2;

        /* Write curve parameter 'p' as 1st part of vector B immediately
         * following vector A at PKA RAM */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::prime[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extraBuf);

        /* Copy curve parameter 'a' in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::a[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extraBuf);

        /* Copy curve parameter 'b' in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::b[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extraBuf);

        /* Update the C ptr with the offset address of the PKA RAM location
         * where the x, y will be stored. */
        HWREG(PKA_CPTR) = offset >> 2;

        /* Write elliptic curve point.x co-ordinate value. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->x[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extraBuf);

        /* Write elliptic curve point.y co-ordinate value. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->y[i];
        }

        /* Determine the offset for the next data. */
        offset += 4 * (i + extraBuf);

        /* Update the result location. */
        result_vector = PKA_RAM_BASE + offset;

        /* Load D ptr with the result location in PKA RAM. */
        HWREG(PKA_DPTR) = offset >> 2;

        /* Load length registers. */
        HWREG(PKA_ALENGTH) = Curve::size;
        HWREG(PKA_BLENGTH) = Curve::size;

        /* set the PKA function to ECC-MULT and start the operation. */
        HWREG(PKA_FUNCTION) = 0x0000D000;

        return PKA_STATUS_SUCCESS;
    }

    tPKAStatus ecc_mul_result(uint32_t result_vector) {
        int i;
        uint32_t addr;
        uint32_t regMSWVal;
        uint32_t len;

        /* Check for the arguments. */
        //assert(result_vector > PKA_RAM_BASE);
        //assert(result_vector < (PKA_RAM_BASE + PKA_RAM_SIZE));

        /* Verify that the operation is completed. */
        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        if(HWREG(PKA_SHIFT) == 0x00000000) {
            /* Get the MSW register value. */
            regMSWVal = HWREG(PKA_MSW);

            /* Check to make sure that the result vector is not all zeroes. */
            if(regMSWVal & PKA_MSW_RESULT_IS_ZERO) {
                return PKA_STATUS_RESULT_0;
            }

            /* Get the length of the result */
            len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1)
                - ((result_vector - PKA_RAM_BASE) >> 2);

            addr = result_vector;

            /* copy the x co-ordinate value of the result from vector D into
             * the \e ec_point. */
            for(i = 0; i < len; i++) {
                this->x[i] = HWREG(addr + 4 * i);
            }

            addr += 4 * (i + 2 + len % 2);

            /* copy the y co-ordinate value of the result from vector D into
             * the \e ec_point. */
            for(i = 0; i < len; i++) {
                this->y[i] = HWREG(addr + 4 * i);
            }

            return PKA_STATUS_SUCCESS;
        } else {
            return PKA_STATUS_FAILURE;
        }
    }

    tPKAStatus ecc_add_start(const ECPoint& p, uint32_t& result_vector) {
        uint8_t extraBuf;
        uint32_t offset;
        int i;

        offset = 0;

        /* Make sure no operation is in progress. */
        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        /* Calculate the extra buffer requirement. */
        extraBuf = 2 + Curve::size % 2;

        /* Update the A ptr with the offset address of the PKA RAM location
         * where the first ecPt will be stored. */
        HWREG(PKA_APTR) = offset >> 2;

        /* Load the x co-ordinate value of the first EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->x[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extraBuf);

        /* Load the y co-ordinate value of the first EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = this->y[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extraBuf);

        /* Update the B ptr with the offset address of the PKA RAM location
         * where the curve parameters will be stored. */
        HWREG(PKA_BPTR) = offset >> 2;

        /* Write curve parameter 'p' as 1st part of vector B */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::prime[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extraBuf);

        /* Write curve parameter 'a'. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = (uint32_t)Curve::a[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extraBuf);

        /* Update the C ptr with the offset address of the PKA RAM location
         * where the ecPt2 will be stored. */
        HWREG(PKA_CPTR) = offset >> 2;

        /* Load the x co-ordinate value of the second EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = p.x[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extraBuf);

        /* Load the y co-ordinate value of the second EC point in PKA RAM. */
        for(i = 0; i < Curve::size; i++) {
            HWREG(PKA_RAM_BASE + offset + 4 * i) = p.y[i];
        }

        /* Determine the offset in PKA RAM for the next data. */
        offset += 4 * (i + extraBuf);

        /* Copy the result vector location. */
        result_vector = PKA_RAM_BASE + offset;

        /* Load D ptr with the result location in PKA RAM. */
        HWREG(PKA_DPTR) = offset >> 2;

        /* Load length registers. */
        HWREG(PKA_BLENGTH) = Curve::size;

        /* Set the PKA Function to ECC-ADD and start the operation. */
        HWREG(PKA_FUNCTION) = 0x0000B000;

        return PKA_STATUS_SUCCESS;
    }

    tPKAStatus ecc_add_result(ECPoint& result, uint32_t result_vector) {
        uint32_t regMSWVal;
        uint32_t addr;
        int i;
        uint32_t len;

        /* Check for the arguments. */
        //ASSERT(result_vector > PKA_RAM_BASE);
        //ASSERT(result_vector < (PKA_RAM_BASE + PKA_RAM_SIZE));

        if((HWREG(PKA_FUNCTION) & PKA_FUNCTION_RUN) != 0) {
            return PKA_STATUS_OPERATION_INPRG;
        }

        if(HWREG(PKA_SHIFT) == 0x00000000) {
            /* Get the MSW register value. */
            regMSWVal = HWREG(PKA_MSW);

            /* Check to make sure that the result vector is not all zeroes. */
            if(regMSWVal & PKA_MSW_RESULT_IS_ZERO) {
                return PKA_STATUS_RESULT_0;
            }

            /* Get the length of the result. */
            len = ((regMSWVal & PKA_MSW_MSW_ADDRESS_M) + 1)
                - ((result_vector - PKA_RAM_BASE) >> 2);

            addr = result_vector;

            /* Copy the x co-ordinate value of result from vector D into the
             * the output EC Point. */
            for(i = 0; i < len; i++) {
                result.x[i] = HWREG(addr + 4 * i);
            }

            addr += 4 * (i + 2 + len % 2);

            /* Copy the y co-ordinate value of result from vector D into the
             * the output EC Point. */
            for(i = 0; i < len; i++) {
                result.y[i] = HWREG(addr + 4 * i);
            }

            return PKA_STATUS_SUCCESS;
        } else {
            return PKA_STATUS_FAILURE;
        }
    }

    uint32_t *x;
    uint32_t *y;

};

__END_UTIL

#endif

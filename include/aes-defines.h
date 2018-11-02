#ifndef __AESDEFINES_H__
#define __AESDEFINES_H__

#include "hw_types.h"

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************

#ifdef __cplusplus
extern "C"
{
#endif

#define NULL 0

//*****************************************************************************
//
//  General constants
//
//*****************************************************************************

// AES and SHA256 module return codes
#define AES_SUCCESS                   0
#define SHA256_SUCCESS                0
#define AES_KEYSTORE_READ_ERROR       1
#define AES_KEYSTORE_WRITE_ERROR      2
#define AES_DMA_BUS_ERROR             3
#define CCM_AUTHENTICATION_FAILED     4
#define SHA2_ERROR                    5
#define SHA256_INVALID_PARAM          6
#define SHA256_TEST_ERROR             7
#define AES_ECB_TEST_ERROR            8
#define AES_NULL_ERROR                9
#define SHA256_NULL_ERROR             9
#define AES_CCM_TEST_ERROR            10


//*****************************************************************************
//
//For 128 bit key all 8 Key Area locations from 0 to 8 are valid
//However for 192 bit and 256 bit keys, only even Key Areas 0, 2, 4, 6
//are valid. This is passes as a parameter to AesECBStart()
//
//*****************************************************************************

// Current AES operation
enum
{
    AES_NONE,
    AES_KEYL0AD,
    AES_ECB,
    AES_CCM,
    AES_SHA256,
    AES_RNG
};

#ifdef __cplusplus
}
#endif

#endif  // __AES_H__

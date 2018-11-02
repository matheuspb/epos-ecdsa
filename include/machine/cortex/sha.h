// EPOS ARM Cortex SHA Mediator 	

#ifndef __cortex_sha_h
#define __cortex_sha_h

#include <sha.h>
#include <utility/sha.h>

#include <utility/ostream.h>

#include <sha-init.h>
#include <sha-int-cfg.h>
#include <hw_types.h>
#include <cc-aes-defines.h>
#include <aes-defines.h>

__BEGIN_SYS

#define ASM_NOP asm("NOP")
#define MIN(n,m) (((n) < (m)) ? (n) : (m))

class SHA_Cortex: public SHA_Common
{

private:
	
	volatile uint8_t g_ui8CurrentAESOp;
	static const uint8_t SHA256_BLOCK_SIZE = 64;
	static const uint8_t SHA256_OUTPUT_LEN = 32;
	static const uint8_t HASH_SHA256_MAX_BLOCK_LEN = 64;
	static const uint8_t ROUND_H_ENTRY = 8;

	typedef struct 
	{
	    uint64_t length;   
	    uint32_t state[ROUND_H_ENTRY]; 
	    uint32_t curlen;    
	    uint8_t  buf[SHA256_BLOCK_SIZE];  
	    uint8_t  new_digest;
	    uint8_t  final_digest;

	} tSHA256State;

	tSHA256State* sha_struct;
	uint8_t  hash[SHA256_OUTPUT_LEN];
	
	uint8_t* entry;
	uint32_t entry_len;

public:
	
	SHA_Cortex(uint8_t *data, uint32_t len) 
	{
		g_ui8CurrentAESOp = AES_NONE;

		entry = data;
		entry_len = len;
		
		sha_struct = new tSHA256State;
		sha_struct->curlen = 0;
	    sha_struct->length = 0;
	    sha_struct->new_digest = true;
	    sha_struct->final_digest = false;
	};

	uint8_t* SHACompute() 
	{
		OStream cout;
		cout << "Computing hash using SHA256 with hardware acceleration.\n" << endl;
		
		SysCtrlPeripheralReset(SYS_CTRL_PERIPH_AES);
    	SysCtrlPeripheralEnable(SYS_CTRL_PERIPH_AES);
	    
	    IntAltMapEnable();

		if(SHA256Process() == SHA256_SUCCESS)
			if(SHA256Done() == SHA256_SUCCESS)
				return hash;

		return NULL;
	}	

private:

	//*****************************************************************************
	//
	//! SHA256Process processes a block of memory through the hash.
	//!
	//! \return  SHA256_SUCCESS if successful.
	//
	//*****************************************************************************
	
	uint8_t SHA256Process()
	{
		uint8_t  ui8Err;
	    uint32_t ui32N, ui32I;

	    if(sha_struct->curlen > sizeof(sha_struct->buf))
	    {
	        return (SHA256_INVALID_PARAM);
	    }

	    g_ui8CurrentAESOp = AES_SHA256;

	    if(entry_len > 0 && sha_struct->new_digest == true)
	    {
	        if(sha_struct->curlen == 0 && entry_len > SHA256_BLOCK_SIZE)
	        {
	            for(ui32I = 0; ui32I < SHA256_BLOCK_SIZE; ui32I++)
	            {
	                sha_struct->buf[sha_struct->curlen + ui32I] = entry[ui32I];
	            }

	            if((ui8Err = SHA256HashNew()) != SHA256_SUCCESS)
	            {
	                g_ui8CurrentAESOp = AES_NONE;
	                return (ui8Err);
	            }

	            sha_struct->new_digest = false;
	            sha_struct->length += SHA256_BLOCK_SIZE * 8;
	            entry_len -= SHA256_BLOCK_SIZE;
	            entry += SHA256_BLOCK_SIZE;

	        }
	        else
	        {
	            ui32N = MIN(entry_len, (SHA256_BLOCK_SIZE - sha_struct->curlen));
	            
	            for(ui32I = 0; ui32I < ui32N; ui32I++)
	            {
	                sha_struct->buf[sha_struct->curlen + ui32I] = entry[ui32I];
	            }

	            sha_struct->curlen += ui32N;
	            entry += ui32N;
	            entry_len -= ui32N;

	            if(sha_struct->curlen == SHA256_BLOCK_SIZE && entry_len > 0)
	            {
	                if((ui8Err = SHA256HashNew()) != SHA256_SUCCESS)
	                {
	                    g_ui8CurrentAESOp = AES_NONE;
	                    return (ui8Err);
	                }
	                sha_struct->new_digest = false;
	                sha_struct->length += 8 * SHA256_BLOCK_SIZE;
	                sha_struct->curlen = 0;
	            }
	        }
	    }

	    while(entry_len > 0 && sha_struct->new_digest == false)
	    {
	        if(sha_struct->curlen == 0 && entry_len > SHA256_BLOCK_SIZE)
	        {
	            for(ui32I = 0; ui32I < SHA256_BLOCK_SIZE; ui32I++)
	            {
	                sha_struct->buf[sha_struct->curlen + ui32I] = entry[ui32I];
	            }
	            if((ui8Err = SHA256HashResume()) != SHA256_SUCCESS)
	            {
	                g_ui8CurrentAESOp = AES_NONE;
	                return (ui8Err);
	            }
	            sha_struct->length += SHA256_BLOCK_SIZE * 8;
	            entry += SHA256_BLOCK_SIZE;
	            entry_len -= SHA256_BLOCK_SIZE;
	        }
	        else
	        {
	            ui32N = MIN(entry_len, (SHA256_BLOCK_SIZE - sha_struct->curlen));
	            for(ui32I = 0; ui32I < ui32N; ui32I++)
	            {
	                sha_struct->buf[sha_struct->curlen + ui32I] = entry[ui32I];
	            }
	            sha_struct->curlen += ui32N;
	            entry  += ui32N;
	            entry_len -= ui32N;
	            if(sha_struct->curlen == SHA256_BLOCK_SIZE && entry_len > 0)
	            {
	                if((ui8Err = SHA256HashResume()) != SHA256_SUCCESS)
	                {
	                    g_ui8CurrentAESOp = AES_NONE;
	                    return (ui8Err);
	                }
	                sha_struct->length += 8 * SHA256_BLOCK_SIZE;
	                sha_struct->curlen = 0;
	            }
	        }
	    }
	    
	    g_ui8CurrentAESOp = AES_NONE;
	    return (SHA256_SUCCESS);
	};

	//*****************************************************************************
	//
	//! SHA256HashNew function is to start a new Hash session in hardware.
	//!
	//! \return  SHA256_SUCCESS if successful.
	//
	//*****************************************************************************
	uint8_t SHA256HashNew()
	{
		uint8_t *ui8Out;
		if(sha_struct->final_digest)
			ui8Out = hash;
		else
			ui8Out =  (uint8_t *)sha_struct->state;

	    // workaround for AES registers not retained after PM2
	    // IntDisable(INT_AES);
		HWREG(g_pui32DisRegs[(INT_AES - 16) / 32]) =
		            1 << ((INT_AES - 16) & 31);

	    HWREG(AES_CTRL_INT_CFG) = AES_CTRL_INT_CFG_LEVEL;
	    HWREG(AES_CTRL_INT_EN)  = (AES_CTRL_INT_EN_RESULT_AV |
	                               AES_CTRL_INT_EN_DMA_IN_DONE);

	    // configure master control module
	    // enable DMA path to the SHA-256 engine + Digest readout
	    HWREG(AES_CTRL_ALG_SEL) = (AES_CTRL_ALG_SEL_TAG | AES_CTRL_ALG_SEL_HASH);
	    // clear any outstanding events
	    HWREG(AES_CTRL_INT_CLR) =  AES_CTRL_INT_CLR_RESULT_AV;

	    // configure hash engine
	    // indicate start of a new hash session and SHA256
	    HWREG(AES_HASH_MODE_IN) = (AES_HASH_MODE_IN_SHA256_MODE |
	                               AES_HASH_MODE_IN_NEW_HASH);

	    // if the final digest is required (pad the input DMA data),
	    // write the following register
	    //
	    if(sha_struct->final_digest)
	    {
	        // write length of the message (lo)
	        HWREG(AES_HASH_LENGTH_IN_L) = (uint32_t)sha_struct->length;
	        // write length of the message (hi)
	        HWREG(AES_HASH_LENGTH_IN_H) = (uint32_t)(sha_struct->length >> 16);
	        // pad the DMA-ed data
	        HWREG(AES_HASH_IO_BUF_CTRL) = AES_HASH_IO_BUF_CTRL_PAD_DMA_MESSAGE;
	    }

	    // enable DMA channel 0 for message data
	    HWREG(AES_DMAC_CH0_CTRL) |= AES_DMAC_CH0_CTRL_EN;
	    // base address of the data in ext. memory
	    HWREG(AES_DMAC_CH0_EXTADDR) = (uint32_t)sha_struct->buf;
	    
	    if(sha_struct->final_digest)
	    {
	        // input data length in bytes, equal to the message
	        HWREG(AES_DMAC_CH0_DMALENGTH) = sha_struct->curlen;
	    }
	    else
	    {
	        HWREG(AES_DMAC_CH0_DMALENGTH) = SHA256_BLOCK_SIZE;
	    }

	    // enable DMA channel 1 for result digest
	    HWREG(AES_DMAC_CH1_CTRL) |= AES_DMAC_CH1_CTRL_EN;
	    // base address of the digest buffer
	    HWREG(AES_DMAC_CH1_EXTADDR) = (uint32_t)ui8Out;
	    // length of the result digest
	    HWREG(AES_DMAC_CH1_DMALENGTH) = SHA256_OUTPUT_LEN;

	    // wait for completion of the operation
	    do
	    {
	        ASM_NOP;
	    }
	    
	    while(!(HWREG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_RESULT_AV));


	    if((HWREG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_DMA_BUS_ERR))
	    {
	        return (AES_DMA_BUS_ERROR);
	    }

	    // clear the interrupt
	    HWREG(AES_CTRL_INT_CLR) = (AES_CTRL_INT_CLR_DMA_IN_DONE |
	                               AES_CTRL_INT_CLR_RESULT_AV);
	    // disable master control/DMA clock
	    HWREG(AES_CTRL_ALG_SEL) = 0x00000000;
	    // clear mode
	    HWREG(AES_AES_CTRL) = 0x00000000;

	    return (SHA256_SUCCESS);
	}

	//*****************************************************************************
	//
	//! SHA256HashResume function resumes an already started hash session in hardware.
	//!
	//! \return  SHA256_SUCCESS if successful.
	//
	//*****************************************************************************
	uint8_t SHA256HashResume()
	{
		uint8_t *ui8Out;
		if(sha_struct->final_digest)
			ui8Out = hash;
		else
			ui8Out =  (uint8_t *)sha_struct->state;
	    
	    // IntDisable(INT_AES);
	    //
	    // Disable the general interrupt.
	    //
	    HWREG(g_pui32DisRegs[(INT_AES - 16) / 32]) =
	        1 << ((INT_AES - 16) & 31);

	    // workaround for AES registers not retained after PM2
	    HWREG(AES_CTRL_INT_CFG) = AES_CTRL_INT_CFG_LEVEL;
	    HWREG(AES_CTRL_INT_EN)  = (AES_CTRL_INT_EN_RESULT_AV |
	                               AES_CTRL_INT_EN_DMA_IN_DONE);

	    // configure master control module and enable
	    // the DMA path to the SHA-256 engine
	    //
	    HWREG(AES_CTRL_ALG_SEL) = AES_CTRL_ALG_SEL_HASH;

	    // clear any outstanding events
	    HWREG(AES_CTRL_INT_CLR) =  AES_CTRL_INT_CLR_RESULT_AV;

	    // configure hash engine
	    // indicate the start of a resumed hash session and SHA256
	    HWREG(AES_HASH_MODE_IN) = AES_HASH_MODE_IN_SHA256_MODE;

	    // if the final digest is required (pad the input DMA data)
	    if(sha_struct->final_digest)
	    {
	        // write length of the message (lo)
	        HWREG(AES_HASH_LENGTH_IN_L) = (uint32_t)sha_struct->length;
	        // write length of the message (hi)
	        HWREG(AES_HASH_LENGTH_IN_H) = (uint32_t)(sha_struct->length >> 16);
	    }

	    // write the initial digest
	    HWREG(AES_HASH_DIGEST_A) = (uint32_t)sha_struct->state[0];
	    HWREG(AES_HASH_DIGEST_B) = (uint32_t)sha_struct->state[1];
	    HWREG(AES_HASH_DIGEST_C) = (uint32_t)sha_struct->state[2];
	    HWREG(AES_HASH_DIGEST_D) = (uint32_t)sha_struct->state[3];
	    HWREG(AES_HASH_DIGEST_E) = (uint32_t)sha_struct->state[4];
	    HWREG(AES_HASH_DIGEST_F) = (uint32_t)sha_struct->state[5];
	    HWREG(AES_HASH_DIGEST_G) = (uint32_t)sha_struct->state[6];
	    HWREG(AES_HASH_DIGEST_H) = (uint32_t)sha_struct->state[7];

	    // If final digest, pad the DMA-ed data
	    if(sha_struct->final_digest)
	    {
	        HWREG(AES_HASH_IO_BUF_CTRL) = AES_HASH_IO_BUF_CTRL_PAD_DMA_MESSAGE;
	    }

	    // enable DMA channel 0 for message data
	    HWREG(AES_DMAC_CH0_CTRL) |= AES_DMAC_CH0_CTRL_EN;
	    // base address of the data in ext. memory
	    HWREG(AES_DMAC_CH0_EXTADDR) = (uint32_t)sha_struct->buf;
	    // input data length in bytes, equal to the message
	    if(sha_struct->final_digest)
	    {
	        HWREG(AES_DMAC_CH0_DMALENGTH) = sha_struct->curlen;
	    }
	    else
	    {
	        HWREG(AES_DMAC_CH0_DMALENGTH) = SHA256_BLOCK_SIZE;
	    }

	    // wait for completion of the operation
	    do
	    {
	        ASM_NOP;
	    }
	    while(!(HWREG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_RESULT_AV));

	    // check for any DMA Bus errors
	    if((HWREG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_DMA_BUS_ERR))
	    {
	        return (AES_DMA_BUS_ERROR);
	    }

	    // read digest
	    ((uint32_t  *)ui8Out)[0] = HWREG(AES_HASH_DIGEST_A);
	    ((uint32_t  *)ui8Out)[1] = HWREG(AES_HASH_DIGEST_B);
	    ((uint32_t  *)ui8Out)[2] = HWREG(AES_HASH_DIGEST_C);
	    ((uint32_t  *)ui8Out)[3] = HWREG(AES_HASH_DIGEST_D);
	    ((uint32_t  *)ui8Out)[4] = HWREG(AES_HASH_DIGEST_E);
	    ((uint32_t  *)ui8Out)[5] = HWREG(AES_HASH_DIGEST_F);
	    ((uint32_t  *)ui8Out)[6] = HWREG(AES_HASH_DIGEST_G);
	    ((uint32_t  *)ui8Out)[7] = HWREG(AES_HASH_DIGEST_H);

	    // acknowledge reading of the digest
	    HWREG(AES_HASH_IO_BUF_CTRL) = AES_HASH_IO_BUF_CTRL_OUTPUT_FULL;

	    // clear the interrupt
	    HWREG(AES_CTRL_INT_CLR) = (AES_CTRL_INT_CLR_DMA_IN_DONE |
	                               AES_CTRL_INT_CLR_RESULT_AV);
	    // acknowledge result and clear interrupts
	    // disable master control/DMA clock
	    HWREG(AES_CTRL_ALG_SEL) = 0x00000000;
	    // clear mode
	    HWREG(AES_AES_CTRL) = 0x00000000;

	    return (SHA256_SUCCESS);
	}

	//*****************************************************************************
	//
	//! SHA256Done function terminates hash session to get the digest.
	//!
	//! \return  SHA256_SUCCESS if successful.
	//
	//*****************************************************************************
	uint8_t SHA256Done()
	{
	    uint8_t ui8Err;

	    if(sha_struct->curlen > sizeof(sha_struct->buf))
	    {
	        return (SHA256_INVALID_PARAM);
	    }

	    g_ui8CurrentAESOp = AES_SHA256;

	    sha_struct->length += sha_struct->curlen * 8;
	    sha_struct->final_digest = true;

	    if(sha_struct->new_digest == true)
	    {
	        if((ui8Err = SHA256HashNew()) != SHA256_SUCCESS)
	        {
	            g_ui8CurrentAESOp = AES_NONE;
	            return (ui8Err);
	        }
	    }
	    else
	    {
	        if((ui8Err = SHA256HashResume()) != SHA256_SUCCESS)
	        {
	            g_ui8CurrentAESOp = AES_NONE;
	            return (ui8Err);
	        }
	    }

	    sha_struct->new_digest = false;
	    sha_struct->final_digest = false;

	    g_ui8CurrentAESOp = AES_NONE;
	    
	    return (SHA256_SUCCESS);
	}

};

class SHA: public IF<Traits<Build>::MODEL == Traits<Build>::eMote3, SHA_Cortex, SHA_Utility>::Result
{

private:
    typedef IF<Traits<Build>::MODEL == Traits<Build>::eMote3, SHA_Cortex, SHA_Utility>::Result Engine;

public:
    SHA(uint8_t *data, uint32_t len)
    : Engine(data, len)
    {

    }

};

__END_SYS

#endif /*__cortex_sha_h*/

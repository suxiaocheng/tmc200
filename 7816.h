#ifndef __7816_H__
#define __7816_H__

#include "debug.h"

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;

struct atr_t
{
	uint8_t atr_len;
	uint8_t *atr;
};

#define init_7816(val)
#define tx_enable()
#define dummy_receive()
#define ke_schedule()
#define delay_etu()

uint32_t get_apdu_length(void);
uint8_t *get_apdu(void);
void set_apdu_buf(uint8_t *buf, uint32_t count);
uint8_t CARDreset(void);
uint16_t trans_t0(void);

#endif


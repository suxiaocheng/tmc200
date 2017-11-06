#include "uart.h"
#include "7816.h"

struct atr_t atr;
uint8_t apdu[320];// qn9020 to cpucard  (head+apdu)   /*5+255*/
uint16_t apdu_length;
uint8_t STATE_FLAG;
uint8_t TC1,WI,T0_T1_FLAG;
uint8_t BRDI;

uint32_t get_apdu_length(void)
{
	return apdu_length;
}

uint8_t *get_apdu(void)
{
	return apdu;
}

void set_apdu_buf(uint8_t *buf, uint32_t count)
{
	uint32_t i;
	for (i=0; i<count; i++) {
		apdu[i] = buf[i];
	}
}

uint8_t CARDrecvbyte(void)
{
	uint8_t ret;
	uint8_t data;
	STATE_FLAG = 0;
	ret = uart_read_bytes(&data, 1, TIMEOUT_CHAR);
	if (ret != 1) {
		STATE_FLAG = 1;
	}
	
	return data;
}

void CARDsendbyte(uint8_t data)
{
	uint8_t ret;
	uint8_t tmp;
	STATE_FLAG = 0;
	ret = uart_send_bytes(&data, 1);
	if (ret != 1) {
		STATE_FLAG = 1;
	}

	ret = uart_read_bytes(&tmp, 1, TIMEOUT_CHAR);
	if (ret != 1) {
		STATE_FLAG = 1;
	}

	if (tmp != data) {
		STATE_FLAG = 1;
	}
}

#if 1
uint8_t atr_info[32];
uint8_t CARDreset(void)
{
	uint8_t kk,answerBcc,len,temp;
	uint8_t TD,TD2 = 0;
	uint32_t ret;
	
	TC1 = 0;
//	TA1 = 0;
	WI = 10;
	kk = 1;
	T0_T1_FLAG = 0;
	len = 2;

	atr.atr = atr_info;
	
	ret = reset_config_status(0);
	if (ret != 0) {
		return ret;
	}
	
	/* Wait at least 400/f, at least 114us@3.5MHz */
	usleep(10000);
	
	flush_uart();
	
	ret = reset_config_status(1);
	if (ret != 0) {
		return ret;
	}

	temp = CARDrecvbyte();
	if(STATE_FLAG)
		return 0;

	if(temp == 0x3b)
		atr.atr[0] = 0x3b;
	else if(temp == 0x3)
		atr.atr[0] = 0x3f;
	else
		return 0;
	
	TD = CARDrecvbyte();
	if(STATE_FLAG)
		return 0;
	answerBcc = TD;
	atr.atr[1] = TD;
	apdu_length = TD&0x0f;

	while(1)
	{
		if(TD&0x10)
		{
			temp = CARDrecvbyte();
			if(STATE_FLAG)
				return 0;

//			if(kk == 1)       //TA1
//				TA1 = temp;

//			if(kk == 2)
//				TA2 = temp;

			atr.atr[len] = temp;
			answerBcc ^= temp;
			len += 1;
		}

		if(TD&0x20)
		{
			temp = CARDrecvbyte();
			if(STATE_FLAG)
				return 0;

			answerBcc ^= temp;
			atr.atr[len] = temp;
			len += 1;
		}

		if(TD&0x40)
		{
			temp = CARDrecvbyte();
			if(STATE_FLAG)
				return 0;

			if(kk == 1) TC1 = temp;if(temp == 0xff) TC1 =0;
			else if(kk == 2) WI = temp;//TC2

			atr.atr[len] = temp;
			answerBcc ^= temp;
			len += 1;
		}

		if(TD&0x80)
		{
			TD = CARDrecvbyte();
			if(STATE_FLAG)
				return 0;

			answerBcc ^= TD;
			atr.atr[len] = TD;

			if((TD&0x0f) == 0x00) T0_T1_FLAG = 0;
			else if((TD&0x0f) == 0x01) T0_T1_FLAG = 1;

			len += 1;

			if((TD&0xf0) == 0x00) break;
			if(kk == 2) TD2 = TD&0xf;

			kk += 1;

		}
		else
			break;
	}

	while(apdu_length)
	{
		temp = CARDrecvbyte();
		if(STATE_FLAG)
			return 0;

		answerBcc ^= temp;
		atr.atr[len] = temp;
		len += 1;
		apdu_length -= 1;
	}

	if(TD2 & 0xf)		// T = 15 receive TCK
	{
		temp = CARDrecvbyte();
		if(STATE_FLAG)
			return 0;

		answerBcc ^= temp;
		if(answerBcc)
			return 0;

		atr.atr[len] = temp;
		len += 1;
	}
	
#if 0//!GPIO_7816
	if(!((TA1 == 0x0)||(TA1 == 0x01)||(TA1 == 0x11)||(fetch_Fi(TA1)==0)||(fetch_Di(TA1)==0)))
		set_bps(TA1);
#endif
	
	return len;
}



// 0 : error
uint16_t trans_t0(void)
{
	uint16_t len;
	uint8_t temp,case3,ins,pp3,sw1,sw2;
	T0_T1_FLAG = 0;
	init_7816(0);
	
#if !GPIO_7816	
	tx_enable();
#endif
	
	ins = apdu[1];
	pp3 = apdu[4];

	if(apdu_length <= 5) 	case3 = 0;
	else 		case3 = 1;

	if(apdu_length < 5) 	pp3 = 0;

	//send APDU header
	for(len = 0; len < 5; len++)
	{
		temp = apdu[len];
		CARDsendbyte(temp);
	}
	dummy_receive();

	len = 5;
	
	//receive sw1
	while(1)
	{
		ke_schedule();
		sw1 = CARDrecvbyte();
		if(STATE_FLAG)
			return 0;
#if 0			
#if __USART_CLK == CLK_16M	
		if( (BRDI != 0x5D) && (sw1 == 0xff))  // 9600bps
#else // 8MHz
		if( (BRDI != 0x2E) && (sw1 == 0xff))  // 9600bps
#endif		
			continue;
#endif		
		if(sw1 != 0x60) break;
	}

	//sw1 != ins
	if((sw1 != ins)&&((sw1+ins) != 0xff))
	{
		sw2 = CARDrecvbyte();
		if(STATE_FLAG)
			return 0;
		apdu[0] = sw1;
		apdu[1] =sw2;
		apdu_length = 2;
		return 2;
	}

//sw1 == ins
//case3
	if(case3)
	{
		delay_etu();
		delay_etu();
		delay_etu();
		delay_etu();

		while((sw1+ins) == 0xff)
		{
	tx_enable();
			temp = apdu[len];
			len ++;
			CARDsendbyte(temp);
	dummy_receive();

			sw1 = CARDrecvbyte();
			if (STATE_FLAG)
				return 0;
			if(--pp3 == 0) break;
		}

		if(sw1 == ins)
		{
	tx_enable();
			while(pp3)
			{
				temp = apdu[len];
				CARDsendbyte(temp);
				len++;
				pp3--;
			}
	dummy_receive();
	  		while(1)
  			{
					ke_schedule();
					sw1 = CARDrecvbyte();
					if(STATE_FLAG)
						return 0;
#if 0						
#if __USART_CLK == CLK_16M	
					if( (BRDI != 0x5D) && (sw1 == 0xff))  // 9600bps
#else // 8MHz
					if( (BRDI != 0x2E) && (sw1 == 0xff))  // 9600bps
#endif		
						continue;
#endif
					if(sw1 != 0x60) break;
   			}
		}

		sw2 = CARDrecvbyte();
		if(STATE_FLAG)
			return 0;

		apdu[0] = sw1;
		apdu[1] = sw2;
		apdu_length = 2;
		return 2;
	}

//case1,2
	apdu_length = 0;
	do
	{
		temp = CARDrecvbyte();
		if(STATE_FLAG)
			return 0;
		apdu[apdu_length] = temp;
		apdu_length++;
		pp3--;
	}while(pp3);

	sw1 = CARDrecvbyte();
	if(STATE_FLAG)
		return 0;
	sw2 = CARDrecvbyte();
	if(STATE_FLAG)
		return 0;

	apdu[apdu_length] = sw1;
	apdu_length++;
	apdu[apdu_length] = sw2;
	apdu_length ++;
	return apdu_length;
}
#endif


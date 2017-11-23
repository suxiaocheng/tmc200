#ifndef __UART_H__
#define __UART_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "debug.h"

#define DEBUG_UART_IO

/* timeout unit: 100us */
#define TIMEOUT_START	200
#define TIMEOUT_CHAR	20

typedef struct{
	int 		type;	// 0: receive, 1: send
	unsigned char 	dat[64]; // storage max 64 bytes data
	unsigned int	valid_len;
	unsigned int	len;
	struct timeval	timestamp;
}debug_log_buf;

void print_uart_log(void);
int init_uart(char *name);
int close_uart(void);
int test_uart(void);
void flush_uart(void);
int uart_send_bytes(char *buf, int count);
int uart_read_bytes(char *buf, int count, int timeout);

#endif

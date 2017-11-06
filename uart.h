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

#include "debug.h"

/* timeout unit: 100us */
#define TIMEOUT_START	200
#define TIMEOUT_CHAR	20

int init_uart(char *name);
int close_uart(void);
int test_uart(void);
void flush_uart(void);
int uart_send_bytes(char *buf, int count);
int uart_read_bytes(char *buf, int count, int timeout);

#endif
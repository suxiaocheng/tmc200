#include "uart.h"

static int uart_fd;

#ifdef DEBUG_UART_IO
debug_log_buf uart_log_buf[1000];
unsigned int uart_log_len = 0;

static int uart_log(unsigned char *buf, int count, int type)
{
	int copy_count = count;
	if (uart_log_len >= sizeof(uart_log_buf)/sizeof(debug_log_buf)) {
		return 1;
	}
	if (count == 0) {
		return 0;
	}
	if (copy_count > sizeof(uart_log_buf[uart_log_len].dat)) {
		copy_count = sizeof(uart_log_buf[uart_log_len].dat);
	}
	memcpy(uart_log_buf[uart_log_len].dat, buf, copy_count);

	gettimeofday(&(uart_log_buf[uart_log_len].timestamp), NULL);

	uart_log_buf[uart_log_len].type = type;
	uart_log_buf[uart_log_len].len = count;
	uart_log_buf[uart_log_len].valid_len = copy_count;

	uart_log_len++;
	return 0;
}

void print_uart_log(void)
{
	unsigned int i;
	for (i=0; i<uart_log_len; i++) {
		printf("%sTimestamp[%ld.%ld]: len[%d/%d]:\n",
			(uart_log_buf[i].type == 0)?"<--":"-->",
			uart_log_buf[i].timestamp.tv_sec,
			uart_log_buf[i].timestamp.tv_usec,
			uart_log_buf[i].valid_len,
			uart_log_buf[i].len);
		dump_memory(uart_log_buf[i].dat, uart_log_buf[i].valid_len);
	}
}
#else

__inline static int uart_log(unsigned char *buf, int count, int type)
{
}

void print_uart_log(void)
{
}

#endif

void set_speed(int speed)
{
	unsigned int i;
	int status;
	struct termios Opt;

	int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	                   B38400, B19200, B9600, B4800, B2400, B1200, B300,
	                  };
	int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300, 38400,
	                  19200, 9600, 4800, 2400, 1200, 300,
	                 };
	tcgetattr(uart_fd, &Opt);
	for (i= 0; i < sizeof(speed_arr)/sizeof(int); i++) {
		if (speed == name_arr[i]) {
			tcflush(uart_fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(uart_fd, TCSANOW, &Opt);
			if (status != 0) {
				printf("tcsetattr g_fd1");
				return;
			}
			tcflush(uart_fd,TCIOFLUSH);
		}
	}
}

int set_Parity(int databits, int stopbits, int parity)
{
	struct termios options;
	if (tcgetattr(uart_fd, &options) !=  0) {
		printf("SetupSerial 1");
		return 0;
	}
	options.c_cflag &= ~CSIZE;
	switch (databits) {
	case 7:
		options.c_cflag |= CS7;
		break;
	case 8:
		options.c_cflag |= CS8;
		break;
	default:
		printf("Unsupported data size.");
		return 0;
	}

	switch (parity) {
	case 'n':
	case 'N':
		options.c_cflag &= ~PARENB;	 /* Clear parity enable */
		options.c_iflag &= ~INPCK;	  /* Enable parity checking */
		break;
	case 'o':
	case 'O':
		options.c_cflag |= (PARODD | PARENB);
		options.c_iflag |= INPCK;	   /* Disnable parity checking */
		break;
	case 'e':
	case 'E':
		options.c_cflag |= PARENB;	  /* Enable parity */
		options.c_cflag &= ~PARODD;
		options.c_iflag |= INPCK;	   /* Disnable parity checking */
		break;
	case 'S':
	case 's':						   /*as no parity*/
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;
		break;
	default:
		printf("Unsupported parity");
		return 0;
	}

	switch (stopbits) {
	case 1:
		options.c_cflag &= ~CSTOPB;
		break;
	case 2:
		options.c_cflag |= CSTOPB;
		break;
	default:
		printf("Unsupported stop bits");
		return 0;
	}

	/*
	options.c_cflag |= CLOCAL | CREAD;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_iflag &= ~(BRKINT | INLCR | ICRNL | IGNCR | INPCK | ISTRIP | IXON);
	*/

	options.c_cflag |= CLOCAL | CREAD;
	options.c_oflag &= ~OPOST;
	options.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOE);
	options.c_iflag &= ~(BRKINT | INLCR | ICRNL | IGNCR | INPCK | ISTRIP | IXON);

	/* Set input parity option */
	if (parity != 'n')
		options.c_iflag |= INPCK;

	tcflush(uart_fd,TCIFLUSH);
	options.c_cc[VTIME] = 10;
	options.c_cc[VMIN] = 0;
	/* Update the options and do it NOW */
	if (tcsetattr(uart_fd, TCSANOW, &options) != 0) {
		printf("SetupSerial 3");
		return 0;
	}
	return 1;
}

int init_uart(char *name)
{
	int ret = 0;
	struct termios options;
	
	uart_fd = open(name, O_RDWR);
	if (uart_fd == -1) {
		err("Open uart[%s] fail\n", name);
		return -1;
	}	

	set_speed(9600);
	if (set_Parity(8, 2, 'E') == 0) {
		err("Set uart[%s] parity fail\n", name);
		return -1;
	}

	return uart_fd;
}

int close_uart(void)
{
	int ret = 0;
	
	if (uart_fd != -1) {
		close(uart_fd);
		uart_fd = -1;
	}
	
	return ret;
}

int test_uart(void)
{
	int ret = 0;
	int count;
	char recv[16], send[16];
	
	memset(send, 'a', sizeof(send));
	
	count = write(uart_fd, send, sizeof(send));
	if (count != sizeof(send)) {
		err("Send %d[%d]\n", sizeof(send), count);
	}
	
	count = read(uart_fd, recv, sizeof(recv));
	if (count != sizeof(recv)) {
		err("Recv %d[%d]\n", sizeof(recv), count);
	}
	
	if (memcmp(send, recv, sizeof(send)) != 0x0) {
		printf("send buf:\n");
		dump_memory(send, sizeof(send));
		printf("recv buf:\n");
		dump_memory(recv, sizeof(recv));
	}
	
	return ret;
}

void flush_uart(void)
{
	#ifdef DEBUG_UART_IO
	int ret;
	unsigned char buf[16];
	debug("*****Start of %s\n", __func__);
	do {
		ret = uart_read_bytes(buf, sizeof(buf), 1);
		if (ret > 0) {
			dump_memory(buf, ret);
		}
	} while(ret > 0);
	debug("*****End of %s\n", __func__);
	#else
	tcflush(uart_fd, TCIOFLUSH);
	#endif
}

int uart_send_bytes(char *buf, int count)
{
	int offset = 0, tmp;
	unsigned char *recv_buf = (unsigned char *)malloc(count);

	uart_log(buf, count, 1);
	
	while (offset < count) {
		tmp = write(uart_fd, buf+offset, count-offset);
		if (tmp > 0) {
			offset += tmp;
		}
		if (offset >= count) {
			break;
		}		
	}

	/* Filter the send bytes in code directly */
	if (count > 0) {
		tmp = read(uart_fd, recv_buf, count);
		if (tmp != count) {
			err("Send bytes, but only %d bytes readback\n", tmp);
		} else {
			if (memcmp(buf, recv_buf, count) != 0) {
				err("Send buf:\n");
				dump_memory(buf, offset);
				err("Recv buf:\n");
				dump_memory(recv_buf, offset);
			}
		}
	}

	if (offset >= count) {
		return count;
	}

	err("%s: %d send and timeout\n", __func__, offset);

	return -EIO;
}

int uart_read_bytes(char *buf, int count, int timeout)
{
	int offset = 0, tmp;
	while (offset < count) {
		tmp = read(uart_fd, buf+offset, count-offset);
		if (tmp > 0) {
			offset += tmp;
		}
		if (offset >= count) {
			break;
		}
		if (timeout > 0) {
			--timeout;
			usleep(100);
		} else {
			break;
		}
	}

	uart_log(buf, offset, 0);

	if (offset >= count) {
		return count;
	}

	err("%s: %d readout and timeout\n", __func__, offset);
	if (offset > 0) {
		dump_memory(buf, offset);
	}

	return -EIO;
}


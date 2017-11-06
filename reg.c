#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include<string.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h> 

#define AUDIO_REG_BASE   (0x4A003000)
#define MAP_SIZE        1024*4
static int dev_fd;

#define DEV_NAME  "/dev/ttyS0"
static int g_fd = -1;
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
	tcgetattr(g_fd, &Opt);
	for (i= 0; i < sizeof(speed_arr)/sizeof(int); i++) {
		if (speed == name_arr[i]) {
			tcflush(g_fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(g_fd, TCSANOW, &Opt);
			if (status != 0) {
				printf("tcsetattr g_fd1");
				return;
			}
			tcflush(g_fd,TCIOFLUSH);
		}
	}
}

int set_Parity(int databits, int stopbits, int parity)
{
	struct termios options;
	if (tcgetattr(g_fd, &options) !=  0) {
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

	tcflush(g_fd,TCIFLUSH);
	options.c_cc[VTIME] = 10;
	options.c_cc[VMIN] = 0;
	/* Update the options and do it NOW */
	if (tcsetattr(g_fd, TCSANOW, &options) != 0) {
		printf("SetupSerial 3");
		return 0;
	}
	return 1;
}

int linux_uart_init(void)
{
	char dev[] = DEV_NAME;

	if (g_fd > 0)
		return 1;

	g_fd = open(dev, O_RDWR);

	if (-1 == g_fd) {
		return 0;
	}
	set_speed(9600);
	usleep(100000);
	if (set_Parity(8, 1, 'N') == 0) {
		close(g_fd);
		return 0;
	}
	usleep(50000);
	return 1;
}
int main(int argc, char **argv)
{

	dev_fd = open("/dev/mem", O_RDWR | O_NDELAY);      
	if (dev_fd < 0)  
	{
		printf("open(/dev/mem) failed.");    
		return 0;
	}  

	unsigned char *map_base=(unsigned char * )mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AUDIO_REG_BASE );
	
	int len,ret;
	char buff[30]="dsagfuifgaigerpy";
	char buf[30];
	ret = linux_uart_init();
	if(ret == 0)
	{
		printf("linux uart init failed!\n");
		return -1;
	}
	if(write(g_fd, buff, 20) < 0)
	{
		printf("write error \n");
		return -1;
	}
	len = read(g_fd, buf, 20);
	if (len <= 0) 
	{
		printf("read error \n");
		return -1;
	}	
	printf("obtain111 version sucess:%s\n",buf);

	*(volatile unsigned int *)(map_base + 0x7E0) = 0x00000; //修改该寄存器地址的value  rx
	//usleep(1000000);
	memset(buf,0,30);
	if(write(g_fd, buff, 20) < 0)
	{
		printf("write error \n");
		return -1;
	}
	len = read(g_fd, buf, 20);
	if (len <= 0) 
	{
		printf("read error \n");
		return -1;
	}	
	printf("obtain22222 version sucess:%s\n",buf);

	*(volatile unsigned int *)(map_base + 0x7E0) = 0xc0000; //修改该寄存器地址的value  rx
	//usleep(1000000);
	*(volatile unsigned int *)(map_base + 0x7E4) = 0x00000; //修改该寄存器地址的value tx
	//usleep(1000000);
	memset(buf,0,30);
	if(write(g_fd, buff, 20) < 0)
	{
		printf("write error \n");
		return -1;
	}
	len = read(g_fd, buf, 20);
	if (len <= 0) 
	{
		printf("read error \n");
		return -1;
	}	
	printf("obtain333 version sucess:%s\n",buf);
#if 0
	printf("%x \n", *(volatile unsigned int *)(map_base+0x7E0)); //打印该寄存器地址的value
	printf("%x \n", *(volatile unsigned int *)(map_base+0x7E4));

	*(volatile unsigned int *)(map_base + 0x7E0) = 0xc0000; //修改该寄存器地址的value  rx
	usleep(1000000);
	*(volatile unsigned int *)(map_base + 0x7E4) &= ~(0x1<<18); //修改该寄存器地址的value tx
	usleep(1000000);
#endif
	if(dev_fd)
		close(dev_fd);

	munmap(map_base,MAP_SIZE);//解除映射关系
	close(g_fd);
	return 0;
}

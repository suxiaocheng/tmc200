#include "tmc200.h"
#include "7816.h"
#include "uart.h"

config_device_t config_device_list[] = {
	{"/sys/class/pwm/pwmchip0/pwm1/duty_cycle", "140", TYPE_STR},
	{"/sys/class/pwm/pwmchip0/pwm1/period", "280", TYPE_STR},	
	{"/sys/class/pwm/pwmchip0/pwm1/enable", "1", TYPE_STR},
	{"/sys/class/gpio/export", RST_GPIO, TYPE_STR},
	{"/sys/class/gpio/gpio"RST_GPIO"/direction", "out", TYPE_STR},
	{"/sys/class/gpio/gpio"RST_GPIO"/value", "1", TYPE_STR},
	{NULL, NULL, TYPE_NONE}
};

config_device_t unconfig_device_list[] = {
	{"/sys/class/gpio/unexport", RST_GPIO, TYPE_STR},
	{NULL, NULL, TYPE_NONE}
};

config_device_t tmc200_reset_low[] = {
	{"/sys/class/gpio/gpio"RST_GPIO"/value", "0", TYPE_STR},
	{NULL, NULL, TYPE_NONE}
};

config_device_t tmc200_reset_high[] = {
	{"/sys/class/gpio/gpio"RST_GPIO"/value", "1", TYPE_STR},
	{NULL, NULL, TYPE_NONE}
};

tmc200_t tmc200;

int write_file_config(char *loc, char *str, config_type_e type)
{
	int ret = 0;
	int fd;
	int count;
	long val;
	int write_only = 0;	

	fd = open(loc, O_SYNC|O_RDWR);
	if (fd == -1) {
		write_only = 1;
		fd = open(loc, O_SYNC|O_WRONLY);
		if (fd == -1) {
			err("Open file[%s] fail\n", loc);
			return -1;
		}
	}
#ifdef DEBUG
	/* Read */
	if (write_only == 0) {
		char buf[512];
		int i, j;
		count = read(fd, buf, sizeof(buf));
		for(i=0; i<(count+15)/16; i++){
			printf("%-8x: ", i*16);
			for(j=0; j<16; j++){
				printf("%4x", buf[i*16+j]);
				if(i*16+j > count){
					break;
				}
			}
			printf("\n");
		}
		printf("\n\n");
	}
#endif
	switch (type) {
		case TYPE_STR:
		count = write(fd, str, strlen(str));
		if ((count == -1) || (count < strlen(str))) {
			err("Write %s[%s], only %d bytes write\n", 
				loc, str, count);
			ret = -1;
		}		
		break;
		
		default:
		err("Unknow type: %d\n", type);
		break;
	}	
	
	close(fd);

	return ret;
}

int reset_config_status(int status)
{
	int ret = 0;
	config_device_t *pconf;
	if (status == 0) {
		pconf = tmc200_reset_low;
	} else {
		pconf = tmc200_reset_high;
	}
	ret = write_file_config(pconf->path, pconf->val, pconf->type);
	if (ret != 0) {
		return ret;
	}
	
	return ret;
}

int warm_reset(void)
{
	int ret = 0;
	int count;
	char recv[32];
	
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
	
	/* Wait at least 40000/f=1142us to wait datain */
	usleep(1000000);
	count = read(tmc200.uart_fd, recv, sizeof(recv));
	if (count == -1) {
		return -1;
	}
	debug("Read %d bytes\n", count);
	dump_memory(recv, count);
	
	return ret;
}

int usage(void)
{
	const char *help[] = {
		"./tmc200 [-d dev] [-c test_case]",
		""		
	};

	int i;
	for(i=0; i<sizeof(help)/sizeof(char **); i++){
		printf("%s\n", help[i]);
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 0;
	int dev_fd;
	int cret;
	int test_case = 0;
	
	tmc200.dev = UART_DEV;
	
	while ((cret = getopt(argc,argv,"d:hvc:")) != EOF) {
		switch (cret) {
			case 'd':
			tmc200.dev = strdup(optarg);
			break;
			
			case 'h':
			usage();
			return 0;
			break;
			
			case 'c':
			test_case = atoi(optarg);
			debug("Test case: %d\n", test_case);
			break;
			
			case 'v':
			break;
		}
	}
	
	dev_fd = open("/dev/mem", O_RDWR | O_NDELAY);      
	if (dev_fd < 0) {
		err("open(/dev/mem) failed.");
		return 0;
	}	
	tmc200.reg = (unsigned char *)mmap(NULL, REG_SIZE, 
		PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, REG_BASE);
	
	ret = init_uart(tmc200.dev);
	if(ret < 0){
		return -1;
	}
	
	*(volatile unsigned int *)(tmc200.reg + 0x7E4) = 0x0000F; // disable tx

	config_device_t *pconf;
	for (pconf = config_device_list; pconf->path != NULL; pconf++) {
		ret = write_file_config(pconf->path, pconf->val, pconf->type);
		if (ret != 0) {
			return ret;
		}
	}

	switch (test_case) {
		case 0:
		ret = warm_reset();
		if (ret != 0) {
			err("Execute warm reset fail\n");
		}
		break;
		
		case 1:
		ret = CARDreset();
		if (ret == 0) {
			err("Execute card reset fail\n");
		}
		break;
		
		case 2:
		{
			char cmd[] = {0x00, 0x84, 0x00, 0x00, 0x08};
			char *ret_cmd;
			int i;
			set_apdu_buf(cmd, sizeof(cmd));
			ret = trans_t0();
			if (ret == 0) {
				err("Execute card reset fail\n");
				break;
			}
			i = get_apdu_length();
			ret_cmd = get_apdu();
			dump_memory(ret_cmd, i);
		}
		break;
	}
	
	
	close_uart();
	
	for (pconf = unconfig_device_list; pconf->path != NULL; pconf++) {
		ret = write_file_config(pconf->path, pconf->val, pconf->type);
		if (ret != 0) {
			err("Unconfig %s[%s] fail\n", pconf->path, pconf->val);
		}
	}
	
	munmap(tmc200.reg, REG_SIZE);
	close(dev_fd);

	return ret;
}


#include "tmc200.h"
#include "7816.h"
#include "uart.h"

#define PWM_DIR		"/sys/class/pwm/pwmchip2/"
#define PWM_CHANNEL	"pwm0/"

config_device_t config_device_list[] = {
	{PWM_DIR"export", "0", TYPE_STR},
	{PWM_DIR PWM_CHANNEL "period", "270", TYPE_STR},
	{PWM_DIR PWM_CHANNEL "duty_cycle", "135", TYPE_STR},
	{PWM_DIR PWM_CHANNEL "enable", "1", TYPE_STR},
	{"/sys/class/gpio/export", RST_GPIO, TYPE_STR},
	{"/sys/class/gpio/gpio"RST_GPIO"/direction", "out", TYPE_STR},
	{"/sys/class/gpio/gpio"RST_GPIO"/value", "1", TYPE_STR},
	{NULL, NULL, TYPE_NONE}
};

config_device_t unconfig_device_list[] = {
	{"/sys/class/gpio/unexport", RST_GPIO, TYPE_STR},
	{"/sys/class/pwm/pwmchip2/pwm0/enable", "0", TYPE_STR},
	{"/sys/class/pwm/pwmchip2/unexport", "0", TYPE_STR},
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
			debug("%-8x: ", i*16);
			for(j=0; j<16; j++){
				debug("%4x", buf[i*16+j]);
				if(i*16+j > count){
					break;
				}
			}
			debug("\n");
		}
		debug("\n\n");
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

int check_device_path(void)
{
	int ret = 0;
	struct stat fstat;
	ret = stat(PWM_DIR, &fstat);
	if (ret < 0) {
		err("PWM is not enable in dts\n");
		return ret;
	}
	ret = stat(PWM_DIR PWM_CHANNEL, &fstat);
	if (ret == 0) {
		err("PWM %s is exist\n", PWM_CHANNEL);
		return ret;
	}
	config_device_t *pconf;
	for (pconf = config_device_list; pconf->path != NULL; pconf++) {
		ret = write_file_config(pconf->path, pconf->val, pconf->type);
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

int unconfig_device_path(void)
{
	int ret = 0;

	config_device_t *pconf;
	for (pconf = unconfig_device_list; pconf->path != NULL; pconf++) {
		ret = write_file_config(pconf->path, pconf->val, pconf->type);
		if (ret != 0) {
			err("Unconfig %s[%s] fail\n", pconf->path, pconf->val);
		}
	}

	return 0;
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

			case 'q':
			unconfig_device_path();
			return 0;
			break;
			
			case 'v':
			break;
		}
	}

	ret = check_device_path();
	if (ret != 0) {
		err("%s fail\n", "check_device_path");
		return ret;
	}
	
	tmc200.uart_fd = init_uart(tmc200.dev);
	if(tmc200.uart_fd < 0){
		return -1;
	}

	switch (test_case) {
		case 0:
		set_speed(9600);
		ret = warm_reset();
		if (ret != 0) {
			err("Execute warm reset fail\n");
		}
		break;
		
		case 1:
		set_speed(9600);
		ret = CARDreset();
		if (ret == 0) {
			err("Execute card reset fail\n");
		}
		break;
		
		case 2:
		{
			char cmd[] = {0x00, 0x84, 0x00, 0x00, 0x04};
			char *ret_cmd;
			int i;
			set_apdu_buf(cmd, sizeof(cmd));
			ret = trans_t0();
			if (ret == 0) {
				err("Execute cmd fail\n");
				break;
			}
			i = get_apdu_length();
			ret_cmd = get_apdu();
			dump_memory(ret_cmd, i);
		}
		break;

		case 3:
		{
			char cmd[] = {0x00, 0xa4, 0x04, 0x00, 0x10, 0xa0,
				0x00, 0x00, 0x06, 0x28, 0x00, 0x00, 0x01, 0x00,
				0x00, 0x00, 0x00, 0x01, 0xe2, 0x00, 0x01};
			char *ret_cmd;
			int i;
			set_apdu_buf(cmd, sizeof(cmd));
			ret = trans_t0();
			if (ret == 0) {
				err("Execute cmd fail\n");
				break;
			}
			i = get_apdu_length();
			ret_cmd = get_apdu();
			dump_memory(ret_cmd, i);
		}
		break;
	}
	
	flush_uart();

	print_uart_log();
	
	close_uart();

	return ret;
}


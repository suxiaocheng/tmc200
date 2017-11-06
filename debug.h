#ifndef __DEBUG_H__
#define __DEBUG_H__

#define debug(f, a...)	printf("[DEBUG]%s(%d):" f,  __func__, errno , ## a)
#define err(f, a...)	printf("[ERR]%s(%d):" f,  __func__, errno , ## a)

static __inline int dump_memory(const char *buf, int count)
{
	int ret = 0; 
	int i, j;	
	for(i=0; i<(count+15)/16; i++){
		printf("%-8x: ", i*16);
		for(j=0; j<16; j++){
			if(i*16+j >= count){
				break;
			}
			printf("%4x", buf[i*16+j]);			
		}
		printf("\n");
	}
	
	return ret;
}

#endif

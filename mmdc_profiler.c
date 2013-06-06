#include <sys/stat.h> 
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

#define MMDCx_SIZE  4096 //place the size here
#define MMDCx_ADDR  0x21B0000 //place the physical address here
#define NULL 0

int thread_exit = 0;

void * mmdc_profiler() {
int _fdmem, i;
uint8_t *map = NULL;
const char memDevice[] = "/dev/mem";

_fdmem = open( memDevice, O_RDWR | O_SYNC );

if (_fdmem < 0){
printf("Failed to open the /dev/mem !\n");
return 0;
}
else{
printf("open /dev/mem successfully !\n");
}

map= (uint8_t *)(mmap(0, MMDCx_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,_fdmem,MMDCx_ADDR));

printf("%x: %s\n", map, strerror(errno));

uint32_t *pcr0 = map + 0x410;
uint32_t *pcr1 = map + 0x414;
uint32_t *psr0 = map + 0x418;
uint32_t *psr1 = map + 0x41C;
uint32_t *psr2 = map + 0x420;
uint32_t *psr3 = map + 0x424;
uint32_t *psr4 = map + 0x428;
uint32_t *psr5 = map + 0x42C;

while (!thread_exit) {
    uint32_t s_pcr0_1 = *pcr0;
    uint32_t s_pcr1_1 = *pcr1;
    uint32_t s_psr0_1 = *psr0;
    uint32_t s_psr1_1 = *psr1;
    uint32_t s_psr2_1 = *psr2;
    uint32_t s_psr3_1 = *psr3;
    uint32_t s_psr4_1 = *psr4;
    uint32_t s_psr5_1 = *psr5;
    //*pcr1 = 0x38060000; //ARM SOURCES
    *pcr0 = 0x9; // START
    usleep(1000000);
    *pcr0 |= 0x4; //FREEZE
    printf("madpcr0: 0x%08x\n", *pcr0);
    printf("madpcr1: 0x%08x\n", *pcr1);
    printf("total cycles: 0x%08x %u/s\n", *psr0 - s_psr0_1, *psr0 - s_psr0_1);
    printf("busy cycles: 0x%08x %u/s (Utilization: %2.1f%)\n", *psr1 - s_psr1_1, *psr1 - s_psr1_1, (float)(*psr1 - s_psr1_1)/(float)(*psr0 - s_psr0_1)*100.0f);
    printf("read access: 0x%08x %u/s\n", *psr2 - s_psr2_1, *psr2 - s_psr2_1);
    printf("write access: 0x%08x %u/s\n", *psr3 - s_psr3_1, *psr3 - s_psr3_1);
    printf("read bytes: 0x%08x %2.1fMB/s\n", *psr4 - s_psr4_1, (float)(*psr4 - s_psr4_1)/(float)(1024*1024));
    printf("write bytes: 0x%08x %2.1fMB/s\n", *psr5 - s_psr5_1, (float)(*psr5 - s_psr5_1)/(float)(1024*1024));
    *pcr0 = 0x2;
}

/* unmap the area & error checking */
if (munmap(map,MMDCx_SIZE)==-1){
perror("Error un-mmapping the file");
}

/* close the character device */
close(_fdmem);

thread_exit = 0;
}

#define BLOCK_SIZE 25*1024*1024

void * mmdc_read(void* arg) {
    uint64_t count = 0;
    uint64_t tmp;
    uint64_t *mem = (uint64_t*) arg;//malloc (1*1024*1024*sizeof(uint64_t));

    while (!thread_exit) {
	for (count = 0; count < BLOCK_SIZE; count++) {
	    tmp = *(mem + count);
	}
    }
}

void * mmdc_write(void* arg) {
    uint64_t count = 0;
    uint64_t *mem = (uint64_t*) arg;//malloc (1*1024*1024*sizeof(uint64_t));

    while (!thread_exit) {
	for (count = 0; count < BLOCK_SIZE; count++)
	    *(mem + count) = count;
    }
}

void * mmdc_write_sleep(void* arg) {
    uint64_t count = 0;
    uint64_t *mem = (uint64_t*) arg;//malloc (1*1024*1024*sizeof(uint64_t));

    while (!thread_exit) {
	for (count = 0; count < BLOCK_SIZE; count++) {
	    *(mem + count) = count;
	}
	usleep(100000);
    }
}

int main(void) {
    int i;
    uint64_t tmp, count = 0;
    pthread_t tprofile, tload_write, tload_read;

    uint64_t *mem = (uint64_t*) malloc (BLOCK_SIZE*sizeof(uint64_t));

    pthread_create(&tprofile, NULL, mmdc_profiler, NULL);
// Write
    for (i = 0; i < 8; i++)
	pthread_create(&tload_write, NULL, mmdc_write, (void*) mem);

// Read
//    pthread_create(&tload_write, NULL, mmdc_write_sleep, (void*) mem);
//    for (i = 0; i < 32; i++)
//	pthread_create(&tload_read, NULL, mmdc_read, (void*) mem);

    getchar();
    //thread_exit = 1;
    //while(thread_exit) {};

    free(mem);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <ogc/aram.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;


static u64 AR_interrupt_time = 0;
static u64 AR_start_time = 0;
static u64 AR_end_time = 0;

void AR_test_handler() {
  AR_interrupt_time = gettime();
}


void * Initialise() {

	void *framebuffer;

	VIDEO_Init();
	PAD_Init();
	
	rmode = VIDEO_GetPreferredMode(NULL);

	framebuffer = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(framebuffer,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(framebuffer);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	return framebuffer;

}

const u32 ar_buffer_size = 10*1024*1024;

int main(int argc, char **argv) {

	xfb = Initialise();

	printf("\nARAM_interrupt test\n");
	AR_Init(NULL,0);
	
	void *ar_buffer = MEM_K0_TO_K1(memalign(32,ar_buffer_size));
	memset(ar_buffer,0xaa, ar_buffer_size);
	
	AR_RegisterCallback(AR_test_handler);
	
	AR_start_time = gettime();
	AR_StartDMA(AR_MRAMTOARAM, MEM_VIRTUAL_TO_PHYSICAL(ar_buffer), 0, ar_buffer_size);
	
	while(AR_GetDMAStatus() != 0) {
	  /* spin */
	}
	AR_end_time = gettime();
	
	
	printf("Before DMA MRAM->ARAM time: 0x%016llx\n", AR_start_time);
	printf("AR_test_handler time:       0x%016llx\n", AR_interrupt_time);
	printf("After DMA MRAM->ARAM time:  0x%016llx\n", AR_end_time);
	
	float pc = 100.0*(AR_interrupt_time-AR_start_time)/(AR_end_time-AR_start_time);
	printf("Interrupt occurs %f%% through the transfer\n", pc);
	printf("Total xfer ticks: 0x%016llx\n", AR_end_time-AR_start_time);
	
	printf("\n");
	AR_start_time = gettime();
	AR_StartDMA(AR_ARAMTOMRAM, MEM_VIRTUAL_TO_PHYSICAL(ar_buffer), 0, ar_buffer_size);
	
	while(AR_GetDMAStatus() != 0) {
	  /* spin */
	}
	AR_end_time = gettime();
	
	
	printf("Before DMA ARAM->MRAM time: 0x%016llx\n", AR_start_time);
	printf("AR_test_handler time:       0x%016llx\n", AR_interrupt_time);
	printf("After DMA ARAM->MRAM time:  0x%016llx\n", AR_end_time);
	
	pc = 100.0*(AR_interrupt_time-AR_start_time)/(AR_end_time-AR_start_time);
	printf("Interrupt occurs %f%% through the transfer\n", pc);
	printf("Total xfer ticks: 0x%016llx\n", AR_end_time-AR_start_time);
	printf("Press START to exit\n");
	
	

	while(1) {

		VIDEO_WaitVSync();
		PAD_ScanPads();

		int buttonsDown = PAD_ButtonsDown(0);
		

		if (buttonsDown & PAD_BUTTON_START) {
			exit(0);
		}
	}

	return 0;
}


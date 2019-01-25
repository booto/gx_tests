#include <stdio.h>
#include <gccore.h>
#include <stdint.h>
#include <cstring>
#include <vector>
#include <limits>
#include <algorithm>
#include <functional>

#define DEFAULT_FIFO_SIZE  (256*1024)

int main( int argc, char **argv ){
  VIDEO_Init();
  PAD_Init();


  GXRModeObj *console_render_mode = VIDEO_GetPreferredMode(NULL);
  void *console_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(console_render_mode));

  console_init(console_xfb, 20, 20, console_render_mode->fbWidth, console_render_mode->xfbHeight, console_render_mode->fbWidth*VI_DISPLAY_PIX_SZ);
  VIDEO_SetNextFramebuffer(console_xfb);
  VIDEO_Configure(console_render_mode);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  if(console_render_mode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
  
  u32 id = SI_GetType(SI_CHAN0);
  printf("SI_Type(0): %08lx\n", id);


  printf("Press A to exit\n");
  while(1) {
    PAD_ScanPads();
    if (PAD_ButtonsDown(0) & PAD_BUTTON_A) break;
    VIDEO_Flush();
    VIDEO_WaitVSync();
  }
}



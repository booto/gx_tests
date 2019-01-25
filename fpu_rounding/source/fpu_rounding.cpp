#include <stdio.h>
#include <gccore.h>
#include <stdint.h>
#include <cstring>
#include <vector>
#include <limits>
#include <algorithm>
#include <functional>

#define DEFAULT_FIFO_SIZE  (256*1024)


// FPSCR routines from https://github.com/lioncash/DolphinPPCTests/blob/master/source/FloatingPoint.cpp


static void fpscr_set(uint32_t value) {
  uint64_t i = value;
  double d = 0.0;
  std::memcpy(&d, &i, sizeof(uint64_t));
  asm volatile ("mtfsf 0xFF, %[reg]" : : [reg]"f"(d));
}


static uint32_t fpscr_get() {
  double d = 0.0;
  asm volatile ("mffs %[out]" : [out]"=f"(d));
  uint64_t i = 0;
  std::memcpy(&i, &d, sizeof(uint64_t));
  return static_cast<uint32_t>(i);
}

static void fpscr_clear_fr() {
  uint32_t fpscr = fpscr_get();
  fpscr &= ~0x00040000;
  fpscr_set(fpscr);
}

static void fpscr_clear_fi() {
  uint32_t fpscr = fpscr_get();
  fpscr &= ~0x00020000;
  fpscr_set(fpscr);
}

static void fpscr_set_rounding_mode(uint32_t index) {
  uint32_t fpscr = fpscr_get();
  fpscr &= ~3;
  fpscr |= (index&3);
  fpscr_set(fpscr);
}

static void fpscr_clear_exceptions() {
  uint32_t fpscr = fpscr_get();
  fpscr &= ~0xFFF807F8;
  fpscr_set(fpscr);
}


template <typename T>
void dump_hex(const T &input) {
  uint8_t buffer[sizeof(T)];
  std::copy_n((uint8_t*)&input, sizeof(T), &buffer[0]);
  for (size_t n=0; n<sizeof(T); ++n) {
    printf("%02hhx", buffer[n]);
  }
}

/*
template <typename T>
void float_tests(uint32_t rounding_mode) {
  uint32_t old_fpscr = fpscr_get();
  fpscr_clear_exceptions();
  fpscr_set_rounding_mode(rounding_mode);
  T t1 = 1.0;
  T t2 = t1 + std::numeric_limits<T>::epsilon();
  T t3 = (t1 + t2)/2;
  printf("t3: "); dump_hex(t3); printf(" fpscr: %08lx\n", fpscr_get());
  fpscr_set(old_fpscr);
}
*/


template <typename T>
void run_fp_tests(uint32_t rounding_mode, std::function<T(void)> f) {
  uint32_t old_fpscr = fpscr_get();
  fpscr_clear_exceptions();
  fpscr_clear_fr();
  fpscr_clear_fi();
  fpscr_set_rounding_mode(rounding_mode);
  T result = f();
  printf("result: "); dump_hex(result); printf(" fpscr: %08lx\n", fpscr_get());
  fpscr_set(old_fpscr);
}


static const uint32_t ROUND_NEAREST = 0;
static const uint32_t ROUND_TO_ZERO = 1;
static const uint32_t ROUND_TO_PINF = 2;
static const uint32_t ROUND_TO_NINF = 3;

std::vector<uint32_t> rounding_modes = {ROUND_NEAREST, ROUND_TO_ZERO, ROUND_TO_PINF, ROUND_TO_NINF};



static void *console_xfb = NULL;

int main( int argc, char **argv ){
  VIDEO_Init();
  PAD_Init();


  GXRModeObj *console_render_mode = VIDEO_GetPreferredMode(NULL);
  console_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(console_render_mode));

  console_init(console_xfb, 20, 20, console_render_mode->fbWidth, console_render_mode->xfbHeight, console_render_mode->fbWidth*VI_DISPLAY_PIX_SZ);
  VIDEO_SetNextFramebuffer(console_xfb);
  VIDEO_Configure(console_render_mode);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  if(console_render_mode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
  
  for (const auto& rounding_mode: rounding_modes) {
    run_fp_tests<float>(rounding_mode, []() {
	  float out;
	  asm volatile ("fdivs %[out], %[num], %[den]" : [out]"=f"(out) : [num]"f"(1.0f), [den]"f"(3.0f));
	  return out;
	});
  }
  


  printf("Press A to exit\n");
  while(1) {
    PAD_ScanPads();
    if (PAD_ButtonsDown(0) & PAD_BUTTON_A) break;
    VIDEO_Flush();
    VIDEO_WaitVSync();
  }
}



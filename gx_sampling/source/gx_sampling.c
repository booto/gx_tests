/* based on gxSprites demo */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>

#define DEFAULT_FIFO_SIZE  (256*1024)

static void *console_xfb = NULL;
static void *gx_xfb = NULL;

// This function searches for which division of the rectangular area with top left corner [start_x, start_y] and
// bottom right corner [end_x, end_y]. Divisions are rectangular, with a width/height of size_x/size_y.
int RefineCoord(float start_x, float end_x, float start_y, float end_y, float size_x, float size_y, float*x, float *y) {
  // This is the color of the quad we search for
  static const u8 color[3] = {0xff, 0x00, 0x00};
  float cur_x=start_x, cur_y=start_y;
  int rv = 1;
  int stop = 0;
  printf("Searching x:[%10.10f], y:[%10.10f]\n", start_x, start_y);
  while(1) {

    PAD_ScanPads();

    if (PAD_ButtonsDown(0) & PAD_BUTTON_A) return 1;

    if (stop == 0) {
      GX_Begin(GX_QUADS, GX_VTXFMT0, 4);      // Draw A Quad
        GX_Position2f32(cur_x, cur_y);          // Top Left
        GX_Color3u8(color[0],color[1],color[2]);
        GX_Position2f32(cur_x+size_x, cur_y);      // Top Right
        GX_Color3u8(color[0],color[1],color[2]);
        GX_Position2f32(cur_x+size_x, cur_y+size_y);  // Bottom Right
        GX_Color3u8(color[0],color[1],color[2]);
        GX_Position2f32(cur_x, cur_y+size_y);      // Bottom Left
        GX_Color3u8(color[0],color[1],color[2]);
      GX_End();
      GX_DrawDone();

      GXColor sample;
      memset(&sample, 0, sizeof(sample));
      GX_PeekARGB(0,0, &sample);
      printf("                                       \rx: %03.2f%% y: %03.2f%%\r", 100.0f*(cur_x-start_x)/(end_x-start_x), 100.0f*(cur_y-start_y)/(end_y-start_y));
      GX_CopyDisp(gx_xfb, GX_TRUE);
      VIDEO_Flush();
      VIDEO_WaitVSync();

      if (sample.r == color[0] &&
          sample.g == color[1] &&
          sample.b == color[2]) {
        printf("\n in: (%10.10f, %10.10f)\n", cur_x, cur_y);
        *x = cur_x;
       *y = cur_y;
        rv = 0;
      }
      cur_x = cur_x + size_x;
      if (cur_x > end_x) {
        cur_y = cur_y + size_y;
        cur_x = start_x;
      }
      if (cur_y > end_y) {
        printf("\ndone!\n\n");
        return rv;
      }
    }
  }
}

float start_x=0.0f;
float start_y=0.0f;
float end_x=1.0f;
float end_y=1.0f;
float step_x;
float step_y;
//---------------------------------------------------------------------------------
int main( int argc, char **argv ){
//---------------------------------------------------------------------------------
  f32 yscale;
  Mtx44 proj;
  Mtx GXmodelView2D;
  void *gp_fifo = NULL;

  GXColor background = {0, 0, 0, 0xff};

  VIDEO_Init();
  PAD_Init();

    
  // GX render mode never hits the screen, but it's how things are rendered on the GPU
  // We set this up to be 640x480, with an orthographic projection of the same
  
  GXRModeObj *gx_render_mode = &TVNtsc480Prog;
  
  
  // Console render mode is what is displayed, and is constructed by direct XFB pokes.
  // Not used for GX at all.
  GXRModeObj *console_render_mode = VIDEO_GetPreferredMode(NULL);
  


  console_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(console_render_mode));
  gx_xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(gx_render_mode));


  // Initialise the console, required for printf
  console_init(console_xfb, 20, 20, console_render_mode->fbWidth, console_render_mode->xfbHeight, gx_render_mode->fbWidth*VI_DISPLAY_PIX_SZ);
  VIDEO_SetNextFramebuffer(console_xfb);
  VIDEO_Configure(console_render_mode);
  VIDEO_SetBlack(FALSE);
  VIDEO_Flush();
  VIDEO_WaitVSync();
  if(console_render_mode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();


  // setup the fifo and then init the flipper
  gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
  memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

  GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);

  // clears the bg to color and clears the z buffer
  GX_SetCopyClear(background, 0x00ffffff);

  // other gx setup
  GX_SetViewport(0, 0, gx_render_mode->fbWidth, gx_render_mode->efbHeight,0,1);
  yscale = GX_GetYScaleFactor(gx_render_mode->efbHeight, gx_render_mode->xfbHeight);
  GX_SetDispCopyYScale(yscale);
  GX_SetScissor(0,0,gx_render_mode->fbWidth, gx_render_mode->efbHeight);
  GX_SetDispCopySrc(0, 0, gx_render_mode->fbWidth, gx_render_mode->efbHeight);
  GX_SetDispCopyDst(gx_render_mode->fbWidth, gx_render_mode->xfbHeight);
  GX_SetCopyFilter(gx_render_mode->aa, gx_render_mode->sample_pattern, GX_TRUE, gx_render_mode->vfilter);
  GX_SetFieldMode(gx_render_mode->field_rendering, ((gx_render_mode->viHeight==2*gx_render_mode->xfbHeight)?GX_ENABLE:GX_DISABLE));
  GX_SetZMode(GX_DISABLE, GX_ALWAYS, GX_FALSE);

  if (gx_render_mode->aa)
    GX_SetPixelFmt(GX_PF_RGB565_Z16, GX_ZC_LINEAR);
  else
    GX_SetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);


  GX_SetCullMode(GX_CULL_NONE);
  GX_SetDispCopyGamma(GX_GM_1_0);

  // setup the vertex descriptor
  // tells the flipper to expect direct data
  GX_ClearVtxDesc();
  GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
  GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);

  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
  GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGB8,  0);

  GX_SetNumChans(1);
  GX_SetNumTexGens(0);
  GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
  GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);


  guOrtho(proj,0,480,0,640,0,300);
  GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);

  GX_SetViewport(0, 0, gx_render_mode->fbWidth, gx_render_mode->efbHeight,0,1);
  guMtxIdentity(GXmodelView2D);
  guMtxTransApply (GXmodelView2D, GXmodelView2D, 0.0F, 0.0F, -5.0F);
  GX_LoadPosMtxImm(GXmodelView2D,GX_PNMTX0);
  
  int rv;
  printf("\n\nBegin: Searching inside (%f,%f)->(%f,%f)\n", start_x, start_y, end_x, end_y);
  
  do {
    step_x = (end_x-start_x)/2;
    step_y = (end_y-start_y)/2;
    rv = RefineCoord(start_x, end_x, start_y, end_y, step_x, step_y, &start_x, &start_y);
    if (rv != 0) {
      printf("error: rv was %d\n", rv);
      break;
    }
    if (step_x < 0.0000001 || step_y < 0.0000001) {
      printf("ending: x:%10.10f y:%10.10f\n", start_x, start_y);
      break;
    }
    end_x = start_x + step_x;
    end_y = start_y + step_y;
  } while(1);
  
  
  printf("Press A to exit\n");
  while(1) {
    PAD_ScanPads();
    if (PAD_ButtonsDown(0) & PAD_BUTTON_A) break;
    VIDEO_Flush();
    VIDEO_WaitVSync();
  }
}



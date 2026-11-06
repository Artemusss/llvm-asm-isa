/* sim_stub.c — headless-реализация графики (без окна/SDL2) для проверки.
 * Считает пиксели/кадры и завершает процесс, чтобы симуляция/интерпретация не
 * крутили полный кадр (1536*768) слишком долго. Для настоящего окна берите
 * sim.c из курса + -lSDL2.
 *
 *   SIM_FRAMES     кадров (simFlush), затем exit(0). По умолч. 1000 (все).
 *   SIM_MAX_PIXELS лимит simPutPixel, затем exit(0). По умолч. 50000. 0 = без лимита.
 */
#include "../include/sim.h"
#include <stdio.h>
#include <stdlib.h>

static unsigned long long g_pixels = 0, g_maxpix = 50000ULL;
static int g_frames = 0, g_frame_limit = 1000;
static unsigned long long g_hash = 1469598103934665603ULL; /* FNV-1a */

static void report(void) {
  fprintf(stderr, "[sim_stub] pixels=%llu frames=%d hash=%016llx\n",
          g_pixels, g_frames, g_hash);
}

void simInit(void) {
  const char *f = getenv("SIM_FRAMES");
  if (f && *f) g_frame_limit = atoi(f);
  const char *p = getenv("SIM_MAX_PIXELS");
  if (p && *p) g_maxpix = strtoull(p, NULL, 10);
  atexit(report); /* печать контрольной суммы даже при раннем exit() */
  fprintf(stderr, "[sim_stub] headless: frame_limit=%d max_pixels=%llu\n",
          g_frame_limit, g_maxpix);
}

void simExit(void) {
  fprintf(stderr, "[sim_stub] exit: pixels=%llu frames=%d\n", g_pixels, g_frames);
}

void simFlush(void) {
  if (++g_frames >= g_frame_limit) {
    fprintf(stderr, "[sim_stub] frame limit (pixels=%llu) -> exit\n", g_pixels);
    exit(0);
  }
}

void simPutPixel(int x, int y, int argb) {
  /* контрольная сумма потока пикселей (x,y,argb) — для сверки режимов */
  unsigned long long v = ((unsigned long long)(unsigned)x << 42) ^
                         ((unsigned long long)(unsigned)y << 21) ^
                         (unsigned)argb;
  g_hash = (g_hash ^ v) * 1099511628211ULL;
  if (g_maxpix && ++g_pixels >= g_maxpix) {
    fprintf(stderr, "[sim_stub] pixel limit %llu -> exit\n", g_maxpix);
    exit(0);
  }
}

int simRand(void) { return rand(); }

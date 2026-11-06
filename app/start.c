/* start.c — точка входа для пути "выгруженный IR + настоящий sim":
 *   ./asmsim app.myasm 3 out.ll
 *   clang out.ll app/sim_stub.c app/start.c -I include -o app && ./app
 * С симулятором (asmsim) НЕ линкуется — там свой main в main.cpp. */
#include "../include/sim.h"
void app(void); /* определяется в сгенерированном out.ll */
int main(void) {
  simInit();
  app();
  simExit();
  return 0;
}

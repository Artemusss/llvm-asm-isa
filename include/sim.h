#ifndef SIM_H
#define SIM_H

/* Графический интерфейс. Реализацию даёт headless-заглушка app/sim_stub.c
 * (без окна) или настоящий SDL2-sim из курса. Размеры — как в app.c заданий 1/3. */
#define SIM_X_SIZE 1536
#define SIM_Y_SIZE 768

#ifdef __cplusplus
extern "C" {
#endif

void simInit(void);
void simExit(void);
void simFlush(void);
void simPutPixel(int x, int y, int argb);
int simRand(void);

#ifdef __cplusplus
}
#endif

#endif // SIM_H

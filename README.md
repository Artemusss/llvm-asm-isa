# Своя ISA + ASM2IR (задание 4)

Собственный набор инструкций графического приложения `app.c`,
приложение на этом ASM и симулятор с тремя режимами исполнения.

Приложение и статистика:
`app.c` = `simPutPixel(x, y, 0xFF000000 + x*y*step)` (экран 1536×768, 1000
кадров), репозитории
[llvm-ir-use-trace](https://github.com/Artemusss/llvm-ir-use-trace) (задания
1–2) и [llvm-app-irgen](https://github.com/Artemusss/llvm-app-irgen) (задание 3).
Статистика задания 2 по этому приложению скопирована в
[`stats/patterns.txt`](stats/patterns.txt) — именно из неё выведена ISA
([`ISA.md`](ISA.md)). `app.myasm` воспроизводит `app.c` побитово.

Репозиторий реализует:
1. **Набор инструкций** из статистики задания 2 и `-O2` IR приложения —
   [`ISA.md`](ISA.md) (машинная таблица — [`include/ISA.h`](include/ISA.h)).
2. **Приложение на своём ASM** — [`app.myasm`](app.myasm).
3. **ASM2IR с вызовами эмулирующих функций** (кроме потока управления) + запуск
   на ExecutionEngine — режим 2, [`extIR.cpp`](extIR.cpp).
4. **ASM2IR с полной генерацией IR-эквивалентов** всех инструкций + запуск на
   ExecutionEngine — режим 3, [`fullIR.cpp`](fullIR.cpp).
5. **(\*) Симуляция без генерации IR** — только вызовы эмулирующих функций —
   режим 1, [`cpu.cpp`](cpu.cpp).

Одна X-macro
таблица `ISA_(...)` задаёт для каждой инструкции парсинг ASM, симуляцию
(`Execute`) и генерацию IR (`IRGenExecute`) сразу.

## Зависимости (Ubuntu)

```bash
sudo apt update
sudo apt install -y clang-18 llvm-18 llvm-18-dev
```

## Сборка и запуск

```bash
./build.sh                 # -> ./asmsim
./run.sh 1                 # режим 1: симуляция (п.5*)
./run.sh 2                 # режим 2: IR с эмулирующими функциями (п.3)
./run.sh 3                 # режим 3: полный IR + ExecutionEngine (п.4)
./run.sh 3 app.gen.ll      # режим 3: выгрузить IR в файл
```
Прямой вызов: `./asmsim <asm-файл> <режим 1-3> [out.ll]`.

Полный кадр — `1536*768` вызовов `simPutPixel`; в режимах 1 и 2 (поинструкционно)
это медленно, поэтому заглушка завершает процесс по лимиту:

| переменная | по умолч. | смысл |
|---|---|---|
| `SIM_MAX_PIXELS` | `50000` | лимит пикселей, затем `exit(0)` (`0` — без лимита) |
| `SIM_FRAMES` | `1000` | лимит кадров (`simFlush`), затем `exit(0)` |

```bash
SIM_MAX_PIXELS=30000 ./run.sh 2      # быстрый прогон режима 2
SIM_MAX_PIXELS=0 SIM_FRAMES=1 ./run.sh 3   # один полный кадр через полный IR
```

## Проверка эквивалентности

`sim_stub` считает контрольную сумму (FNV-1a) потока пикселей `(x,y,argb)` и
печатает её в конце (`hash=...`). На одинаковом лимите все режимы и оригинал
дают один хэш:

```bash
for m in 1 2 3; do SIM_MAX_PIXELS=30000 ./asmsim app.myasm $m 2>&1 \
  | grep -oE 'hash=[0-9a-f]+' | tail -1; done
# все три -> hash=6b283957716989c3, совпадает с исходным app.c
```

## Генерация IR и линковка с настоящим sim

Режим 3 выгружает корректный LLVM IR (`define @app` + внешние `@simPutPixel`/
`@simFlush`), который линкуется с реализацией графики:

```bash
./asmsim app.myasm 3 app.gen.ll
clang app.gen.ll app/sim_stub.c app/start.c -I . -o app && ./app
# для настоящего окна: заменить sim_stub.c на SDL/sim.c из курса + -lSDL2
```
`opt -O2 app.gen.ll` сворачивает память-регистры в **phi-форму из задания 1**.

## Структура

```
include/ISA.h       X-macro таблица инструкций
include/*.h         заголовки (instr, bin, cpu, sim, extIR, fullIR)
instr.cpp bin.cpp   словари + парсер/дизассемблер ASM
cpu.cpp             режим 1: симуляция
extIR.cpp           режим 2: IR с вызовами do_<Name>, нативный поток упр
fullIR.cpp          режим 3: полный IR-эквивалент + выгрузка в .ll
main.cpp            диспетчер режимов
ISA.md              описание набора инструкций                           [п.1]
stats/              моя статистика задания 2 (источник для ISA)
app.myasm           приложение на своей ISA                              [п.2]
app/sim_stub.c      headless-графика
app/start.c         точка входа для пути "выгруженный IR + sim"
build.sh run.sh     сборка и запуск
```


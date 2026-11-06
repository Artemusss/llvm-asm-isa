#!/usr/bin/env bash
# Запуск приложения на своей ISA в одном из режимов.
#   ./run.sh 1            симуляция (п.5*)
#   ./run.sh 2            IR с эмулирующими функциями (п.3)
#   ./run.sh 3            полный IR-эквивалент (п.4)
#   ./run.sh 3 app.gen.ll выгрузить сгенерированный IR в файл
# Переменные SIM_MAX_PIXELS / SIM_FRAMES ограничивают длительность (см. sim_stub.c).
set -euo pipefail
cd "$(dirname "$0")"
[[ -x ./asmsim ]] || ./build.sh
./asmsim app.myasm "$@"

#ifndef FULLIR_H
#define FULLIR_H
#include "bin.h"
#include "cpu.h"
#include "extIR.h"

// Режим 3 (задание п.4): генерация ПОЛНОГО IR-эквивалента каждой инструкции
// (IRGenExecute), без эмулирующих функций для данных. Графика — прямые call
// @simPutPixel/@simFlush. Регистровый файл — alloca [16 x i32].
struct FullIR : ExtIR {
  void buildIR(Binary &Bin);
  void executeIR(CPU &Cpu);
  bool printIR(const std::string &FileName, std::string &ErrorMsg); // -> *.ll
};
#endif // FULLIR_H

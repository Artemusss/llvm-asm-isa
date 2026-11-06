#ifndef CPU_H
#define CPU_H
#include "bin.h"
#include "sim.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <string>

// Модель процессора: регистровый файл + Execute-цикл (режим 1 / п.5*).
// Статические do_<Name> — те самые "эмулирующие инструкции функции", которые
// режим 2 (extIR) вызывает из сгенерированного IR.
struct CPU {
  static constexpr uint32_t RegSize = 16;
  uint32_t RegFile[RegSize] = {};
  uint32_t PC = 0;
  uint32_t NextPC = 0;
  uint32_t Run = 0;
  bool DumpInstrs = false;

  bool Execute(Binary &Bin, std::string &ErrorMsg); // режим 1
  std::string dumpStatus(Binary &Bin);

  static CPU *C;
  static void setCPU(CPU *Cpu) { C = Cpu; }

#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  static void do_##Name_(uint32_t R1, uint32_t R2, uint32_t R3Imm) {           \
    if (C->DumpInstrs)                                                         \
      llvm::outs() << #Name_ "\n";                                             \
    Execute_;                                                                  \
  }
#include "ISA.h"
#undef ISA_
};
#endif // CPU_H

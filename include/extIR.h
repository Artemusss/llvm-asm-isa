#ifndef EXTIR_H
#define EXTIR_H
#include "bin.h"
#include "cpu.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

// Режим 2 (задание п.3): IR, в котором КАЖДАЯ инструкция, кроме потока
// управления, превращается в call эмулирующей функции do_<Name>; поток
// управления (BRNZ/JMP/HALT) строится нативным IR. Регистровый файл — глобальный
// массив, отображённый на CPU::RegFile.
struct ExtIR {
  llvm::LLVMContext context;
  llvm::Module *module = nullptr;
  llvm::Function *mainFunc = nullptr;
  llvm::GlobalVariable *regFile = nullptr;
  llvm::Type *voidType = nullptr;
  llvm::Type *int32Type = nullptr;

  void buildIR(Binary &Bin);
  void dumpIR();
  bool verifyIR();
  void executeIR(CPU &Cpu);
};
#endif // EXTIR_H

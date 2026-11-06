#include "include/bin.h"
#include "include/cpu.h"
#include "include/extIR.h"
#include "include/fullIR.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
using namespace llvm;

// Симулятор собственной ISA. Аргументы: <asm-файл> <режим 1-3> [out.ll]
//   1 : симуляция (п.5*)
//   2 : IR с эмулирующими функциями (п.3)
//   3 : полный IR-эквивалент (п.4); с 3-м аргументом — выгрузка IR в файл.
int main(int argc, char *argv[]) {
  if (argc != 3 && argc != 4) {
    outs() << "Usage: " << argv[0]
           << " <asm-file> <mode 1-3> [out.ll]\n"
              "  1: simulation (emulating funcs, no IR)\n"
              "  2: IR with emulating-function calls + ExecutionEngine\n"
              "  3: full IR equivalents + ExecutionEngine (or dump to out.ll)\n";
    return 1;
  }

  Instr::prepareDicts();
  Binary Bin;
  std::string ErrorMsg;
  if (Bin.readFile(argv[1], ErrorMsg)) {
    outs() << "[ASM READ ERROR] " << ErrorMsg << '\n';
    return 1;
  }
  outs() << "[BasicBlocks]\n" << Bin.writeBBs();
  outs() << "[Disassembly]\n" << Bin.writeInstrs();

  CPU Cpu;
  int Mode = atoi(argv[2]);
  switch (Mode) {
  case 1:
    if (Cpu.Execute(Bin, ErrorMsg)) {
      outs() << "[CPU RUN ERROR] " << ErrorMsg << '\n';
      return 1;
    }
    break;
  case 2: {
    ExtIR Ext;
    Ext.buildIR(Bin);
    Ext.dumpIR();
    if (Ext.verifyIR())
      return 1;
    Ext.executeIR(Cpu);
    break;
  }
  case 3: {
    FullIR Full;
    Full.buildIR(Bin);
    Full.dumpIR();
    if (Full.verifyIR())
      return 1;
    if (argc == 4) {
      outs() << "Print LLVM IR to " << argv[3] << "\n";
      if (Full.printIR(argv[3], ErrorMsg)) {
        outs() << "[IR DUMP ERROR] " << ErrorMsg << "\n";
        return 1;
      }
    } else {
      Full.executeIR(Cpu);
    }
    break;
  }
  default:
    outs() << "[ERROR] mode must be 1-3\n";
    return 1;
  }
  return 0;
}

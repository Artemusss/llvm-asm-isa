#include "include/fullIR.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <system_error>
#include <unordered_map>

using namespace llvm;

// Режим 3 (п.4): полноценный IR-эквивалент каждой инструкции (IRGenExecute).
// Регистровый файл — alloca [16 x i32]; графика — прямые call @simPutPixel /
// @simFlush. Это тот же IR, что порождает clang для app.c (после opt -O2
// память-в-регистры даёт phi-форму из задания 1).
void FullIR::buildIR(Binary &Bin) {
  module = new Module("full", context);
  IRBuilder<> builder(context);
  voidType = Type::getVoidTy(context);
  int32Type = Type::getInt32Ty(context);

  FunctionType *funcType = FunctionType::get(voidType, false);
  mainFunc = Function::Create(funcType, Function::ExternalLinkage, "app", module);

  // Внешние графические функции.
  ArrayRef<Type *> i32x3 = {int32Type, int32Type, int32Type};
  FunctionType *i32x3Type = FunctionType::get(voidType, i32x3, false);
  FunctionCallee simPutPixelFunc =
      module->getOrInsertFunction("simPutPixel", i32x3Type);
  FunctionType *voidFnType = FunctionType::get(voidType, false);
  FunctionCallee simFlushFunc =
      module->getOrInsertFunction("simFlush", voidFnType);

  // Блоки в порядке PC: блок PC 0 должен быть первым блоком функции.
  std::unordered_map<uint32_t, BasicBlock *> BBMap;
  for (auto &BB : Bin.PC2BBName)
    BBMap[BB.first] = BasicBlock::Create(context, BB.second, mainFunc);

  uint32_t PC = 0;
  builder.SetInsertPoint(BBMap[0]);
  // Регистровый файл в стеке.
  ArrayType *regFileType = ArrayType::get(int32Type, CPU::RegSize);
  Value *regFile = builder.CreateAlloca(regFileType, nullptr, "regFile");

  for (Instr &I : Bin.Instrs) {
    switch (I.Op) {
    default:
      break;
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  case (Opcode_):                                                              \
    IRGenExecute_; /* BRNZ/JMP делают PC++/SetInsertPoint/continue сами */      \
    break;
#include "include/ISA.h"
#undef ISA_
    }
    PC++;
    auto BB = BBMap.find(PC);
    if (BB != BBMap.end()) {
      builder.CreateBr(BB->second); // падение в следующий блок
      builder.SetInsertPoint(BB->second);
    }
  }
}

void FullIR::executeIR(CPU &Cpu) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();

  ExecutionEngine *ee = EngineBuilder(std::unique_ptr<Module>(module)).create();
  ee->InstallLazyFunctionCreator([](const std::string &fn) -> void * {
    if (fn == "simFlush") return reinterpret_cast<void *>(simFlush);
    if (fn == "simPutPixel") return reinterpret_cast<void *>(simPutPixel);
    if (fn == "simRand") return reinterpret_cast<void *>(simRand);
    outs() << "[EE] Can't find function " << fn << '\n';
    return nullptr;
  });
  ee->finalizeObject();

  simInit();
  ArrayRef<GenericValue> noargs;
  outs() << "\n#[Running mode 3]\n";
  ee->runFunction(mainFunc, noargs);
  outs() << "#[Done mode 3]\n";
  simExit();
}

// Выгрузить сгенерированный IR в *.ll — его можно слинковать с настоящим
// sim + start.c:  clang out.ll app/sim_stub.c app/start.c -I include
bool FullIR::printIR(const std::string &FileName, std::string &ErrorMsg) {
  std::error_code EC;
  raw_fd_ostream Out(FileName, EC);
  if (EC) {
    ErrorMsg = "Can't print LLVM IR to " + FileName;
    return true;
  }
  module->print(Out, nullptr);
  return false;
}

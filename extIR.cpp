#include "include/extIR.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>
#include <unordered_map>

using namespace llvm;

// Режим 2 (п.3): каждая НЕ-управляющая инструкция -> call do_<Name>;
// поток управления строится нативным IR. regFile — глобальный массив,
// отображаемый на CPU::RegFile при исполнении.
void ExtIR::buildIR(Binary &Bin) {
  module = new Module("ext", context);
  IRBuilder<> builder(context);
  voidType = Type::getVoidTy(context);
  int32Type = Type::getInt32Ty(context);

  ArrayType *regFileType = ArrayType::get(int32Type, CPU::RegSize);
  module->getOrInsertGlobal("regFile", regFileType);
  regFile = module->getNamedGlobal("regFile");

  FunctionType *funcType = FunctionType::get(voidType, false);
  mainFunc = Function::Create(funcType, Function::ExternalLinkage, "app", module);

  // Прототипы эмулирующих функций do_<Name>(i32, i32, i32).
  ArrayRef<Type *> i32x3 = {int32Type, int32Type, int32Type};
  FunctionType *doType = FunctionType::get(voidType, i32x3, false);
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  FunctionCallee Callee##Name_ =                                               \
      module->getOrInsertFunction("do_" #Name_, doType);
#include "include/ISA.h"
#undef ISA_

  // Блоки создаём в порядке PC (PC2BBName упорядочен), чтобы блок с PC 0 стал
  // первым блоком функции — его требует LLVM (entry без предшественников).
  std::unordered_map<uint32_t, BasicBlock *> BBMap;
  for (auto &BB : Bin.PC2BBName)
    BBMap[BB.first] = BasicBlock::Create(context, BB.second, mainFunc);

  uint32_t PC = 0;
  builder.SetInsertPoint(BBMap[0]);
  for (Instr &I : Bin.Instrs) {
    // 1) Тело: вызов эмулирующей функции — ТОЛЬКО для не-управляющих инструкций.
    if (!Instr::isControlFlow(I.Op)) {
      Value *args[] = {builder.getInt32(I.R1), builder.getInt32(I.R2),
                       builder.getInt32(I.R3Imm)};
      switch (I.Op) {
      default:
        break;
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  case (Opcode_):                                                              \
    builder.CreateCall(Callee##Name_, args);                                   \
    break;
#include "include/ISA.h"
#undef ISA_
      }
    }

    // 2) Поток управления / переход на следующий блок.
    PC++;
    auto BB = BBMap.find(PC);
    switch (I.Op) {
    default:
      if (BB != BBMap.end())
        builder.CreateBr(BB->second); // падение в следующий блок
      break;
    case Instr::BRNZ: {
      Value *flagP = builder.CreateConstGEP2_32(regFileType, regFile, 0, I.R1);
      Value *cond = builder.CreateTrunc(
          builder.CreateLoad(int32Type, flagP), builder.getInt1Ty());
      builder.CreateCondBr(cond, BBMap[I.R3Imm], BB->second);
      break;
    }
    case Instr::JMP:
      builder.CreateBr(BBMap[I.R3Imm]);
      break;
    case Instr::HALT:
      builder.CreateRetVoid();
      break;
    }
    if (BB != BBMap.end())
      builder.SetInsertPoint(BB->second);
  }
}

void ExtIR::dumpIR() {
  outs() << "\n[LLVM IR mode 2 (emulating calls)]:\n";
  module->print(outs(), nullptr);
  outs() << '\n';
}

bool ExtIR::verifyIR() {
  bool bad = verifyFunction(*mainFunc, &outs());
  outs() << "[VERIFICATION] " << (bad ? "FAIL\n" : "OK\n");
  return bad;
}

void ExtIR::executeIR(CPU &Cpu) {
  InitializeNativeTarget();
  InitializeNativeTargetAsmPrinter();

  ExecutionEngine *ee = EngineBuilder(std::unique_ptr<Module>(module)).create();
  ee->InstallLazyFunctionCreator([](const std::string &fn) -> void * {
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  if (fn == "do_" #Name_)                                                      \
    return reinterpret_cast<void *>(CPU::do_##Name_);
#include "include/ISA.h"
#undef ISA_
    outs() << "[EE] Can't find function " << fn << '\n';
    return nullptr;
  });
  ee->addGlobalMapping(regFile, (void *)Cpu.RegFile);
  ee->finalizeObject();

  simInit();
  CPU::setCPU(&Cpu);

  ArrayRef<GenericValue> noargs;
  outs() << "\n#[Running mode 2]\n";
  ee->runFunction(mainFunc, noargs);
  outs() << "#[Done mode 2]\n";
  simExit();
}

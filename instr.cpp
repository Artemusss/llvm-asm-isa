#include "include/instr.h"

std::map<std::string, uint32_t> Instr::Str2Op;
std::map<uint32_t, std::string> Instr::Op2Str;

void Instr::prepareDicts() {
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  Str2Op[#Name_] = Opcode_;                                                    \
  Op2Str[Opcode_] = #Name_;
#include "include/ISA.h"
#undef ISA_
}

uint32_t Instr::getOpcode(const std::string &InstrName) {
  auto It = Str2Op.find(InstrName);
  return It == Str2Op.end() ? 0 : It->second; // 0 => не инструкция (метка)
}

bool Instr::isControlFlow(uint32_t Op) {
  return Op == BRNZ || Op == JMP || Op == HALT;
}

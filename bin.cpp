#include "include/bin.h"
#include "include/instr.h"
#include <cstdint>
#include <fstream>
#include <sstream>

// Убрать комментарии (';' или '//' до конца строки) — чтобы ASM можно было
// снабжать пояснениями, а токен-парсер их не видел.
static std::string stripComments(std::istream &In) {
  std::stringstream Out;
  std::string Line;
  while (std::getline(In, Line)) {
    auto p = Line.find(';');
    if (p != std::string::npos) Line.erase(p);
    p = Line.find("//");
    if (p != std::string::npos) Line.erase(p);
    Out << Line << '\n';
  }
  return Out.str();
}

// Первый проход: собрать метки (токены, не являющиеся инструкциями) и их PC.
bool Binary::searchBBs(std::istream &InputFile, std::string &ErrorMsg) {
  std::string Name, Arg;
  uint32_t PC = 0;
  while (InputFile >> Name) {
    uint32_t Opcode = Instr::getOpcode(Name);
    switch (Opcode) {
    default: // метка
      if (BBName2PC.count(Name)) {
        ErrorMsg = "Repetition of label: " + Name;
        return true;
      }
      if (PC2BBName.count(PC)) {
        ErrorMsg = "2 labels on one PC: " + Name + " and " + PC2BBName[PC];
        return true;
      }
      BBName2PC[Name] = PC;
      PC2BBName[PC] = Name;
      continue;
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  case (Opcode_):                                                              \
    SkipArgs_;                                                                 \
    break;
#include "include/ISA.h"
#undef ISA_
    }
    PC++;
  }
  return false;
}

// Второй проход: разобрать инструкции с аргументами (метки уже известны).
bool Binary::readInstrs(std::istream &InputFile, std::string &ErrorMsg) {
  std::string Name, Arg;
  while (InputFile >> Name) {
    uint32_t Opcode = Instr::getOpcode(Name);
    Instr I;
    I.Op = Opcode;
    switch (Opcode) {
    default:
      if (BBName2PC.count(Name)) continue; // метка — пропускаем
      ErrorMsg = "Wrong Opcode for " + Name;
      return true;
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  case (Opcode_):                                                              \
    ReadArgs_;                                                                 \
    break;
#include "include/ISA.h"
#undef ISA_
    }
    Instrs.push_back(I);
  }
  return false;
}

std::string Binary::writeBBs() {
  std::stringstream Stream;
  for (auto &it : BBName2PC)
    Stream << "  " << it.second << ": " << it.first << '\n';
  return Stream.str();
}

std::string Binary::writeInstrs() {
  std::stringstream Stream;
  uint32_t PC = 0;
  for (Instr &I : Instrs) {
    auto Label = PC2BBName.find(PC);
    if (Label != PC2BBName.end())
      Stream << Label->second << '\n';
    std::map<uint32_t, std::string> &PC2BBName = this->PC2BBName; // для WRITE_LABEL
    switch (I.Op) {
    default:
      Stream << "\nWrong Opcode: " << I.Op << '\n';
      break;
#define ISA_(Opcode_, Name_, SkipArgs_, ReadArgs_, WriteArgs_, Execute_,       \
             IRGenExecute_)                                                    \
  case (Opcode_):                                                              \
    Stream << "  " << #Name_;                                                  \
    WriteArgs_;                                                                \
    Stream << '\n';                                                            \
    break;
#include "include/ISA.h"
#undef ISA_
    }
    PC++;
  }
  return Stream.str();
}

bool Binary::readFile(const std::string &FileName, std::string &ErrorMsg) {
  std::ifstream Input(FileName);
  if (!Input.is_open()) {
    ErrorMsg = "Can't open " + FileName;
    return true;
  }
  std::string Text = stripComments(Input);

  std::string Local;
  std::istringstream Pass1(Text);
  if (searchBBs(Pass1, Local)) {
    ErrorMsg = "In searchBBs: " + Local;
    return true;
  }
  std::istringstream Pass2(Text);
  if (readInstrs(Pass2, Local)) {
    ErrorMsg = "In readInstrs: " + Local;
    return true;
  }
  return false;
}

#ifndef BIN_H
#define BIN_H
#include "instr.h"
#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

// Загруженная программа: поток инструкций + таблицы меток (имя<->PC).
struct Binary {
  std::vector<Instr> Instrs;
  std::map<std::string, uint32_t> BBName2PC; // метка -> индекс инструкции
  std::map<uint32_t, std::string> PC2BBName; // индекс инструкции -> метка

  bool readFile(const std::string &FileName, std::string &ErrorMsg);
  std::string writeBBs();
  std::string writeInstrs();

private:
  // Оба прохода работают с istream, поэтому можно скормить текст без комментариев.
  bool searchBBs(std::istream &InputFile, std::string &ErrorMsg);
  bool readInstrs(std::istream &InputFile, std::string &ErrorMsg);
};
#endif // BIN_H

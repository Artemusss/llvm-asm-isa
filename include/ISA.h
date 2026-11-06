// ============================================================================
//  ISA.h — таблица инструкций (X-macro). Один источник правды для:
//    * словарей имя<->опкод        (instr.cpp)
//    * парсера/печати ассемблера    (bin.cpp:   SkipArgs / ReadArgs / WriteArgs)
//    * симуляции                    (cpu.cpp:   Execute)       -> режим 1 (5*)
//    * IR с эмулирующими функциями  (extIR.cpp)                -> режим 2 (п.3)
//    * генерации чистого IR         (fullIR.cpp: IRGenExecute) -> режим 3 (п.4)
//
//  Формат записи:
//  ISA_(Opcode, Name, SkipArgs, ReadArgs, WriteArgs, Execute, IRGenExecute)
//
//  Регистры r0..r15 (16 шт). R1 — приёмник, R2/R3Imm — источники/имм/метка.
//  Опкоды НЕнулевые: 0 зарезервирован под "не инструкция" (метка) в getOpcode.
//
//  Набор выведен из статистики задания 2 (горячие рёбра use-трассы) и из
//  -O2 LLVM IR приложения app.c. Подробное обоснование — в ISA.md.
// ============================================================================

// ---- SkipArgs: пропустить аргументы при первом проходе (поиск меток) --------
#define SKIP_3ARGS { InputFile >> Arg >> Arg >> Arg; }
#define SKIP_2ARGS { InputFile >> Arg >> Arg; }
#define SKIP_1ARGS { InputFile >> Arg; }

// ---- ReadArgs: разобрать аргументы во втором проходе -------------------------
#define READ_REG(Reg) { InputFile >> Arg; I.Reg = std::stoi(Arg.substr(1)); }
#define READ_IMM      { InputFile >> Arg; I.R3Imm = (uint32_t)std::stoll(Arg); }
#define READ_LABEL                                                             \
  {                                                                            \
    InputFile >> Arg;                                                          \
    if (BBName2PC.find(Arg) == BBName2PC.end()) {                              \
      ErrorMsg = std::string("Can't find label: " + Arg);                      \
      return true;                                                             \
    }                                                                          \
    I.R3Imm = BBName2PC[Arg];                                                  \
  }
#define READ_3REGS      { READ_REG(R1) READ_REG(R2) READ_REG(R3Imm) }
#define READ_2REGS_IMM  { READ_REG(R1) READ_REG(R2) READ_IMM }
#define READ_REG_LABEL  { READ_REG(R1) READ_LABEL }
#define READ_REG_IMM    { READ_REG(R1) READ_IMM }

// ---- WriteArgs: напечатать инструкцию обратно в ассемблер --------------------
#define WRITE_REG(Reg)  { Stream << " r" << I.Reg; }
#define WRITE_IMM       { Stream << " " << (int32_t)I.R3Imm; }
#define WRITE_LABEL     { Stream << " " << PC2BBName[I.R3Imm]; }
#define WRITE_3REGS     { WRITE_REG(R1) WRITE_REG(R2) WRITE_REG(R3Imm) }
#define WRITE_2REGS_IMM { WRITE_REG(R1) WRITE_REG(R2) WRITE_IMM }
#define WRITE_REG_LABEL { WRITE_REG(R1) WRITE_LABEL }
#define WRITE_REG_IMM   { WRITE_REG(R1) WRITE_IMM }

// ---- Хелперы для генерации IR (регистры = ячейки массива regFile) ------------
#define GEP2_32(Arg) builder.CreateConstGEP2_32(regFileType, regFile, 0, Arg)
#define LOAD_REG(Arg) builder.CreateLoad(int32Type, GEP2_32(Arg))
#define GEN_IMM(Arg) builder.getInt32(Arg)

// ============================================================================
//  ИНСТРУКЦИИ
// ============================================================================

//  HALT — конец программы.                               [control flow]
//  IR-эквивалент: ret void.
ISA_(0xF0, HALT, {}, {}, {},
     { C->Run = 0; },
     { builder.CreateRetVoid(); })

//  FLUSH — показать кадр (simFlush).                      [граф. вызов]
//  IR из app.c: call void @simFlush()
ISA_(0xF1, FLUSH, {}, {}, {},
     { simFlush(); },
     { builder.CreateCall(simFlushFunc); })

//  LDI rd, imm — загрузка константы (инициализация счётчиков нулём и т.п.)
ISA_(0x20, LDI, SKIP_2ARGS, READ_REG_IMM, WRITE_REG_IMM,
     { C->RegFile[R1] = R3Imm; },
     { builder.CreateStore(GEN_IMM(I.R3Imm), GEP2_32(I.R1)); })

//  MUL rd, ra, rb — rd = ra * rb.
//  IR: mul nuw nsw i32  (рёбра mul<-mul, mul<-add, add<-mul из статистики)
ISA_(0x30, MUL, SKIP_3ARGS, READ_3REGS, WRITE_3REGS,
     { C->RegFile[R1] = C->RegFile[R2] * C->RegFile[R3Imm]; },
     { builder.CreateStore(builder.CreateMul(LOAD_REG(I.R2), LOAD_REG(I.R3Imm)),
                           GEP2_32(I.R1)); })

//  ADDI rd, ra, imm — rd = ra + imm (знаковый имм).
//  IR: add nsw i32 %.., -16777216  (формирование ARGB-цвета, 0xFF000000)
ISA_(0x40, ADDI, SKIP_3ARGS, READ_2REGS_IMM, WRITE_2REGS_IMM,
     { C->RegFile[R1] = C->RegFile[R2] + (int32_t)R3Imm; },
     { builder.CreateStore(builder.CreateAdd(LOAD_REG(I.R2), GEN_IMM(I.R3Imm)),
                           GEP2_32(I.R1)); })

//  INEQ rd, rc, imm — слитые инкремент+сравнение: rc = rc + 1; rd = (rc != imm).
//  IR: add nuw nsw %i,1 ; icmp eq ; (используется ветвлением) —
//  покрывает горячую тройку рёбер add<-add, icmp<-add, br<-icmp из статистики.
ISA_(0x50, INEQ, SKIP_3ARGS, READ_2REGS_IMM, WRITE_2REGS_IMM,
     { C->RegFile[R1] = (++C->RegFile[R2]) != R3Imm; },
     {
       Value *p = GEP2_32(I.R2);
       Value *inc = builder.CreateAdd(builder.CreateLoad(int32Type, p),
                                      GEN_IMM(1));
       builder.CreateStore(inc, p);
       Value *ne = builder.CreateZExt(
           builder.CreateICmpNE(inc, GEN_IMM(I.R3Imm)), int32Type);
       builder.CreateStore(ne, GEP2_32(I.R1));
     })

//  PIXEL rx, ry, rc — simPutPixel(rx, ry, rc).            [граф. вызов]
//  IR из app.c: call void @simPutPixel(i32 x, i32 y, i32 color)
ISA_(0x60, PIXEL, SKIP_3ARGS, READ_3REGS, WRITE_3REGS,
     { simPutPixel(C->RegFile[R1], C->RegFile[R2], C->RegFile[R3Imm]); },
     { builder.CreateCall(simPutPixelFunc,
                          {LOAD_REG(I.R1), LOAD_REG(I.R2), LOAD_REG(I.R3Imm)}); })

//  BRNZ rc, label — если rc != 0, перейти на label.       [control flow]
//  IR: br i1 (trunc rc), label_target, label_fallthrough
ISA_(0x70, BRNZ, SKIP_2ARGS, READ_REG_LABEL, WRITE_REG_LABEL,
     { if (C->RegFile[R1]) C->NextPC = R3Imm; },
     {
       PC++;
       builder.CreateCondBr(
           builder.CreateTrunc(LOAD_REG(I.R1), builder.getInt1Ty()),
           BBMap[I.R3Imm], BBMap[PC]);
       builder.SetInsertPoint(BBMap[PC]);
       continue;
     })

//  JMP label — безусловный переход.                        [control flow]
ISA_(0x80, JMP, SKIP_1ARGS, READ_LABEL, WRITE_LABEL,
     { C->NextPC = R3Imm; },
     {
       PC++;
       builder.CreateBr(BBMap[I.R3Imm]);
       builder.SetInsertPoint(BBMap[PC]);
       continue;
     })

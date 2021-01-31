#include "chip8.hpp"

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <limits.h>
#include <raylib.h>

static inline uint16_t barrelShiftLeft(uint16_t val, uint16_t amount) {
  const uint16_t mask = (CHAR_BIT * sizeof(val) - 1);
  amount &= mask;
  return (val << amount) | (val >> ((-amount) & mask));
}

static inline uint16_t barrelShiftRight(uint16_t val, uint16_t amount) {
  const uint16_t mask = (CHAR_BIT * sizeof(val) - 1);
  amount &= mask;
  return (val >> amount) | (val << ((-amount) & mask));
}

static void Op0(Chip8 *c, uint8_t *instr) {
  switch (instr[1]) {
  case 0xE0:
    for (size_t i = 0; i < WIN_SIZE; i++) {
      c->FrameBuffer[i] = false;
    }
    return;
  case 0xEE:
    if (c->SP < 0) {
      std::cout << "Stack underflow" << std::endl;
      exit(1);
    }
    c->PC = c->Stack[c->SP - 1];
    c->SP--;
    return;
  }
}

static void JMP(Chip8 *c, uint8_t *instr) {
  uint16_t addr = instr[1];
  addr |= (instr[0] & 0xF) << 8;
  c->PC = addr;
}

static void CALL(Chip8 *c, uint8_t *instr) {
  uint16_t addr = instr[1];
  addr |= (instr[0] & 0xF) << 8;
  if (c->SP >= STACK_SIZE) {
    std::cout << "Stack overflow" << std::endl;
    exit(1);
  }
  c->SP++;
  c->Stack[c->SP - 1] = c->PC;
  c->PC = addr;
}

static void SE(Chip8 *c, uint8_t *instr) {
  uint8_t reg = instr[0] & 0xF;
  if (c->V[reg] == instr[1]) {
    c->PC += 2;
  }
}

static void SNE(Chip8 *c, uint8_t *instr) {
  uint8_t reg = instr[0] & 0xF;
  if (c->V[reg] != instr[1]) {
    c->PC += 2;
  }
}

static void SEREG(Chip8 *c, uint8_t *instr) {
  uint8_t a = instr[0] & 0xF;
  uint8_t b = instr[1] >> 4;
  if (c->V[a] == c->V[b]) {
    c->PC += 2;
  }
}

static void LDI(Chip8 *c, uint8_t *instr) {
  uint8_t reg = instr[0] & 0xF;
  c->V[reg] = instr[1];
}

static void ADDI(Chip8 *c, uint8_t *instr) {
  uint8_t reg = instr[0] & 0xF;
  c->V[reg] += instr[1];
}

static void Op8(Chip8 *c, uint8_t *instr) {
  uint8_t lowNibble = instr[1] & 0xF;
  uint8_t x = instr[0] & 0xF;
  uint8_t y = instr[1] >> 4;
  switch (lowNibble) {
  case 0x0: // LDR
  {
    c->V[x] = c->V[y];
    return;
  }
  case 0x1: // OR
  {
    c->V[x] |= c->V[y];
    return;
  }
  case 0x2: // AND
  {
    c->V[x] &= c->V[y];
    return;
  }
  case 0x3: // XOR
  {
    c->V[x] ^= c->V[y];
    return;
  }
  case 0x4: // ADD SET CARRY
  {
    uint16_t res = c->V[x] + c->V[y];
    if (res > 0xFF) {
      c->V[0xF] = 1;
    } else {
      c->V[0xF] = 0;
    }
    c->V[x] = res & 0xFF;
    return;
  }
  case 0x5: // SUB SET NOT BORROW
  {
    if (c->V[x] > c->V[y]) {
      c->V[0xF] = 1;
    } else {
      c->V[0xF] = 0;
    }
    c->V[x] -= c->V[y];
    return;
  }
  case 0x6: // SHR
  {
    c->V[0xF] = c->V[x] & 1;
    c->V[x] = c->V[x] >> 1;
    return;
  }
  case 0x7: // SUBN SET NOT BORROW
  {
    if (c->V[y] > c->V[x]) {
      c->V[0xF] = 1;
    } else {
      c->V[0xF] = 0;
    }
    c->V[x] = c->V[y] - c->V[x];
    return;
  }
  case 0xE: // SHL
  {
    if ((c->V[x] & 0b1000'0000) != 0) {
      c->V[0xF] = 1;
    } else {
      c->V[0xF] = 0;
    }
    c->V[x] = c->V[x] << 1;
    return;
  }
  }
}

static void SNEREG(Chip8 *c, uint8_t *instr) {
  uint8_t a = instr[0] & 0xF;
  uint8_t b = instr[1] >> 4;
  if (c->V[a] != c->V[b]) {
    c->PC += 2;
  }
}

// LD into reg I, Immediate
static void LDII(Chip8 *c, uint8_t *instr) {
  uint16_t val = instr[1];
  val |= (instr[0] & 0xF) << 8;
  c->I = val;
}

// JP V0, addr
// jump to V0 + addr
static void JPOff(Chip8 *c, uint8_t *instr) {
  uint16_t val = instr[1];
  val |= (instr[0] & 0xF) << 8;
  c->PC = c->V[0] + val;
}

// RND creates an 8 bit random number generated by XOR shift
// and with instr Cxkk, sets Vx = kk
static void RND(Chip8 *c, uint8_t *instr) {
  c->SEED ^= barrelShiftLeft(c->SEED, 13);
  c->SEED ^= barrelShiftRight(c->SEED, 17);
  c->SEED ^= barrelShiftLeft(c->SEED, 5);
  if (c->SEED == 0) {
    c->SEED = static_cast<uint16_t>(time(NULL));
  }

  uint8_t reg = instr[0] & 0xF;
  c->V[reg] = instr[1] & c->SEED;
}

static void DRW(Chip8 *c, uint8_t *instr) {
  uint8_t x = c->V[instr[0] & 0xF];
  uint8_t y = c->V[instr[1] >> 4];
  const uint8_t count = instr[1] & 0xF;
  const uint8_t sprite_start_x = x;
  for (uint8_t i = 0; i < count; i++) {
    uint8_t value = c->mem->get(c->I + i);
    x = sprite_start_x;
    for (size_t j = sizeof(uint8_t) * 8; j > 0; j--) {
      size_t idx = x + (y * WIN_SIZE_X);
      x = (x + 1) % WIN_SIZE_X;
      bool bit = (value & (1 << j)) != 0;
      bool oldVal = c->FrameBuffer[idx];
      bool newVal = oldVal ^ bit;
      c->FrameBuffer[idx] = newVal;
      if ((oldVal == 1) && (newVal == 0)) {
        c->V[0xF] = 1;
      } else {
        c->V[0xF] = 0;
      }
    }
    y = (y + 1) % WIN_SIZE_Y;
  }
}

// Skip instruction if key pressed/not pressed
static void SKPP(Chip8 *c, uint8_t *instr) {
  uint8_t reg = instr[0] >> 4;
  bool keyPressed = c->KeyPad[c->V[reg]];
  switch (instr[1]) {
  case 0x9E: // Skip next instruction if key with value of Vx is pressed
  {
    if (keyPressed) {
      c->PC += 2;
    }
    return;
  }
  case 0xA1: // Skip next instruction if key with value of Vx is not pressed
  {
    if (!keyPressed) {
      c->PC += 2;
    }
    return;
  }
  }
}

static void OpF(Chip8 *c, uint8_t *instr) {
  uint8_t reg = instr[0] & 0xF;
  switch (instr[1]) {
  case 0x07: // LD Vx, DT
  {
    c->V[reg] = c->DT;
    return;
  }
  case 0x0A: // LD Vx, K
  {
    // wait for a key press, store the value of key into Vx
    // pause CPU till key is pressed
    c->inputReg = reg;
    c->Paused = true;
    return;
  }
  case 0x15: // LD DT, xVx
  {
    c->DT = c->V[reg];
    return;
  }
  case 0x18: // LD ST, Vx
  {
    c->ST = c->V[reg];
    return;
  }
  case 0x1E: // ADD I, Vx
  {
    c->I = c->I + c->V[reg];
    return;
  }
  case 0x29: // LD F, Vx
  {
    // set the value of I to the location for the hex sprite corresponding to
    // the value of Vx
    c->I = c->V[reg] * CHAR_SPRITE_SIZE;
    return;
  }
  case 0x33: // LD B, Vx
  {
    uint8_t val = c->V[reg];
    uint8_t ones = val % 10;
    val /= 10;
    uint8_t tens = val % 10;
    val /= 10;
    uint8_t hundreds = val % 10;
    c->mem->set(c->I, hundreds);
    c->mem->set(c->I + 1, tens);
    c->mem->set(c->I + 2, ones);
    return;
  }
  case 0x55: // LD [I], Vx
  {
    for (size_t i = 0; i < 0x10; i++) {
      c->mem->set(c->I + i, c->V[i]);
    }
    return;
  }
  case 0x65: // LD Vx, [I]
  {
    for (size_t i = 0; i < 0x10; i++) {
      c->V[i] = c->mem->get(c->I + i);
    }
    return;
  }
  }
}

static void NOOP(Chip8 *c, uint8_t *instr) {}

Chip8::Chip8(Memory *m) : PC(0x200), mem(m) {
  opcodes[0x0] = &Op0;
  opcodes[0x1] = &JMP;
  opcodes[0x2] = &CALL;
  opcodes[0x3] = &SE;
  opcodes[0x4] = &SNE;
  opcodes[0x5] = &SEREG;
  opcodes[0x6] = &LDI;
  opcodes[0x7] = &ADDI;
  opcodes[0x8] = &Op8;
  opcodes[0x9] = &SNEREG;
  opcodes[0xA] = &LDII;
  opcodes[0xB] = &JPOff;
  opcodes[0xC] = &RND;
  opcodes[0xD] = &DRW;
  opcodes[0xE] = &SKPP;
  opcodes[0xF] = &OpF;

  for (size_t i = 0; i < 0x10; i++) {
    V[i] = 0;
  }

  for (size_t i = 0; i < 0x10; i++) {
    KeyPad[i] = 0;
  }

  for (size_t i = 0; i < STACK_SIZE; i++) {
    Stack[i] = 0;
  }

  for (size_t i = 0; i < WIN_SIZE; i++) {
    FrameBuffer[i] = false;
  }

  // Write interpreter into memory
  // Char 0
  mem->set16(0x00, 0b11110000);
  mem->set16(0x02, 0b10010000);
  mem->set16(0x04, 0b10010000);
  mem->set16(0x06, 0b10010000);
  mem->set16(0x08, 0b11110000);
  // Char 1
  mem->set16(0x0A, 0b00100000);
  mem->set16(0x0C, 0b01100000);
  mem->set16(0x0E, 0b00100000);
  mem->set16(0x10, 0b00100000);
  mem->set16(0x12, 0b01110000);
  // Char 2
  mem->set16(0x14, 0b11110000);
  mem->set16(0x16, 0b00010000);
  mem->set16(0x18, 0b11110000);
  mem->set16(0x1A, 0b10000000);
  mem->set16(0x1C, 0b11110000);
  // Char 3
  mem->set16(0x1E, 0b11110000);
  mem->set16(0x20, 0b00010000);
  mem->set16(0x22, 0b11110000);
  mem->set16(0x24, 0b00010000);
  mem->set16(0x26, 0b11110000);
  // Char 4
  mem->set16(0x28, 0b10010000);
  mem->set16(0x2A, 0b10010000);
  mem->set16(0x2C, 0b11110000);
  mem->set16(0x2E, 0b00010000);
  mem->set16(0x30, 0b00010000);
  // Char 5
  mem->set16(0x32, 0b11110000);
  mem->set16(0x34, 0b10000000);
  mem->set16(0x36, 0b11110000);
  mem->set16(0x38, 0b00010000);
  mem->set16(0x3A, 0b11110000);
  // Char 6
  mem->set16(0x3C, 0b11110000);
  mem->set16(0x3E, 0b10000000);
  mem->set16(0x40, 0b11110000);
  mem->set16(0x42, 0b10010000);
  mem->set16(0x44, 0b11110000);
  // Char 7
  mem->set16(0x46, 0b11110000);
  mem->set16(0x48, 0b00010000);
  mem->set16(0x4A, 0b00100000);
  mem->set16(0x4C, 0b01000000);
  mem->set16(0x4E, 0b01000000);
  // Char 8
  mem->set16(0x50, 0b11110000);
  mem->set16(0x52, 0b10010000);
  mem->set16(0x54, 0b11110000);
  mem->set16(0x56, 0b10010000);
  mem->set16(0x58, 0b11110000);
  // Char 9
  mem->set16(0x5A, 0b11110000);
  mem->set16(0x5C, 0b10010000);
  mem->set16(0x5E, 0b11110000);
  mem->set16(0x60, 0b00010000);
  mem->set16(0x62, 0b11110000);
  // Char A
  mem->set16(0x64, 0b11110000);
  mem->set16(0x66, 0b10010000);
  mem->set16(0x68, 0b11110000);
  mem->set16(0x6A, 0b10010000);
  mem->set16(0x6C, 0b10010000);
  // Char B
  mem->set16(0x6E, 0b11110000);
  mem->set16(0x70, 0b10010000);
  mem->set16(0x72, 0b11100000);
  mem->set16(0x74, 0b10010000);
  mem->set16(0x76, 0b11110000);
  // Char C
  mem->set16(0x78, 0b11110000);
  mem->set16(0x7A, 0b10000000);
  mem->set16(0x7C, 0b10000000);
  mem->set16(0x7E, 0b10000000);
  mem->set16(0x80, 0b11110000);
  // Char D
  mem->set16(0x82, 0b11100000);
  mem->set16(0x84, 0b10010000);
  mem->set16(0x86, 0b10010000);
  mem->set16(0x88, 0b10010000);
  mem->set16(0x8A, 0b11100000);
  // Char E
  mem->set16(0x8C, 0b11110000);
  mem->set16(0x8E, 0b10000000);
  mem->set16(0x90, 0b11110000);
  mem->set16(0x92, 0b10000000);
  mem->set16(0x94, 0b11110000);
  // Char F
  mem->set16(0x96, 0b11110000);
  mem->set16(0x98, 0b10000000);
  mem->set16(0x9A, 0b11110000);
  mem->set16(0x9C, 0b10000000);
  mem->set16(0x9E, 0b10000000);
}

void Chip8::step() {
  if (Paused) {
    return;
  }
  IR[0] = mem->get(PC);
  IR[1] = mem->get(PC + 1);
  PC += 2;
  uint8_t op = IR[0] >> 4;
  opcodes[op](this, IR);
  if (ST != 0) {
    ST--;
  }
  if (DT != 0) {
    DT--;
  }
}

void Chip8::sendInput(uint8_t key, bool value) {
  KeyPad[key & 0xF] = value;
  if (value) {
    if (Paused) {
      V[inputReg] = key & 0xF;
      inputReg = 0;
    }
    Paused = false;
  }
}

void Chip8::drawScr(int sizeX, int sizeY) {
  Rectangle r;
  r.height = sizeY / WIN_SIZE_Y;
  r.width = sizeX / WIN_SIZE_X;
  for (int j = 0; j < WIN_SIZE_Y; j++) {
    for (int i = 0; i < WIN_SIZE_X; i++) {
      int idx = i + (j * WIN_SIZE_X);
      if (FrameBuffer[idx]) {
        r.x = i * r.width;
        r.y = j * r.height;
        DrawRectangleRec(r, DARKGREEN);
      }
    }
  }
}

void Chip8::drawReg(int winSizeX, int winSizeY) {
  const auto color = LIGHTGRAY;
  const int size = 20;
  int startY = winSizeY;
  char buf[512];
  sprintf(buf, "PC: %04x\tSP: %04x\tIR: %02x%02x", PC, SP, IR[0], IR[1]);
  DrawText(buf, 0, startY, size, color);
  startY += size;
  sprintf(buf, "I: %04x\tDT: %04x\t ST: %04x\tRND Seed: %04x", I, DT, ST, SEED);
  DrawText(buf, 0, startY, size, color);
  startY += size;
  int startX = 0;
  int hSize = 140;
  for (int i = 0; i < 0x10; i++) {
    sprintf(buf, "V[%02x]=%02x", i, V[i]);
    DrawText(buf, startX, startY, size, color);
    startX += hSize;
    if ((i + 1) % 4 == 0) {
      startY += size;
      startX = 0;
    }
  }

  startY = winSizeY;
  DrawText("Stack: ", winSizeX / 2, startY, size, color);
  startY += size;
  startX = winSizeX / 2;
  for (int i = 0; i < 0x10; i++) {
    sprintf(buf, "%04x\t", Stack[i]);
    DrawText(buf, startX, startY, size, color);
    startX += hSize;
    if ((i + 1) % 4 == 0) {
      startY += size;
      startX = winSizeX / 2;
    }
  }

  const int mem_render_size = 0x3D;
  startY = 0;
  startX = winSizeX;
  auto center = mem_start_render + (mem_render_size / 2);
  auto diff = PC - center;
  if (diff < 0) {
    diff = -diff;
  }
  if (diff > mem_render_size / 2) {
    mem_start_render = PC - (mem_render_size / 2);
  }
  hSize = 240;
  for (size_t i = 0; i < mem_render_size; i += 2) {
    uint8_t ir[2] = {mem->get(i + mem_start_render),
                     mem->get(i + 1 + mem_start_render)};
    uint16_t irL = (ir[0] << 8) | ir[1];
    sprintf(buf, "mem[%#04x:%#04x]=%04x\n", (uint8_t)i + mem_start_render,
            (uint8_t)i + 1 + mem_start_render, irL);
    auto c = color;
    if (i + mem_start_render == PC) {
      c = DARKGREEN;
    }
    DrawText(buf, startX, startY, size, c);
    DrawText(dissasemble(ir).c_str(), startX + hSize, startY, size, c);
    startY += size;
  }
}

void Chip8::printreg() {
  printf("PC: %#04x\tSP: %#04x\tIR: %#02x,%#02x\n", PC, SP, IR[0], IR[1]);
  printf(" I: %#04x\tDT: %#04x\t ST: %#04x\n", I, DT, ST);
  for (int i = 0; i < 0x10; i++) {
    printf("V[%#02x]=%#02x\t", i, V[i]);
    if ((i + 1) % 4 == 0) {
      printf("\n");
    }
  }
  printf("\nStack\n");
  for (int i = 0; i < 0x10; i++) {
    printf("%#04x\t", Stack[i]);
    if ((i + 1) % 4 == 0) {
      printf("\n");
    }
  }
  printf("\n\n");
}

std::string Chip8::dissasemble(uint8_t *instr) {
  char buf[128];
  uint8_t op = instr[0] >> 4;
  uint16_t addr3b = instr[1];
  addr3b |= (instr[0] & 0xF) << 8;
  uint8_t instr0_low = instr[0] & 0xF;
  uint8_t instr1_high = instr[1] >> 4;
  uint8_t instr1_low = instr[1] & 0xF;

  switch (op) {
  case 0x0: {
    if (instr[1] == 0xE0) {
      return "CLS";
    } else if (instr[1] == 0xEE) {
      return "RET";
    }
    break;
  }
  case 0x1: {
    sprintf(buf, "JP %08x", addr3b);
    return buf;
  }
  case 0x2: {
    sprintf(buf, "CALL %08x", addr3b);
    return buf;
  }
  case 0x3: {
    sprintf(buf, "SE V%02X, %04x", instr0_low, instr[1]);
    return buf;
  }
  case 0x4: {
    sprintf(buf, "SNE V%02x, %04x", instr0_low, instr[1]);
    return buf;
  }
  case 0x5: {
    sprintf(buf, "SE V%02x, V%02x", instr0_low, instr1_high);
    return buf;
  }
  case 0x6: {
    sprintf(buf, "LD V%02x, %04x", instr0_low, instr[1]);
    return buf;
  }
  case 0x7: {
    sprintf(buf, "ADD V%02x, %04x", instr0_low, instr[1]);
    return buf;
  }
  case 0x8: {
    switch (instr1_low) {
    case 0x0: {
      sprintf(buf, "LD V%02x, V%02x", instr0_low, instr1_high);
      return buf;
    }
    case 0x1: {
      sprintf(buf, "OR V%02x, V%02x", instr0_low, instr1_high);
      return buf;
    }
    case 0x2: {
      sprintf(buf, "AND V%02x, V%02x", instr0_low, instr1_high);
      return buf;
    }
    case 0x3: {
      sprintf(buf, "XOR V%02x, V%02x", instr0_low, instr1_high);
      return buf;
    }
    case 0x4: {
      sprintf(buf, "ADD V%02x, V%02x", instr0_low, instr1_high);
      return buf;
    }
    case 0x5: {
      sprintf(buf, "SUB V%02x, V%02x", instr0_low, instr1_high);
      return buf;
    }
    case 0x6: {
      sprintf(buf, "SHR V%02x", instr0_low);
      return buf;
    }
    case 0x7: {
      sprintf(buf, "SUBN V%02x, V%02x", instr0_low, instr1_high);
      return buf;
    }
    case 0xE: {
      sprintf(buf, "SHL V%02x", instr0_low);
      return buf;
    }
    }
  }
  case 0x9: {
    sprintf(buf, "SNE V%02x, V%02x", instr0_low, instr1_high);
    return buf;
  }
  case 0xA: {
    sprintf(buf, "LD I, %08x", addr3b);
    return buf;
  }
  case 0xB: {
    sprintf(buf, "JP V%02x, %04x", instr0_low, instr[1]);
    return buf;
  }
  case 0xC: {
    sprintf(buf, "RND V%02x, %04x", instr0_low, instr[1]);
    return buf;
  }
  case 0xD: {
    sprintf(buf, "DRW (V%02x V%02x), %02x", instr0_low, instr1_high,
            instr1_low);
    return buf;
  }
  case 0xE: {
    switch (instr[1]) {
    case 0x9E: {
      sprintf(buf, "SKPKP V%02x", instr0_low);
      return buf;
    }
    case 0xA1: {
      sprintf(buf, "SKPNKP V%02x", instr0_low);
      return buf;
    }
    }
  case 0xF: {
    switch (instr[1]) {
    case 0x07: {
      sprintf(buf, "LD V%02x, DT", instr0_low);
      return buf;
    }
    case 0x0A: {
      sprintf(buf, "LD V%02x, K", instr0_low);
      return buf;
    }
    case 0x15: {
      sprintf(buf, "LD DT, V%02x", instr0_low);
      return buf;
    }
    case 0x18: {
      sprintf(buf, "LD ST, V%02x", instr0_low);
      return buf;
    }
    case 0x1E: {
      sprintf(buf, "LD I, V%02x", instr0_low);
      return buf;
    }
    case 0x29: {
      sprintf(buf, "LD F, V%02x", instr0_low);
      return buf;
    }
    case 0x33: {
      sprintf(buf, "LD B, V%02x", instr0_low);
      return buf;
    }
    case 0x55: {
      sprintf(buf, "LD [I], V%02x", instr0_low);
      return buf;
    }
    case 0x65: {
      sprintf(buf, "LD V%02x, [I]", instr0_low);
      return buf;
    }
    }
  }
  }
  }
  return "";
}
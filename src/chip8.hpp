#ifndef CHIP8_HPP
#define CHIP8_HPP

#include "memory.hpp"
#include <cstdint>
#include <raylib.h>

const size_t STACK_SIZE = 0x10;
const size_t WIN_SIZE_X = 128;
const size_t WIN_SIZE_Y = 64;
const size_t WIN_SIZE = (WIN_SIZE_X * WIN_SIZE_Y);
const uint8_t CHAR_SPRITE_SIZE = 10; // bytes

class Chip8 {
public:
  Chip8(Memory *m);

  void step();
  void sendInput(uint8_t key, bool value);
  void drawScr(int sizeX, int sizeY);
  void drawReg(int winSizeX, int winSizeY);
  void printreg();
  std::string dissasemble(uint8_t *instr);

  uint8_t V[0x10]; // V general purpose registers addressed V0-VF
  uint16_t I = 0;  // I register
  uint8_t DT = 0;  // Delay Timer register
  uint8_t ST = 0;  // Sound Timer register

  uint16_t SEED = 0;
  uint16_t PC = 0; // Program Counter
  uint8_t SP = 0;  // Stack Pointer
  uint8_t IR[2];
  uint16_t Stack[STACK_SIZE]; // Stack allows for 16 levels of nested functions

  Memory *mem;
  bool FrameBuffer[WIN_SIZE];
  bool KeyPad[0x10];

  uint8_t inputReg = 0;
  bool Paused = false;

  uint16_t mem_start_render = 0;
  using op = void (*)(Chip8 *, uint8_t *);
  op opcodes[0x10];
};

#endif // CHIP8_HPP
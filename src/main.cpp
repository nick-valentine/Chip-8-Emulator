#include "chip8.hpp"
#include "memory.hpp"
//#include <SDL2/SDL.h>
#include <fstream>
#include <iostream>
#include <map>
#include <raylib.h>
#include <string>

enum class StepMode {
  RUN,
  SINGLE,
};

const int SCREEN_SIZE_MULTIPLIER = 8;
const int SCREEN_HEIGHT = WIN_SIZE_Y * SCREEN_SIZE_MULTIPLIER;
const int SCREEN_WIDTH = WIN_SIZE_X * SCREEN_SIZE_MULTIPLIER;
const int CPU_INFO_HEIGHT = 120;
const int CPU_INFO_WIDTH = 280;

void load_rom(Memory *mem, uint16_t starting_address, std::string filename);

int main() {
  InitWindow(SCREEN_WIDTH + CPU_INFO_WIDTH, SCREEN_HEIGHT + CPU_INFO_HEIGHT,
             "Chip-8-Emulator");
  SetTargetFPS(60);

  Memory mem(4096);
  Chip8 cpu(&mem);
  load_rom(&mem, 0x200, "./roms/test_opcode.ch8");
  // load_rom(&mem, 0x200, "./roms/random_number_test.ch8");

  std::map<int, std::pair<int, bool>> keyboard;
  keyboard[KEY_ONE] = {0x1, false};
  keyboard[KEY_Q] = {0x4, false};
  keyboard[KEY_A] = {0x7, false};
  keyboard[KEY_Z] = {0xA, false};
  keyboard[KEY_TWO] = {0x2, false};
  keyboard[KEY_W] = {0x5, false};
  keyboard[KEY_S] = {0x8, false};
  keyboard[KEY_X] = {0x0, false};
  keyboard[KEY_THREE] = {0x3, false};
  keyboard[KEY_E] = {0x6, false};
  keyboard[KEY_D] = {0x9, false};
  keyboard[KEY_C] = {0xB, false};
  keyboard[KEY_FOUR] = {0xC, false};
  keyboard[KEY_R] = {0xD, false};
  keyboard[KEY_F] = {0xE, false};
  keyboard[KEY_V] = {0xF, false};

  auto runMode = StepMode::SINGLE;
  bool shouldStep = true;

  while (!WindowShouldClose()) {
    if (shouldStep) {
      cpu.step();
      if (runMode == StepMode::SINGLE) {
        shouldStep = false;
      }
    }

    for (auto &i : keyboard) {
      if (IsKeyDown(i.first) && !i.second.second) {
        cpu.sendInput(i.second.first, true);
        i.second.second = true;
      } else if (i.second.second) {
        cpu.sendInput(i.second.first, false);
        i.second.second = false;
      }
    }
    if (IsKeyPressed(KEY_SPACE)) {
      shouldStep = true;
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (runMode == StepMode::SINGLE) {
        runMode = StepMode::RUN;
        shouldStep = true;
      } else {
        runMode = StepMode::SINGLE;
        shouldStep = false;
      }
    }

    BeginDrawing();
    ClearBackground(DARKGRAY);
    cpu.drawScr(SCREEN_WIDTH, SCREEN_HEIGHT);
    cpu.drawReg(SCREEN_WIDTH, SCREEN_HEIGHT);
    EndDrawing();
  }

  CloseWindow();

  return 0;
}

void load_rom(Memory *mem, uint16_t starting_address, std::string filename) {
  std::ifstream input(filename.c_str(), std::ios::binary);

  // copies all data into buffer
  std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input), {});
  for (const auto &i : buffer) {
    mem->set(starting_address++, i);
  }
}

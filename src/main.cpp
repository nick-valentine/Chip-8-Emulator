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

const char *TITLE = "Chip 8 Emulator";
const int SCREEN_SIZE_MULTIPLIER = 8;
const int SCREEN_HEIGHT = WIN_SIZE_Y * SCREEN_SIZE_MULTIPLIER;
const int SCREEN_WIDTH = WIN_SIZE_X * SCREEN_SIZE_MULTIPLIER;
const int CPU_INFO_HEIGHT = 240;
const int CPU_INFO_WIDTH = 560;

void load_rom(Memory *mem, uint16_t starting_address, std::string filename);

int main() {
  InitWindow(SCREEN_WIDTH + CPU_INFO_WIDTH, SCREEN_HEIGHT + CPU_INFO_HEIGHT,
             TITLE);
  SetTargetFPS(60);

  Memory mem(4096);
  Chip8 cpu(&mem);

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
  bool shouldStep = false;

  int count = 0;
  char **droppedFiles = {0};

  while (!WindowShouldClose()) {

    if (IsFileDropped()) {
      droppedFiles = GetDroppedFiles(&count);
      mem.clear();
      cpu = Chip8(&mem);
      load_rom(&mem, 0x200, droppedFiles[0]);
      ClearDroppedFiles();
      std::string newTitle(TITLE);
      newTitle += std::string(droppedFiles[0]);
      SetWindowTitle(newTitle.c_str());
    }

    if (shouldStep) {
      cpu.step((runMode == StepMode::RUN) ? 1000 : 1);
      if (runMode == StepMode::SINGLE) {
        shouldStep = false;
      }
      cpu.fixedUpdate();
    }

    for (auto &i : keyboard) {
      if (IsKeyDown(i.first)) {
        if (!i.second.second) {
          cpu.sendInput(i.second.first, true);
          i.second.second = true;
        }
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

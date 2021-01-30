#include "chip8.hpp"
#include "memory.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <map>

const int SCREEN_SIZE_MULTIPLIER = 8;
const int SCREEN_HEIGHT = WIN_SIZE_Y * SCREEN_SIZE_MULTIPLIER;
const int SCREEN_WIDTH = WIN_SIZE_X * SCREEN_SIZE_MULTIPLIER;

bool init();
void close();
void sdl_error(const char *wat);

SDL_Window *g_window = nullptr;
SDL_Renderer *g_chip8_renderer = nullptr;

int main() {
  Memory mem(4096);

  Chip8 cpu(&mem);

  // CLS
  mem.set16(0x200, 0x00E0);
  // Load location for sprite 0 into I
  mem.set16(0x202, 0x600F); // ld 0 E
  mem.set16(0x204, 0xF029); // ld I V[0]
  mem.set16(0x206, 0x6003); // ld 0 3
  mem.set16(0x208, 0x6103); // ld 1 3
  // Draw sprite 0
  mem.set16(0x20A, 0xD011); // draw (V[0], V[1]) <- 1 byte
  mem.set16(0x20C, 0x6A02); // ld A 01
  mem.set16(0x20E, 0xFA1E); // Add I V[A]
  mem.set16(0x210, 0x7101); // add V[1] 1
  mem.set16(0x212, 0xD011); // draw (V[0], V[1]) <- 1 byte
  mem.set16(0x214, 0xFA1E); // Add I V[A]
  mem.set16(0x216, 0x7101); // add V[1] 1
  mem.set16(0x218, 0xD011); // draw (V[0], V[1]) <- 1 byte
  mem.set16(0x21A, 0xFA1E); // Add I V[A]
  mem.set16(0x21C, 0x7101); // add V[1] 1
  mem.set16(0x21E, 0xD011); // draw (V[0], V[1]) <- 1 byte
  mem.set16(0x220, 0xFA1E); // Add I V[A]
  mem.set16(0x222, 0x7101); // add V[1] 1
  mem.set16(0x224, 0xD011); // draw (V[0], V[1]) <- 1 byte
  mem.set16(0x226, 0x1226); // Loop in place

  mem.dump(0, 0xA5);
  mem.dump(0x200, 0x225);

  std::map<int, bool> keyboard;
  keyboard[SDLK_1] = 0x1;
  keyboard[SDLK_q] = 0x4;
  keyboard[SDLK_a] = 0x7;
  keyboard[SDLK_z] = 0xA;
  keyboard[SDLK_2] = 0x2;
  keyboard[SDLK_w] = 0x5;
  keyboard[SDLK_s] = 0x8;
  keyboard[SDLK_x] = 0x0;
  keyboard[SDLK_3] = 0x3;
  keyboard[SDLK_e] = 0x6;
  keyboard[SDLK_d] = 0x9;
  keyboard[SDLK_c] = 0xB;
  keyboard[SDLK_4] = 0xC;
  keyboard[SDLK_r] = 0xD;
  keyboard[SDLK_f] = 0xE;
  keyboard[SDLK_v] = 0xF;

  if (!init()) {
    return 1;
  }

  bool quit = false;
  SDL_Event e;
  while (!quit) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_KEYDOWN: {
        auto sym = e.key.keysym.sym;
        if (keyboard.find(sym) != keyboard.end()) {
          cpu.sendInput(keyboard[sym], true);
        }
      } break;
      case SDL_KEYUP: {
        auto sym = e.key.keysym.sym;
        if (keyboard.find(sym) != keyboard.end()) {
          cpu.sendInput(keyboard[sym], false);
        }
      } break;
      case SDL_QUIT:
        quit = true;
        break;
      }
    }
    cpu.step();

    SDL_SetRenderTarget(g_chip8_renderer, NULL);
    SDL_SetRenderDrawColor(g_chip8_renderer, 20, 20, 40, 255);
    SDL_RenderClear(g_chip8_renderer);
    SDL_SetRenderDrawColor(g_chip8_renderer, 100, 255, 100, 255);
    cpu.drawscr(g_chip8_renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_RenderPresent(g_chip8_renderer);
  }

  close();
  return 0;
}

bool init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    sdl_error("SDL could not initialize");
    return false;
  }
  g_window = SDL_CreateWindow("Chip 8 Emulator", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (g_window == nullptr) {
    sdl_error("Window could not be created");
    return false;
  }

  g_chip8_renderer = SDL_CreateRenderer(g_window, -1, 0);
  return true;
}

void close() {
  SDL_DestroyRenderer(g_chip8_renderer);
  g_chip8_renderer = nullptr;
  SDL_DestroyWindow(g_window);
  g_window = nullptr;
  SDL_Quit();
}

void sdl_error(const char *wat) {
  printf("%s", wat);
  printf(": SDL_Error: %s\n", SDL_GetError());
}

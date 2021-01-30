#include "chip8.hpp"
#include "memory.hpp"
#include <iostream>

int main() {
  std::cout << "Hello, World!" << std::endl;
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

  mem.dump(0, 0xA5);
  mem.dump(0x200, 0x225);

  for (int i = 0; i < 20; i++) {
    cpu.step();
    cpu.printreg();
  }
  cpu.printscr();
  std::cout << std::endl << std::endl;

  return 0;
}
#include "memory.hpp"

#include <cstdint>
#include <cstdio>

Memory::Memory(size_t size) : memory(size) {}

void Memory::dump() { dump(0, memory.size()); }

void Memory::dump(size_t low, size_t high) {
  for (size_t i = low; i < memory.size() && i < high; i++) {
    printf("mem[%#04x]=%#04x\n", (uint8_t)i, memory[i]);
  }
}
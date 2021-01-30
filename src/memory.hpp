#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <vector>

class Memory {
public:
  Memory() = delete;
  explicit Memory(size_t size);

  inline uint8_t get(size_t idx) { return memory[idx]; };
  inline void set(size_t idx, uint8_t val) { memory[idx] = val; };
  inline void set16(size_t idx, uint16_t val) {
    memory[idx] = val & 0xFF;
    memory[idx + 1] = val >> 8;
  }

  void dump();
  void dump(size_t low, size_t high);

private:
  std::vector<uint8_t> memory;
};

#endif // MEMORY_HPP
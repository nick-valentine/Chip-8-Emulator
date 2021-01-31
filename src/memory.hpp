#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <cstdint>
#include <vector>

class Memory {
public:
  Memory() = delete;
  explicit Memory(size_t size);

  inline uint8_t get(size_t idx) { return memory[idx % memory.size()]; };
  inline void set(size_t idx, uint8_t val) {
    memory[idx % memory.size()] = val;
  };
  inline void set16(size_t idx, uint16_t val) {
    memory[idx] = val & 0xFF;
    memory[idx + 1] = val >> 8;
  }
  inline void clear() {
    for (int i = 0; i < memory.size(); i++) {
      memory[i] = 0;
    }
  }

  void dump();
  void dump(size_t low, size_t high);

private:
  std::vector<uint8_t> memory;
};

#endif // MEMORY_HPP
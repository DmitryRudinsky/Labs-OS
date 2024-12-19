#include <iostream>
#include <cstdlib>
#include <cstddef>
#include <vector>

const size_t NUM_SIZES = 10;
const size_t BLOCK_SIZES[NUM_SIZES] = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};

class Allocator2 {
private:
    void* memory;
    size_t size;
    std::vector<void*> free_lists[NUM_SIZES];

public:
    Allocator2(void* mem, size_t sz) : memory(mem), size(sz) {
        for (size_t i = 0; i < NUM_SIZES; ++i) {
            free_lists[i].reserve(sz / BLOCK_SIZES[i]);
        }
    }

    ~Allocator2() {
    }

    void* alloc(size_t size) {
        for (size_t i = 0; i < NUM_SIZES; ++i) {
            if (size <= BLOCK_SIZES[i]) {
                if (!free_lists[i].empty()) {
                    void* ptr = free_lists[i].back();
                    free_lists[i].pop_back();
                    return ptr;
                } else {
                    if ((char*)memory + BLOCK_SIZES[i] <= (char*)memory + size) {
                        void* ptr = memory;
                        memory = (char*)memory + BLOCK_SIZES[i];
                        size -= BLOCK_SIZES[i];
                        return ptr;
                    }
                }
            }
        }
        return nullptr;
    }

    void free(void* ptr) {
        if (!ptr) return;

        for (size_t i = 0; i < NUM_SIZES; ++i) {
            if (ptr >= memory && (char*)ptr < (char*)memory + size) {
                free_lists[i].push_back(ptr);
                return;
            }
        }
    }
};

extern "C" {
    Allocator2* allocator2_create(void* memory, size_t size) {
        return new (memory) Allocator2(memory, size);
    }

    void allocator2_destroy(Allocator2* allocator) {
        allocator->~Allocator2();
    }

    void* allocator2_alloc(Allocator2* allocator, size_t size) {
        return allocator->alloc(size);
    }

    void allocator2_free(Allocator2* allocator, void* ptr) {
        allocator->free(ptr);
    }
}
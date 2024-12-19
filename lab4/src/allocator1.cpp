#include <iostream>
#include <cstdlib>
#include <cstddef>

struct Block {
    size_t size;
    bool is_free;
    Block* next;
};

class Allocator {
private:
    void* memory;
    size_t size;
    Block* free_list;

public:
    Allocator(void* mem, size_t sz) {
        memory = (void*)((char*)mem + sizeof(Allocator));
        size = sz - sizeof(Allocator);
        free_list = (Block*)memory;
        free_list->size = size;
        free_list->is_free = true;
        free_list->next = nullptr;
    }

    ~Allocator() {
    }

    void* alloc(size_t size) {
        Block* current = free_list;
        Block* prev = nullptr;

        while (current) {
            if (current->is_free && current->size >= size) {
                if (current->size >= size + sizeof(Block)) {
                    Block* new_block = (Block*)((char*)current + sizeof(Block) + size);
                    new_block->size = current->size - size - sizeof(Block);
                    new_block->is_free = true;
                    new_block->next = current->next;

                    current->size = size;
                    current->is_free = false;
                    current->next = new_block;
                } else {
                    current->is_free = false;
                }
                return (void*)((char*)current + sizeof(Block));
            }
            prev = current;
            current = current->next;
        }
        return nullptr;
    }

    void free(void* ptr) {
        if (!ptr) return;

        Block* block = (Block*)((char*)ptr - sizeof(Block));
        block->is_free = true;

        Block* current = free_list;
        Block* prev = nullptr;
        while (current && current != block) {
            prev = current;
            current = current->next;
        }

        if (prev && prev->is_free) {
            prev->size += sizeof(Block) + block->size;
            prev->next = block->next;
            block = prev;
        }

        if (block->next && block->next->is_free) {
            block->size += sizeof(Block) + block->next->size;
            block->next = block->next->next;
        }
    }
};

extern "C" {
    Allocator* allocator_create(void* memory, size_t size) {
        return new (memory) Allocator(memory, size);
    }

    void allocator_destroy(Allocator* allocator) {
        allocator->~Allocator();
    }

    void* allocator_alloc(Allocator* allocator, size_t size) {
        return allocator->alloc(size);
    }

    void allocator_free(Allocator* allocator, void* ptr) {
        allocator->free(ptr);
    }
}
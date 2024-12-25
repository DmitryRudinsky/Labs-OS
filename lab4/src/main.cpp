#include <iostream>
#include <dlfcn.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <chrono>

typedef void* (*CreateFunc)(void*, size_t);
typedef void (*DestroyFunc)(void*);
typedef void* (*AllocFunc)(void*, size_t);
typedef void (*FreeFunc)(void*, void*);

void* sys_alloc(size_t size) {
    return malloc(size);
}

void sys_free(void* ptr) {
    free(ptr);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <path_to_allocator_library>" << std::endl;
        return 1;
    }

    void* handle = dlopen(argv[1], RTLD_LAZY);
    if (!handle) {
        std::cerr << "Error loading library: " << dlerror() << std::endl;
        return 1;
    }

    CreateFunc create = (CreateFunc)dlsym(handle, "allocator_create");
    DestroyFunc destroy = (DestroyFunc)dlsym(handle, "allocator_destroy");
    AllocFunc alloc = (AllocFunc)dlsym(handle, "allocator_alloc");
    FreeFunc free_func = (FreeFunc)dlsym(handle, "allocator_free");

    if (!create || !destroy || !alloc || !free_func) {
        std::cerr << "Error loading functions: " << dlerror() << std::endl;
        dlclose(handle);
        return 1;
    }

    size_t allocator_size = 1024 * 1024; // 1 MB
    void* allocator_memory = malloc(allocator_size);
    void* allocator = create(allocator_memory, allocator_size);

    // Тест 1: Базовое выделение и освобождение
    void* ptr1 = alloc(allocator, 100);
    void* ptr2 = alloc(allocator, 200);

    if (ptr1 && ptr2) {
        std::cout << "Allocation successful!" << std::endl;
        std::cout << "ptr1: " << ptr1 << " (100 bytes)" << std::endl;
        std::cout << "ptr2: " << ptr2 << " (200 bytes)" << std::endl;
    } else {
        std::cerr << "Allocation failed!" << std::endl;
        if (!ptr1) std::cerr << "Failed to allocate 100 bytes" << std::endl;
        if (!ptr2) std::cerr << "Failed to allocate 200 bytes" << std::endl;
    }

    free_func(allocator, ptr1);
    free_func(allocator, ptr2);

    // Тест 2: Выделение и освобождение множества блоков
    std::vector<void*> pointers;
    for (int i = 0; i < 100; ++i) {
        void* ptr = alloc(allocator, 128);
        if (ptr) {
            pointers.push_back(ptr);
        } else {
            std::cerr << "Failed to allocate block " << i << std::endl;
            break;
        }
    }

    for (void* ptr : pointers) {
        free_func(allocator, ptr);
    }

    // Тест 3: Проверка на утечки памяти
    size_t allocated_memory = 0;
    size_t freed_memory = 0;

    void* ptr3 = alloc(allocator, 300);
    if (ptr3) allocated_memory += 300;

    void* ptr4 = alloc(allocator, 400);
    if (ptr4) allocated_memory += 400;

    free_func(allocator, ptr3);
    freed_memory += 300;

    free_func(allocator, ptr4);
    freed_memory += 400;

    if (allocated_memory == freed_memory) {
        std::cout << "No memory leaks detected!" << std::endl;
    } else {
        std::cerr << "Memory leak detected!" << std::endl;
    }

    destroy(allocator);
    free(allocator_memory);
    dlclose(handle);

    return 0;
}
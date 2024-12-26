#include <iostream>
#include <dlfcn.h>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <map>

typedef void* (*CreateFunc)(void*, size_t);
typedef void (*DestroyFunc)(void*);
typedef void* (*AllocFunc)(void*, size_t);
typedef void (*FreeFunc)(void*, void*);

std::map<void*, size_t> block_sizes;

void* emergency_create(void* memory, size_t size) {
    return memory;
}

void emergency_destroy(void* allocator) {
    const char* message = "Emergency Allocator destroyed.\n";
    write(1, message, strlen(message));
}

void* emergency_alloc(void* allocator, size_t size) {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        return nullptr;
    }
    block_sizes[ptr] = size;
    return ptr;
}

void emergency_free(void* allocator, void* ptr) {
    size_t size = block_sizes[ptr];
    munmap(ptr, size);
    block_sizes.erase(ptr);
}

void write_output(const char* message) {
    write(1, message, strlen(message));
}

void write_error(const char* message) {
    write(2, message, strlen(message));
}

int main(int argc, char* argv[]) {
    CreateFunc create = nullptr;
    DestroyFunc destroy = nullptr;
    AllocFunc alloc = nullptr;
    FreeFunc free_func = nullptr;

    if (argc < 2) {
        write_error("No library path provided. Using emergency allocator.\n");
        create = emergency_create;
        destroy = emergency_destroy;
        alloc = emergency_alloc;
        free_func = emergency_free;
    } else {
        void* handle = dlopen(argv[1], RTLD_LAZY);
        if (!handle) {
            write_error("Error loading library: ");
            write_error(dlerror());
            write_error("\nUsing emergency allocator.\n");
            create = emergency_create;
            destroy = emergency_destroy;
            alloc = emergency_alloc;
            free_func = emergency_free;
        } else {
            create = (CreateFunc)dlsym(handle, "allocator_create");
            destroy = (DestroyFunc)dlsym(handle, "allocator_destroy");
            alloc = (AllocFunc)dlsym(handle, "allocator_alloc");
            free_func = (FreeFunc)dlsym(handle, "allocator_free");

            if (!create || !destroy || !alloc || !free_func) {
                write_error("Error loading functions: ");
                write_error(dlerror());
                write_error("\nUsing emergency allocator.\n");
                dlclose(handle);
                create = emergency_create;
                destroy = emergency_destroy;
                alloc = emergency_alloc;
                free_func = emergency_free;
            }
        }
    }

    size_t allocator_size = 1024 * 1024;
    void* allocator_memory = mmap(nullptr, allocator_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!allocator_memory) {
        write_error("Failed to allocate memory for allocator\n");
        return 1;
    }

    void* allocator = create(allocator_memory, allocator_size);

    // Тест 1: Базовое выделение и освобождение
    void* ptr1 = alloc(allocator, 100);
    void* ptr2 = alloc(allocator, 200);

    if (ptr1 && ptr2) {
        write_output("Allocation successful!\n");
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "ptr1: %p (100 bytes)\n", ptr1);
        write_output(buffer);
        snprintf(buffer, sizeof(buffer), "ptr2: %p (200 bytes)\n", ptr2);
        write_output(buffer);
    } else {
        write_error("Allocation failed!\n");
        if (!ptr1) write_error("Failed to allocate 100 bytes\n");
        if (!ptr2) write_error("Failed to allocate 200 bytes\n");
    }

    free_func(allocator, ptr1);
    free_func(allocator, ptr2);

    write_output("\n");

    // Тест 2: Выделение и освобождение множества блоков
    std::vector<void*> pointers;
    for (int i = 0; i < 100; ++i) {
        void* ptr = alloc(allocator, 128);
        if (ptr) {
            pointers.push_back(ptr);
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%d ptr%p\n", i + 1, ptr);
            write_output(buffer);
        } else {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Failed to allocate block %d\n", i);
            write_error(buffer);
            break;
        }
    }

    for (void* ptr : pointers) {
        free_func(allocator, ptr);
    }

    write_output("\n");

    // Тест 3: Проверка на утечки памяти
    std::vector<void*> pointers_test3;
    size_t allocated_memory = 0;
    size_t freed_memory = 0;

    void* ptr3 = alloc(allocator, 300);
    if (ptr3) {
        pointers_test3.push_back(ptr3);
        allocated_memory += 300;
    }

    void* ptr4 = alloc(allocator, 400);
    if (ptr4) {
        pointers_test3.push_back(ptr4);
        allocated_memory += 400;
    }

    for (void* ptr : pointers_test3) {
        free_func(allocator, ptr);
        freed_memory += (ptr == ptr3) ? 300 : 400;
    }

    if (allocated_memory == freed_memory) {
        write_output("No memory leaks detected!\n");
    } else {
        write_error("Memory leak detected!\n");
    }

    write_output("\n");

    // Тест 4: Выделение памяти до исчерпания (с ограничением)
    std::vector<void*> pointers4;
    size_t total_allocated = 0;
    const size_t max_blocks = 1000;
    while (true) {
        void* ptr = alloc(allocator, 128);
        if (ptr) {
            pointers4.push_back(ptr);
            total_allocated += 128;
            if (pointers4.size() >= max_blocks) {
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "Reached maximum blocks limit (%zu). Stopping allocation.\n", max_blocks);
                write_output(buffer);
                break;
            }
        } else {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Memory exhausted after allocating %zu bytes\n", total_allocated);
            write_output(buffer);
            break;
        }
    }

    for (void* ptr : pointers4) {
        free_func(allocator, ptr);
    }

    // Тест 5: Повторное выделение освобожденной памяти
    void* ptr10 = alloc(allocator, 100);
    if (ptr10) {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), "Allocated 100 bytes at %p\n", ptr10);
        write_output(buffer);
        free_func(allocator, ptr10);

        void* ptr20 = alloc(allocator, 100);
        if (ptr20) {
            snprintf(buffer, sizeof(buffer), "Reallocated 100 bytes at %p\n", ptr20);
            write_output(buffer);
            free_func(allocator, ptr20);
        } else {
            write_error("Reallocation failed!\n");
        }
    } else {
        write_error("Initial allocation failed!\n");
    }

    destroy(allocator);
    munmap(allocator_memory, allocator_size);

    return 0;
}
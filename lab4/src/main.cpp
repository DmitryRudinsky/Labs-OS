#include <iostream>
#include <dlfcn.h>
#include <cstdlib>
#include <ctime>

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

    void* ptr1 = alloc(allocator, 100);
    void* ptr2 = alloc(allocator, 200);

    if (ptr1 && ptr2) {
        std::cout << "Allocation successful!" << std::endl;
    } else {
        std::cerr << "Allocation failed!" << std::endl;
    }

    free_func(allocator, ptr1);
    free_func(allocator, ptr2);

    destroy(allocator);
    free(allocator_memory);
    dlclose(handle);

    return 0;
}
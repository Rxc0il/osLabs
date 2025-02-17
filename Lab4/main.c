#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h> //dlopen, dlsym, dlclose


#ifndef MAP_ANON
  #ifdef MAP_ANONYMOUS
    #define MAP_ANON MAP_ANONYMOUS
  #else
    #define MAP_ANON 0x20
  #endif
#endif


typedef struct memory_manager {
    void *(*create)(void *mem_area, size_t total_size);
    void *(*allocate)(void *manager, size_t size);
    void (*release)(void *manager, void* chunk);
    void (*destroy)(void *manager);
} memory_manager;


void* stub_create_manager(void* memory_area, size_t total_size) {
    (void)memory_area;
    (void)total_size;
    return memory_area; 
}


void* stub_allocate_memory(void* manager, size_t size) {
    manager = manager;
    uint32_t* memory = mmap(NULL, size + sizeof(uint32_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (memory == MAP_FAILED) {
        return NULL;
    }
    *memory = (uint32_t)(size + sizeof(uint32_t));
    return memory + 1;
}

void stub_release_memory(void* manager, void* chunk){
        (void)manager;
        (void)chunk;
}

void stub_destroy_manager(void* manager){
    (void)manager;
}

memory_manager* load_memory_manager(const char* library_path) {
    if (!library_path || !library_path[0]) {
        const char msg[] = "ERROR: Do`s not set library path\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        memory_manager *manager = malloc(sizeof(memory_manager));
        if (!manager) {
            const char msg[] = "ERROR: failed to allocate memory manager\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return NULL;
        }
        manager->create = stub_create_manager;
        manager->allocate = stub_allocate_memory;
        manager->release = stub_release_memory;
        manager->destroy = stub_destroy_manager;
        return manager;
    }

    void* library = dlopen(library_path, RTLD_LOCAL | RTLD_NOW);
    if (!library) {
        const char msg[] = "ERROR: I can`t open library\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        memory_manager *manager = malloc(sizeof(memory_manager));
        if (!manager) {
            const char msg[] = "ERROR: failed to allocate memory manager\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            return NULL;
        }
        manager->create = stub_create_manager;
        manager->allocate = stub_allocate_memory;
        manager->release = stub_release_memory;
        manager->destroy = stub_destroy_manager;
        return manager;
    }

    const char msg[] = "SUCCESS: I load the library\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    
    memory_manager *manager = malloc(sizeof(memory_manager));
    if (!manager) {
        const char msg[] = "ERROR: failed to allocate memory manager\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }
    manager->create = dlsym(library, "create_memory_manager");
    manager->allocate = dlsym(library, "malloc_my_realize");
    manager->release = dlsym(library, "release_memory");
    manager->destroy = dlsym(library, "destroy_memory_manager");


    if (!manager->create || !manager->allocate || !manager->release || !manager->destroy) {
        const char msg[] = "ERROR: failed to load symbols\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        free(manager);
        dlclose(library);
        return NULL;
    }

    return manager;
}

int run_memory_test(const char* library_path) {
    memory_manager* manager_api = load_memory_manager(library_path);

    if (!manager_api) {
        return EXIT_FAILURE;
    }
    size_t test_area_size = 4096;
    void *test_area_address = mmap(NULL, test_area_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (test_area_address == MAP_FAILED) {
        const char msg[] = "ERROR: failed to create test area\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        free(manager_api);
        return EXIT_FAILURE;
    }
    
    void *manager = manager_api->create(test_area_address, test_area_size);
    if (!manager) {
        const char msg[] = "ERROR: failed to create memory manager\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        munmap(test_area_address, test_area_size);
        free(manager_api);
        return EXIT_FAILURE;
    }

    char msg[] = "SUCCESS: I created memory manager :)\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    

    void *allocated_chunk = manager_api->allocate(manager, 64);
    if (allocated_chunk == NULL) {
        const char msg[] = "ERROR: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
    } else {
        const char msg[] = "SUCCESS: memory allocated\n";
        write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    }
    

    strcpy(allocated_chunk, "VERY VERY LONG WORD!\n");
    
        
    write(STDOUT_FILENO, allocated_chunk, strlen(allocated_chunk));
    char address_buffer[64];
    snprintf(address_buffer, sizeof(address_buffer), "SUCCESS: Allocated memory address: %p\n", allocated_chunk);
    write(STDOUT_FILENO, address_buffer, strlen(address_buffer));
    
    manager_api->release(manager, allocated_chunk);
    char free_message[] = "SUCCESS: I released memory\n";
    write(STDOUT_FILENO, free_message, sizeof(free_message) - 1);

    
    manager_api->destroy(manager);


    const char msg2[] = "SUCCESS: I destroyed memory manager :)\n";
    write(STDOUT_FILENO, msg2, sizeof(msg2) - 1);
    
    free(manager_api);
    int err = munmap(test_area_address, test_area_size); //destroy test area
    if (err != 0) {
        const char msg[] = "ERROR: failed to destroy test area\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


int main(int argc, char** argv) {
    const char* library_path = NULL;
    if (argc > 1) {
        library_path = argv[1];
    }

    if (run_memory_test(library_path) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
    }
       return EXIT_SUCCESS;
}
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#define COUNT 100
#define MAX 100
#define MIN 0

typedef struct mem_chunk {
    struct mem_chunk* next;
    struct mem_chunk* prev;
    int capacity;
} mem_chunk;

typedef struct {
    mem_chunk* lists_chunks[COUNT];
    void* memory;
    size_t size;
} allocator;

long long calculate_power(int base, int exponent) {
    long long result = 1;
    if (exponent < 0) {
         return -1;
    }
    for (int i = 0; i < exponent; ++i) {
        result *= base;
    }
    return result;
}

allocator* create_memory_manager(void* mem, size_t total_size) {
    allocator* manager = (allocator*)mem;
    manager->size = total_size - sizeof(allocator);
    manager->memory = (char*)mem + sizeof(allocator);
    
    size_t current_offset = 0;
    int list_index = 0;
    
    
    for (int i = 0; i < COUNT; ++i) {
        manager->lists_chunks[i] = NULL;
    }
    
    
    while (current_offset < manager->size) {
      int chunk_size = calculate_power(2, list_index/5);
        if (current_offset + chunk_size > manager->size) {
            break; 
        }
        
        mem_chunk* new_chunk = (mem_chunk*)((char*)manager->memory + current_offset);
        new_chunk->capacity = chunk_size;

        
        if (manager->lists_chunks[list_index] == NULL) {
          new_chunk->next = NULL;
           new_chunk->prev = NULL;
        }
         else{
            new_chunk->next = manager->lists_chunks[list_index];
             manager->lists_chunks[list_index]->prev = new_chunk;
        }

        manager->lists_chunks[list_index] = new_chunk;
         
        
        current_offset += chunk_size;
        list_index++;
    }
    
    return manager;
}

void split_memory_chunk(allocator* manager, mem_chunk* chunk) {
    int list_index = 0;
    int new_capacity = chunk->capacity / 2;
    
    while (calculate_power(2, list_index) < new_capacity) {
        list_index++;
    }

    
    mem_chunk* new_chunk = (mem_chunk*)((char*)manager->memory + new_capacity);
    new_chunk->capacity = new_capacity;
     
    chunk->capacity = new_capacity;
    
    if(manager->lists_chunks[list_index] == NULL){
        new_chunk->next = NULL;
           new_chunk->prev = NULL;
    } else{
        new_chunk->next = manager->lists_chunks[list_index];
        manager->lists_chunks[list_index]->prev = new_chunk;
    }
    manager->lists_chunks[list_index] = new_chunk;
}

void* malloc_my_realize(allocator* manager, size_t requested_size) {
   int list_index = 0;
    
    while (calculate_power(2, list_index) < requested_size) {
        list_index++;
    }
    
    if (list_index >= COUNT) {
        const char msg[] = "ERROR: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    if (manager->lists_chunks[list_index] != NULL) {
        mem_chunk* found_chunk = manager->lists_chunks[list_index];
         manager->lists_chunks[list_index] = found_chunk->next;
        
        return found_chunk; 
    }

    for (int i = list_index + 1; i < COUNT; ++i) {
        if (manager->lists_chunks[i] != NULL) {
           mem_chunk* current_chunk = manager->lists_chunks[i];

            while (i > list_index)
            {
                manager->lists_chunks[i] = current_chunk->next;
               split_memory_chunk(manager, current_chunk);
               i--;
                current_chunk = manager->lists_chunks[i];
            }
            
            if(manager->lists_chunks[list_index] != NULL){
              mem_chunk *block = manager->lists_chunks[list_index];
                manager->lists_chunks[list_index] = block->next;
                return block;
            }
        }
    }
    return NULL;    
}

void release_memory(allocator* manager, void* chunk_ptr) {
    if (!manager || !chunk_ptr) {
        return;
    }
    
    mem_chunk* chunk = (mem_chunk*)chunk_ptr;
     int list_index = 0;
    
    while (calculate_power(2, list_index) < chunk->capacity) {
        list_index++;
    }
    if(list_index >= COUNT){
        const char msg[] = "ERROR: failed to release memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }

    if(manager->lists_chunks[list_index] != NULL){
        chunk->next = manager->lists_chunks[list_index];
        manager->lists_chunks[list_index]->prev = chunk;
    } else {
        chunk->next = NULL;
    }
    manager->lists_chunks[list_index] = chunk;
}

void destroy_memory_manager(allocator* manager) {
     if (!manager) {
        return;
    }
    if (munmap((void *)manager, manager->size + sizeof(allocator)) != 0) { 
        const char msg[] = "ERROR: failed to destroy allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
}

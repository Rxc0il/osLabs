#include <unistd.h> //write
#include <stdlib.h>
#include <sys/mman.h>


//для блока памяти
typedef struct mem_chunk {
    int is_available;
    int capacity;
    struct mem_chunk *left_brother;
    struct mem_chunk *right_brother;
} mem_chunk;

//для аллокатора
typedef struct allocator {
    void *area_memory;
    mem_chunk *top;
    int current_offset;
    int current_size;
} allocator;

//тут создаем новый блок памяти
mem_chunk* allocate_memory(allocator *alloc, int capacity) {
    
    if ((sizeof(mem_chunk) + alloc->current_offset) > alloc->current_size) {
        const char msg[] = "ERROR BUDDY: not enough memory for new chunk\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    mem_chunk *new_chunk = (mem_chunk *)((char *)alloc->area_memory + alloc->current_offset);
    alloc->current_offset += sizeof(mem_chunk);

    new_chunk->capacity = capacity;
    new_chunk->is_available = 1;
    
    new_chunk->left_brother = NULL;
    new_chunk->right_brother = NULL;

    return new_chunk;
    
}

int is_power_two(unsigned int n) {
    if (n <= 0) {
        return 0;
    }
    int count = 0;
    while (n > 0) {
        if (n % 2 == 1) {
            count++;
        }
        n = n / 2;
    }
    return count == 1;
}

//тут создаем наш аллокатор
allocator* create_memory_manager(void* mem_area, size_t total_size) {

    if (!is_power_two(total_size)) {
        const char msg[] = "ERROR BUDDY: total_size is not a power of 2\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    allocator *new_allocator = (allocator *)mem_area;

    new_allocator->area_memory = (char*)mem_area + sizeof(allocator);
    new_allocator->current_offset = 0;
    new_allocator->current_size = total_size - sizeof(allocator);

    new_allocator->top = allocate_memory(new_allocator, total_size);
    if (!new_allocator->top) {
        const char msg[] = "ERROR BUDDY: failed to create top chunk\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    } 

    return new_allocator;
}

void split(allocator *alloc, mem_chunk *chunk)
{
    int newSize = chunk->capacity / 2;

    chunk->right_brother = allocate_memory(alloc, newSize);
    chunk->left_brother = allocate_memory(alloc, newSize);

   
}

//тут ищем блок нужного размера
mem_chunk *allocate_memory_chunk(allocator *alloc, mem_chunk *current_chunk, int capacity) {

    if (current_chunk == NULL || current_chunk->capacity < capacity || !current_chunk->is_available) {
        const char msg[] = "ERROR BUDDY: failed to allocate memory chunk\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    if (current_chunk->capacity == capacity) {
        current_chunk->is_available = 0;
        return (void *)current_chunk;
    }

    if (current_chunk->left_brother == NULL) {
       split(alloc, current_chunk);
    }

    void* allocated = allocate_memory_chunk(alloc, current_chunk->left_brother, capacity);
    if (allocated == NULL) {
        allocated = allocate_memory_chunk(alloc, current_chunk->right_brother, capacity);
    }

    current_chunk->is_available = (current_chunk->left_brother && current_chunk->left_brother->is_available) || 
                                   (current_chunk->right_brother && current_chunk->right_brother->is_available);
    return allocated;
}

void* malloc_my_realize(allocator *alloc, int capacity) {
    if ((alloc == NULL) || (capacity <= 0)) {
        const char msg[] = "ERROR BUDDY: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }

    while (!is_power_two(capacity)) {
        capacity++;
    }

    void* allocated = allocate_memory_chunk(alloc, alloc->top, capacity);
    if (!allocated) {
        const char msg[] = "ERROR BUDDY: failed to allocate memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return NULL;
    }
    return (char*)allocated + sizeof(mem_chunk);    
}

void release_memory(allocator *alloc, void* chunk_ptr) {
    
    if (alloc == NULL || chunk_ptr == NULL) {
        const char msg[] = "ERROR BUDDY: failed to free memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return; 
    }
    
    mem_chunk* freed_chunk = (mem_chunk*)((char*)chunk_ptr - sizeof(mem_chunk));
    if (freed_chunk == NULL) {
        const char msg[] = "ERROR BUDDY: failed to free memory\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return; 
    }
    
    freed_chunk->is_available = 1;

    if (freed_chunk->left_brother != NULL && freed_chunk->left_brother->is_available 
        && freed_chunk->right_brother->is_available) {
        release_memory(alloc, freed_chunk->left_brother);
        release_memory(alloc, freed_chunk->right_brother);
        freed_chunk->left_brother = freed_chunk->right_brother = NULL;
    }
    
}

void destroy_memory_manager(allocator *alloc) {
    if (!alloc) {
        const char msg[] = "ERROR: failed to destroy allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }
    
    release_memory(alloc, alloc->top + sizeof(mem_chunk));


    if (munmap((void *)alloc, alloc->current_size + sizeof(allocator)) == 1) {
        const char msg[] = "ERROR: failed to destroy allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        return;
    }
}
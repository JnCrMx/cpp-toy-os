#include <kernel/memory.hpp>

#include <cstdlib>
#include <cstddef>
#include <type_traits>

#include <config.hpp>
#include <kernel/basic.hpp>
#include <kernel/debug.hpp>
#include <lib/string.hpp>

namespace kernel {

static memory_statistics stats{};
const memory_statistics& malloc_stats() {
    return stats;
}

struct mem_block {
    mem_block* next;
    mem_block* prev;
    size_t size;
};
static constexpr size_t MEMORY_SIZE = config::malloc_memory_size;
static char MEMORY[MEMORY_SIZE];
static mem_block* last_allocation = nullptr;

constexpr size_t alignment = std::alignment_of_v<max_align_t>;
constexpr size_t used_mask = 0b1UL;
constexpr size_t size_mask = ~static_cast<size_t>(0b111UL);

void malloc_init()
{
    mem_block* block = std::launder(reinterpret_cast<mem_block*>(MEMORY));
    block->next = nullptr;
    block->prev = nullptr;

    block->size = MEMORY_SIZE - sizeof(mem_block);
    last_allocation = block;

    stats.memory_total = MEMORY_SIZE;
    stats.memory_allocated = 0;
    stats.num_blocks = 1;
    stats.num_allocations = 0;
    stats.block_overhead = sizeof(mem_block);
}

void* calloc(size_t num, size_t size)
{
    size_t total;
    if(__builtin_mul_overflow(num, size, &total)) {
        return nullptr;
    }
    return malloc(total);
}

void* malloc(size_t size)
{
    if(size == 0) {
        kernel::debug::ktrace("malloc({}) -> nullptr", size);
        return nullptr;
    }
    size_t sizeAligned = size;
    if(sizeAligned % alignment != 0) {
        sizeAligned += alignment - sizeAligned % alignment;
    }

    mem_block* pick = nullptr;
    for(mem_block* current = last_allocation; current; current = current->next) {
        if(current->size & used_mask)
            continue;
        if((current->size & size_mask) >= sizeAligned) {
            pick = current;
            break;
        }
    }
    if(!pick) {
        for(mem_block* current = reinterpret_cast<mem_block*>(MEMORY); current != last_allocation; current = current->next) {
            if(current->size & used_mask)
                continue;
            if((current->size & size_mask) >= sizeAligned) {
                pick = current;
                break;
            }
        }
    }
    if(!pick) {
        kernel::debug::kwarn("malloc({}) -> nullptr (OUT OF MEMORY)", size);
        return nullptr;
    }

    void* ptr = reinterpret_cast<char*>(pick) + sizeof(mem_block);
    size_t finalSize = sizeAligned;
    if((pick->size) == sizeAligned) {
        pick->size |= used_mask;
    }
    else if((pick->size & size_mask) < sizeAligned + sizeof(mem_block) + alignment) {
        pick->size |= used_mask;
        finalSize = (pick->size & size_mask);
    }
    else {
        size_t remaining = (pick->size & size_mask) - sizeof(mem_block) - sizeAligned;

        mem_block* new_block = reinterpret_cast<mem_block*>(static_cast<char*>(ptr) + sizeAligned);
        new_block->next = pick->next;
        new_block->prev = pick;
        new_block->size = remaining;
        if(pick->next)
        {
            pick->next->prev = new_block;
        }
        pick->next = new_block;

        pick->size = sizeAligned | used_mask;

        stats.num_blocks+=1;
    }
    last_allocation = pick;

    kernel::memset(ptr, 0, finalSize);
    stats.num_allocations++;
    stats.memory_allocated += finalSize;

    kernel::debug::ktrace("malloc({}) -> {}", size, ptr);
    return ptr;
}

void free(void* ptr)
{
    kernel::debug::ktrace("free({})", ptr);
    if(!ptr) {
        return;
    }

    mem_block* block = reinterpret_cast<mem_block*>(static_cast<char*>(ptr) - sizeof(mem_block));
    if(!(block->size & used_mask)) {
        panic("Double free");
    }

    stats.memory_allocated -= block->size & ~used_mask;
    stats.num_allocations--;
    if(block->prev && block->next && !(block->prev->size & used_mask) && !(block->next->size & used_mask)) // prev und next frei
    {
        block->prev->size += (block->size & size_mask) + (block->next->size & size_mask) + 2 * sizeof(mem_block);
        block->prev->next = block->next->next;
        if(block->next->next)
        {
            block->next->next->prev = block->prev;
        }
        if(last_allocation == block || last_allocation == block->next)
        {
            last_allocation = block->prev;
        }
        stats.num_blocks -= 2;
    }
    else if(block->prev && !(block->prev->size & used_mask)) // prev frei
    {
        block->prev->size += (block->size & size_mask) + sizeof(mem_block);
        block->prev->next = block->next;
        if(block->next)
        {
            block->next->prev = block->prev;
        }
        if(last_allocation == block)
        {
            last_allocation = block->prev;
        }
        stats.num_blocks -= 1;
    }
    else if(block->next && !(block->next->size & used_mask)) // next frei
    {
        block->size += (block->next->size & size_mask) + sizeof(mem_block);
        block->size &= ~used_mask;
        if(block->next->next)
        {
            block->next->next->prev = block;
        }
        if(last_allocation == block->next)
        {
            last_allocation = block;
        }
        block->next = block->next->next;
        stats.num_blocks -= 1;
    }
    else // nur block frei
    {
        block->size &= ~used_mask;
    }
}

}

void* operator new(std::size_t size)
{
    void* ptr = kernel::malloc(size);
    kernel::debug::ktrace("operator new({}) -> {}", size, ptr);
    if(ptr == nullptr) {
        kernel::panic("memory allocation failed");
    }
    return ptr;
}

void* operator new[](std::size_t size)
{
    void* ptr = kernel::malloc(size);
    kernel::debug::ktrace("operator new[]({}) -> {}", size, ptr);
    if(ptr == nullptr) {
        kernel::panic("memory allocation failed");
    }
    return ptr;
}

void operator delete(void* ptr) noexcept
{
    kernel::free(ptr);
    kernel::debug::ktrace("operator delete({})", ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept
{
    kernel::free(ptr);
    kernel::debug::ktrace("operator delete({}, {})", ptr, size);
}

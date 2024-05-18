#include "deque.h"
#include "list.h"
#include "set.h"
#include <stdio.h>
#include <string.h>

typedef enum {
    FIRST_FIT = 0,
    BEST_FIT = 1,
    WORST_FIT = 2,
} StrategyType;

static const char *StrategyTypeStr[] = {
    "FIRST_FIT",
    "BEST_FIT",
    "WORST_FIT"
};

typedef struct {
    int blockId;
    unsigned int low_address;
    unsigned int high_address;
} block_interval_t;
 
typedef struct {
    list_t* blocks;
    unsigned int size;
} memory_t;

typedef enum {
    ALLOCATION,
    DEALLOCATION,
    COMPACT
} InstructionType;

typedef struct {
    InstructionType type;
    int blockId;
    int dimension;
} instruction_t;

memory_t* memory_init(unsigned int size) {
    memory_t* mem = malloc(sizeof(memory_t));
    mem->blocks = list_new();
    mem->size = size;
    return mem;
}

bool containsBlock(memory_t* memory, int blockId) {
    if (blockId < 0 || blockId > memory->size) {
        return false;
    }

    // loop start (memory->list->head), continues until node is not null, (node->next) proceeds with each iteration.
    int currentBlockId = 1;
    for(list_node_t* node = memory->blocks->head; node != NULL; node-> next) {
        if (node->val != NULL) {
            block_interval_t* block = (block_interval_t*)node->val; // common practise, to cast generic point.
            if (currentBlockId == blockId) {
                return true;
            }
            currentBlockId++;
        }
    }
    return false;
}

int blockDimension(memory_t* memory, int blockId) {
    list_node_t* ls = list_at(memory->blocks, blockId);
    block_interval_t* block_interval = (block_interval_t*)ls->val;

    if (block_interval != NULL) {
        return block_interval->high_address - block_interval->low_address;; 
    }
    return 0;
}

list_t* blocks(memory_t* memory) {
    list_t* allocated_blocks = list_new();
    unsigned int block_id = 0;

    for(list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
        if (node->val != NULL) //IDK MAYBE...
        {
            block_interval_t* interval = (block_interval_t*)node->val;
            list_node_t* block_node = list_node_new(&block_id);
            list_rpush(allocated_blocks, block_node);
        }
    }
    return allocated_blocks;
}

block_interval_t* getBlockInterval(memory_t* memory, int blockId) {
    list_node_t* element = list_at(memory->blocks, blockId);
    if (element == NULL) {
        return NULL;
    }
    block_interval_t* block = (block_interval_t*)element->val;
    return block;
}

simple_set* neighboringBlocks(memory_t* memory, int blockId) {
    simple_set* neighbors_set;
    set_init(neighbors_set);
    
    for(list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
        if(node->val != NULL) {
            block_interval_t* block = (block_interval_t*)node->val;
            char* st = malloc(sizeof(block->low_address) + sizeof(block->high_address) + sizeof('\n'));
            memcpy(st,&block->low_address, sizeof(block->low_address));
            memcpy(st + sizeof(block->low_address),&block->high_address, sizeof(block->high_address));
            st[sizeof(block->low_address)+ sizeof(block->high_address)] = '\n';
            set_add(neighbors_set, (const char * )st); //ere
        }
    }
    return neighbors_set;
}

double fragmentation(memory_t* memory) {
    double larget_block_free_memory  = 0;
    double current_block_free_memory = 0;
    double total_free_memory = 0;
    bool continues = false;

    for(list_node_t* node = memory->blocks->head; node !=NULL; node =node->next){
        block_interval_t* val = (block_interval_t*)node->val;

        if (val == NULL) // IS free memory
        {
            total_free_memory += val->low_address-val->high_address;
            if (continues) {
                current_block_free_memory += val->low_address-val->high_address;
                if (current_block_free_memory > larget_block_free_memory) {
                    larget_block_free_memory = current_block_free_memory;
                }
            }
            else {
                continues = true;
                current_block_free_memory = val->low_address-val->high_address;
            }
        }
        current_block_free_memory=0;
        continues = false;
    };

    double frag = 1-(larget_block_free_memory/total_free_memory);
    return frag;
}

simple_set* freeSlots(memory_t* memory, int blockId) {

    simple_set* free_sets;  
    if(memory->blocks->len == memory->size) {
        return free_sets; // if full
    }
    set_init(free_sets);


    for(list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
        block_interval_t* block = (block_interval_t*)node->val;
        if(block == NULL) {
            // char* st 
            char* st = malloc(sizeof(block->low_address) + sizeof(block->high_address) + sizeof('\n'));
            memcpy(st,&block->low_address, sizeof(block->low_address));
            memcpy(st + sizeof(block->low_address),&block->high_address, sizeof(block->high_address));
            st[sizeof(block->low_address)+ sizeof(block->high_address)] = '\n';
            set_add(free_sets, (const char * )st); //ere
        }
    }
    return free_sets;
}   


void printBlocks(memory_t* memory) {
    printf("Allocated Blocks:\n");

    for (list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
        block_interval_t* bi = (block_interval_t*)node->val;

        printf("(%u-%u) --> ID %d\n", bi->low_address, bi->high_address, bi->blockId);
    }
}

void printFreeSlots(memory_t* memory) {
    printf("Free Slots:\n");

    unsigned int currentAddress = 0;

    for (list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
        block_interval_t* bi = (block_interval_t*)node->val;

        if (bi->low_address > currentAddress) {
            printf("(%u-%u) --> EMPTY\n", currentAddress, bi->low_address - 1);
        }

        currentAddress = bi->high_address + 1;
    }

    if (currentAddress < memory->size) {
        printf("(%u-%u) --> EMPTY\n", currentAddress, memory->size - 1);
    }
}


typedef struct {
    StrategyType strategy;
    Deque* queue;
    memory_t* memory;
} simulation_instance_t;

simulation_instance_t* strategy_type_init(StrategyType strategy, Deque* queue, memory_t* memory) {
    simulation_instance_t* sim = malloc(sizeof(simulation_instance_t));
    sim->memory = memory;
    sim->queue = queue;
    sim->strategy = strategy;
    return sim;
}

void AllocationInstruction(memory_t* memory, StrategyType strategy, int blockId, int dimension) {
    switch (strategy) {
        case FIRST_FIT: {
            unsigned int currentAddress = 0;

            for (list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
                block_interval_t* block = (block_interval_t*)node->val;

                if (block == NULL) {
                    block_interval_t* newBlock = malloc(sizeof(block_interval_t));
                    newBlock->low_address = currentAddress;
                    newBlock->high_address = currentAddress + dimension - 1;

                    list_node_t* blockNode = list_node_new(newBlock);
                    list_rpush(memory->blocks, blockNode);

                    printf("AllocationInstruction: Allocated block: %d  dimension: %d addresses: %u-%u\n",
                           blockId, dimension, (unsigned int)newBlock->low_address, (unsigned int)newBlock->high_address);

                    return;
                } else {
                    currentAddress = block->high_address + 1;
                }
            }

            printf("AllocationInstruction failed: block: %d dimension: %d\n", blockId, dimension);
            break;
        }
        case BEST_FIT: {
            unsigned int currentAddress = 0;
            unsigned int best_fit_index = 0;
            unsigned int old_best_fit_size = 0; // Initialize with a large value
            block_interval_t* old_best_fit_val = NULL;

            for (list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
                block_interval_t* block = (block_interval_t*)node->val;

                if (block == NULL) {
                    unsigned int block_size = block->high_address - block->low_address;
                    if (block_size >= dimension) { // Enough memory to allocate to block.
                        unsigned int diff_current = abs(block_size - dimension);
                        unsigned int diff_old = abs(old_best_fit_size - dimension);
                        if (diff_current < diff_old) {
                            old_best_fit_val = block;
                            old_best_fit_size = block_size;
                        }
                    }
                }
            }

            if (old_best_fit_val != NULL) {
                block_interval_t* newBlock = malloc(sizeof(block_interval_t));
                newBlock->low_address = old_best_fit_val->low_address;
                newBlock->high_address = old_best_fit_val->low_address + dimension - 1;

                list_node_t* blockNode = list_node_new(newBlock);
                list_rpush(memory->blocks, blockNode);

                printf("BEST_FIT: Allocated block: %d dimension: %d addresses: %u-%u\n",
                       blockId, dimension, (unsigned int)newBlock->low_address, (unsigned int)newBlock->high_address);
            } else {
                printf("BEST_FIT: AllocationInstruction failed: block: %d dimension: %d\n", blockId, dimension);
            }
            break;
        }
        case WORST_FIT: {
            unsigned int currentAddress = 0;
            unsigned int worst_fit_start = 0;
            unsigned int worst_fit_size = 0;

            for (list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
                block_interval_t* block = (block_interval_t*)node->val;

                if (block == NULL) {
                    unsigned int currentBlockSize = 0;
                    list_node_t* nextNode = node;
                    while (nextNode != NULL && (block_interval_t*)nextNode->val == NULL) {
                        currentBlockSize++;
                        nextNode = nextNode->next;
                    }

                    if (currentBlockSize > worst_fit_size) {
                        worst_fit_size = currentBlockSize;
                        worst_fit_start = currentAddress;
                    }
                }

                if (block != NULL) {
                    currentAddress = block->high_address + 1;
                }
            }

            if (worst_fit_size >= dimension) {
                block_interval_t* newBlock = malloc(sizeof(block_interval_t));
                if (newBlock != NULL) {
                    newBlock->low_address = worst_fit_start;
                    newBlock->high_address = worst_fit_start + dimension - 1;

                    list_node_t* blockNode = list_node_new(newBlock);
                    list_rpush(memory->blocks, blockNode);

                    printf("WORST_FIT: Allocated block: %d dimension: %d addresses: %u-%u\n",
                           blockId, dimension, (unsigned int)newBlock->low_address, (unsigned int)newBlock->high_address);
                } else {
                    printf("WORST_FIT: AllocationInstruction failed: block: %d dimension: %d\n", blockId, dimension);
                }
            }
            break;
        }
        default:
            printf("invalid");
    }

}

void DeallocationInstruction(memory_t* memory, int blockId) {
    list_node_t* node = NULL;

    for (list_node_t* current = memory->blocks->head; current != NULL; current = current->next) {
        block_interval_t* block = (block_interval_t*)current->val;
        
        if (block != NULL && block->blockId == blockId) {
            node = current;
            break;
        }
    }

    if (node != NULL) {
        block_interval_t* block = (block_interval_t*)node->val;
        list_remove(memory->blocks, node);
        free(block);
    }
}



void CompactInstruction(memory_t* memory) {
    unsigned int currentAddress = 0;

    for (list_node_t* node = memory->blocks->head; node != NULL; node = node->next) {
        block_interval_t* block = (block_interval_t*)node->val; 

        if (block != NULL) {
            block->low_address = currentAddress;
            block->high_address = currentAddress + blockDimension(memory, currentAddress) - 1;
            currentAddress = block->high_address + 1;
        }
    }
    memory->size = currentAddress;
}


void run(simulation_instance_t* simulation, unsigned int steps) {
    unsigned int currentStep = 0;

    while (currentStep < steps && deque_count(*simulation->queue) > 0) {
        instruction_t* instruction = deque_peek(*simulation->queue);

        switch (instruction->type) {
            case ALLOCATION:
                AllocationInstruction(simulation->memory, simulation->strategy,
                                      instruction->blockId, instruction->dimension);
                break;
            case DEALLOCATION:
                DeallocationInstruction(simulation->memory, instruction->blockId);
                break;
            case COMPACT:
                CompactInstruction(simulation->memory);
                break;
            default:
                // nothing
                break;
        }
        deque_pop(*simulation->queue);
        currentStep++;
    }
}

void runAll(simulation_instance_t* simulation) {
    while (deque_count(*simulation->queue) > 0) {
        instruction_t* instruction = deque_peek(*simulation->queue);

        switch (instruction->type) {
            case ALLOCATION:
                AllocationInstruction(simulation->memory, simulation->strategy,
                                      instruction->blockId, instruction->dimension);
                break;
            case DEALLOCATION:
                DeallocationInstruction(simulation->memory, instruction->blockId);
                break;
            case COMPACT:
                CompactInstruction(simulation->memory);
                break;
            default:
                // nothing
                break;
        }

        deque_pop(*simulation->queue);
    }
}

void printSimulationDetails(simulation_instance_t* simulation, memory_t* memory) {
    printf("Simulation Details:\n");
    printf("Strategy: %s\n", StrategyTypeStr[simulation->strategy]);
    printf("List of Remaining Instructions: [");

    // Print remaining instructions
    while (deque_count(*simulation->queue) > 0) {
        instruction_t* instruction = deque_pop(*simulation->queue);
        printf("(%d, %d, %d)", instruction->type, instruction->blockId, instruction->dimension);

        if (deque_count(*simulation->queue) > 0) {
            printf(", ");
        }
    }
    printf("]\n");

    printf("Current Memory Structure:\n");

    // Print memory blocks
    list_t* blocks = simulation->memory->blocks;

    if (blocks->len == 0) {
        printf("Memory is empty.\n");
    } else {
        printf("Memory Blocks:\n");

        int blockId = 0;
        for (list_node_t* node = blocks->head; node != NULL; node = node->next) {
            block_interval_t* block = (block_interval_t*)node->val;

            printf("Block ID: %d, Low Address: %u, High Address: %u, Dimension: %u\n",
                   blockId, block->low_address, block->high_address, blockDimension(memory, blockId));

            blockId++;
        }
    }
}   


static int8_t compare_fn(const void * a,const void * b) {

    if (a > b) { return 1;}			 
    if (a < b) { return -1;}
  	if (a = b) { return 0;}
    return 0;
}

int main() {
    memory_t* memory = memory_init(10);
    Deque d = malloc(sizeof(struct deque_t));

	if (d != NULL) {
		deque_init(d, NULL);
	}
    Deque instructions = d;

    instruction_t allocInstruction1 = {ALLOCATION, 1, 3};
    instruction_t allocInstruction2 = {ALLOCATION, 3, 3};
    instruction_t allocInstruction3 = {ALLOCATION, 2, 3};
    instruction_t deallocInstruction1 = {DEALLOCATION, 0, 0};    
    instruction_t compactInstruction1 = {COMPACT, 0, 0};

    deque_append(instructions, &allocInstruction1);
    deque_append(instructions, &allocInstruction2);
    deque_append(instructions, &allocInstruction3);

    deque_append(instructions, &deallocInstruction1);
    
    deque_append(instructions, &compactInstruction1);

    simulation_instance_t* simulation = strategy_type_init(BEST_FIT, &instructions, memory);

    run(simulation, 2);
    runAll(simulation); 
    printSimulationDetails(simulation, memory);

    return 0;
    }


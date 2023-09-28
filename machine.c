#include <string.h>
#include "bof.h"
#include "instruction.h"
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)


static union mem_u {
     byte_type bytes[MEMORY_SIZE_IN_BYTES];
     word_type words[MEMORY_SIZE_IN_WORDS];
     bin_instr_t instrs[MEMORY_SIZE_IN_WORDS];
}memory;

int main(int argc,char* argv[])
{
    BOFFILE bf = bof_read_open(argv[1]);

    BOFHeader header = bof_read_header(bf);

    bin_instr_t instruction;

    //union mem_u memory;

    for(int i = 0; i < (header.text_length)/BYTES_PER_WORD;i++){
        instruction = instruction_read(bf);
        printf("%d instruction: op:%d rs: %d rt: %d  rd: %d shift: %d  func: %d\n",i,instruction.reg.op,instruction.reg.rs,instruction.reg.rt,instruction.reg.rd,instruction.reg.shift,instruction.reg.func);
        memory.instrs[i] = instruction;
    }
    for(int i = 0; i< (header.data_length)/4;i++){
        memory.words[header.data_start_address+(i*4)] = bof_read_word(bf);
    }
}
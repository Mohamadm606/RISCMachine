#include <string.h>
#include "bof.h"
#include "instruction.h"
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)
int GPR[31];

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
    printf("# of instructions%d\n",header.text_length);

    for(int i = 0; i < (header.text_length/4);i++){
        instruction = instruction_read(bf);
        printf("%d instruction: op:%d rs: %d rt: %d  rd: %d shift: %d  func: %d\n",i,instruction.reg.op,instruction.reg.rs,instruction.reg.rt,instruction.reg.rd,instruction.reg.shift,instruction.reg.func);
        memory.instrs[i] = instruction;
    }
    printf("test2\n");
    for(int i = header.data_start_address; i< (header.data_length)+header.data_start_address;i+=4){
        memory.words[i] = bof_read_word(bf);
        printf("%d ",memory.words[i]);
    }



    /*
    To do: global variable PC implementation
           Runtime Stack implemntation
    
    In regard to register access and use. 
        There are macros set in place already. They provide exra clarity and will be helpful once
        we start debugging i think. 
        regname.c and .h have basic info but should probably be researched more.




    Some notes on switch cluster
    Avoid using ints. Find built in macros in instructions.c for each operator. 

    */
    switch(instruction_type(instruction)){
        case(reg_instr_type):
            switch(instruction.reg.func){
                case(ADD_F):
                    break;
                case(SUB_F):
                    break;
                case(MUL_F):
                    break;
                case(DIV_F):
                    break;
                case(MFHI_F):
                    break;
                case(MFLO_F):
                    break;
                case(AND_F):
                    break;
                case(BOR_F):
                    break;
                case(NOR_F):
                    break;
                case(XOR_F):
                    break;
                case(SLL_F):
                    break;
                case(SRL_F):
                    break;
                case(JR_F):
                    break;
                case(SYSCALL_F):
                    break;
            }
            break;
        case(syscall_instr_type)://may need to go into Reg. but Instruction.c funtion (instruction_type) gives its own return??
            break;
        case(immed_instr_type)://use for inner switchinstruction_mnemonic 
            switch(instruction.immed.op){
                case(ADDI_O):
                    break;
                case(ANDI_O):
                    break;
                case(BORI_O):
                    break;
                case(XORI_O):
                    break;
                case(BEQ_O):
                    break;
                case(BGEZ_O):
                    break;
                case(BGTZ_O):
                    break;
                case(BLEZ_O):
                    break;    
                case(BLTZ_O):
                    break;
                case(BNE_O):
                    break;
                case(LBU_O):
                    break;
                case(LW_O):
                    break;
                case(SB_O):
                    break;
                case(SW_O):
                    break;
                

            break;
            }
            break;
        case(jump_instr_type):
            switch(instruction.jump.op)
            {
                case(2)://JUMP
                    break;
                case(3)://JUMP AND LINK
                    break;
            }
            break;
        case(error_instr_type):
                    break;
        
    };
}
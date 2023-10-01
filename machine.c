#include <string.h>
#include <stdlib.h>
#include "bof.h"
#include "instruction.h"
#include "utilities.h"
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)
int GPR[31];
int LO;
int HI;
int PC;
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
        memory.instrs[i] = instruction;
    }
    
    for(int i = header.data_start_address; i < (header.data_length)+header.data_start_address; i+=4) {
        memory.words[i] = bof_read_word(bf);
        printf("%d ",memory.words[i]);
    }



    /*
    To do: Runtime Stack implemntation
    
    In regard to register access and use. 
        There are macros set in place already. They provide exra clarity and will be helpful once
        we start debugging i think. 
        regname.c and .h have basic info but should probably be researched more.




    Some notes on switch cluster
    Avoid using ints. Find built in macros in instructions.c for each operator. 

    */

    int flag = 1;
    int length = header.text_length / 4;
    while (PC < length)
    {   
        instruction = memory.instrs[PC];
        //print instructions
        instr_type temp = instruction_type(instruction);
        PC += 4;
        switch(temp){
            case(reg_instr_type):
                switch(instruction.reg.func){
                    case(ADD_F):
                        GPR[instruction.reg.rd] = GPR[instruction.reg.rs] + GPR[instruction.reg.rt];
                        break;
                    case(SUB_F):
                        GPR[instruction.reg.rd] = GPR[instruction.reg.rs] - GPR[instruction.reg.rt];
                        break;
                    case(MUL_F):
                        long mulNum = GPR[instruction.reg.rs] * GPR[instruction.reg.rt];
                        HI = mulNum >> 32;
                        LO = mulNum << 32; 
                        break;
                    case(DIV_F):
                        HI = GPR[instruction.reg.rs] % GPR[instruction.reg.rt];
                        LO = GPR[instruction.reg.rs] / GPR[instruction.reg.rt];
                        break;
                    case(MFHI_F):
                        GPR[instruction.reg.rd] = HI;
                        break;
                    case(MFLO_F):
                        GPR[instruction.reg.rd] = LO;
                        break;
                    case(AND_F):
                        GPR[instruction.reg.rd] = GPR[instruction.reg.rs] & GPR[instruction.reg.rt];
                        break;
                    case(BOR_F):
                        GPR[instruction.reg.rd] = GPR[instruction.reg.rs] | GPR[instruction.reg.rt];
                        break;
                    case(NOR_F):
                        GPR[instruction.reg.rd] = ~(GPR[instruction.reg.rs] | GPR[instruction.reg.rt]);
                        break;
                    case(XOR_F):
                        GPR[instruction.reg.rd] = GPR[instruction.reg.rs] ^ GPR[instruction.reg.rt];
                        break;
                    case(SLL_F):
                        GPR[instruction.reg.rd] = GPR[instruction.reg.rt] << instruction.reg.shift;
                        break;
                    case(SRL_F):
                        GPR[instruction.reg.rd] = GPR[instruction.reg.rt] >> instruction.reg.shift;
                        break;
                    case(JR_F):
                        PC = GPR[instruction.reg.rs];
                        break;
                }
            case(syscall_instr_type)://may need to go into Reg. but Instruction.c funtion (instruction_type) gives its own return??
                switch(instruction.syscall.code) { 
                    case(exit_sc):
                        exit(0);
                        break;
                    case(print_str_sc):
                        // check
                        printf("%s", &memory.bytes[GPR[4]]);
                        GPR[2] = GPR[4];
                        break;
                    case(print_char_sc):
                        fputc(GPR[4], stdout);
                        GPR[2] = GPR[4];
                        break;
                    case(read_char_sc):
                        GPR[2] = getc(stdin);
                        break;
                    case(start_tracing_sc):
                        flag = 1;
                        break;
                    case(stop_tracing_sc):
                        flag = 0;
                        break;
                }
                break;
            case(immed_instr_type)://use for inner switchinstruction_mnemonic 
                switch(instruction.immed.op){
                    case(ADDI_O):
                        GPR[instruction.immed.rt] = GPR[instruction.immed.rs] + machine_types_sgnExt(instruction.immed.immed);
                        break;
                    case(ANDI_O):
                        GPR[instruction.immed.rt] = GPR[instruction.immed.rs] & machine_types_zeroExt(instruction.immed.immed);
                        break;
                    case(BORI_O):
                        GPR[instruction.immed.rt] = GPR[instruction.immed.rs] | machine_types_zeroExt(instruction.immed.immed);
                        break;
                    case(XORI_O):
                        GPR[instruction.immed.rt] = GPR[instruction.immed.rs] ^ machine_types_zeroExt(instruction.immed.immed);
                        break;
                    case(BEQ_O):
                        if(GPR[instruction.immed.rs]==GPR[instruction.immed.rt])
                            PC = PC + machine_types_formOffset(instruction.immed.immed);
                        break;
                    case(BGEZ_O):
                        if(GPR[instruction.immed.rs]>=0)
                            PC = PC + machine_types_formOffset(instruction.immed.immed);
                        break;
                    case(BGTZ_O):
                        if(GPR[instruction.immed.rs]>0)
                                PC = PC + machine_types_formOffset(instruction.immed.immed);
                        break;
                    case(BLEZ_O):
                        if(GPR[instruction.immed.rs]<=0)
                                    PC = PC + machine_types_formOffset(instruction.immed.immed);
                        break;    
                    case(BLTZ_O):
                        if(GPR[instruction.immed.rs]<0)
                                PC = PC + machine_types_formOffset(instruction.immed.immed);
                        break;
                    case(BNE_O):
                        if(GPR[instruction.immed.rs]!=GPR[instruction.immed.rt])
                                PC = PC + machine_types_formOffset(instruction.immed.immed);
                        break;
                    case(LBU_O):
                        GPR[instruction.immed.rt] = machine_types_zeroExt(memory.bytes[GPR[instruction.immed.rs] + machine_types_formOffset(instruction.immed.immed)]);
                        break;
                    case(LW_O):
                        GPR[instruction.immed.rt] = memory.words[GPR[instruction.immed.rs]+machine_types_formOffset(instruction.immed.immed)];
                        break;
                    case(SB_O):
                        memory.bytes[GPR[instruction.immed.rs]+machine_types_formOffset(instruction.immed.immed)] = GPR[instruction.immed.rt];
                        break;
                    case(SW_O):
                        memory.words[GPR[instruction.immed.rs]+machine_types_formOffset(instruction.immed.immed)] = GPR[instruction.immed.rt];
                        break;
                }
                break;
            case(jump_instr_type):
                switch(instruction.jump.op)
                {
                    case(JMP_O)://JUMP
                        PC = machine_types_formAddress(PC, instruction.jump.addr);
                        break;
                    case(JAL_O)://JUMP AND LINK
                        GPR[31] = PC;
                        PC = machine_types_formAddress(PC, instruction.jump.addr);
                        break;
                }
                break;
            case(error_instr_type):
                bail_with_error("uh oh");
                break;
            
        };
    }
}

void print_instr(instr_type instr, bin_instr_t bin_Instr)
{
    if ((instr == syscall_instr_type) && (bin_Instr.syscall.code == exit_sc))
    {

        fprintf(stdout, "\t%d EXIT\n", &PC);
        fprintf(stdout, " 1024: 0	...\n");
        return;
    }

}

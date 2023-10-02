#include <string.h>
#include <stdlib.h>
#include "bof.h"
#include "instruction.h"
#include "utilities.h"
#include "regname.h"
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

   // re read manual for types of registers we should not let user write to

    int flag = 1;
    int length = header.text_length;
    
    fprintf(stdout,"Addr Instruction\n");
    while (PC < length)
    {   
        // enforcing required register stuff
        if (!enforce_invarients())
            bail_with_error("uh oh");
        
        // PRINT FORMATING DOES NOT MATTER ONLY NEW LINE MUST BE CORRECT

        //printf("PC = %d Length is %d\n",PC,length);
        instruction = memory.instrs[PC/4];
        instr_type temp = instruction_type(instruction);

        //print instructions
        print_instr(temp,instruction);

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
            case(syscall_instr_type):
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
    if(PC < 10)
        fpirntf(stdout, "   ");
    else if (PC >= 10 && PC < 100)
        fpirntf(stdout, "  ");
    else if (PC >= 100 && PC < 1000)
        fpirntf(stdout, " ");
    else
        fprintf(stdout, "");
        
    fprintf(stdout,"%d ",PC);
    switch (instr)
    {
        case(reg_instr_type):
            fprintf(stdout,"%s",instruction_func2name(bin_Instr));
            break;
        case(immed_instr_type):
            fprintf(stdout,"%s",instruction_mnemonic(bin_Instr));
            break;
        case(jump_instr_type):
            fprintf(stdout,"%s",instruction_mnemonic(bin_Instr));
            break;
        case(syscall_instr_type):
            fprintf(stdout,"%s",instruction_syscall_mnemonic(bin_Instr.syscall.code));
            break;
    }
    if ((instr == syscall_instr_type) && (bin_Instr.syscall.code == exit_sc))
        fprintf(stdout, "\n     1024: 0 ...");
    fprintf(stdout,"\n");
    return;
}

void print_reg(instr_type instr, bin_instr_t bin_Instr) {
    printf("\t%d\t", PC);

    if (HI || LO)
    {
        printf("HI: %d\t LO: %d\n", HI, LO);
    }
    else
        printf("\n");

    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        if (i % 6 == 0)
            printf("\n");
        
        printf("GPR[%s]: %d\t", regname_get(i), GPR[i]);
    }
    
    printf("\n");

    // implement data and stack nums here and figure
    // out what they are

    // page 4 of manual
    // numbers at bottom are $gp and $sp
    // something about how the words are stored in mem

    printf("==> addr:");

    print_instr(instr, bin_Instr);
}

int enforce_invarients() {
    if (PC % BYTES_PER_WORD != 0 || GPR[28] % BYTES_PER_WORD != 0 || GPR[29] % BYTES_PER_WORD != 0
        || GPR[30] % BYTES_PER_WORD != 0 || 0 > GPR[28] || GPR[28] >= GPR[29] || GPR[29] > GPR[30]
        || GPR[30] >= MEMORY_SIZE_IN_BYTES || 0 > PC || PC >= MEMORY_SIZE_IN_BYTES || GPR[0] != 0)
        return 0;
    else
        return 1;

}

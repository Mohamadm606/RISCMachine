#include <string.h>
#include <stdlib.h>
#include "bof.h"
#include "instruction.h"
#include "utilities.h"
#include "regname.h"
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)
int GPR[31];
long LO;
long HI;
int PC;

static union mem_u {
     byte_type bytes[MEMORY_SIZE_IN_BYTES];
     word_type words[MEMORY_SIZE_IN_WORDS];
     bin_instr_t instrs[MEMORY_SIZE_IN_WORDS];
}memory;

void print_instr();
void initialize(BOFHeader header);
void print_data(BOFHeader header);
void print_stack(BOFHeader header);
void print_reg();
int enforce_invarients();
int main(int argc,char* argv[])
{
    BOFFILE bf;
    
    if (argc < 3)
        bf = bof_read_open(argv[1]);
    else
        bf = bof_read_open(argv[2]);

    BOFHeader header = bof_read_header(bf);

    initialize(header);

    bin_instr_t instruction;


    for(int i = 0; i < (header.text_length/4);i++){
        instruction = instruction_read(bf);
        memory.instrs[i] = instruction;
    }
    
    for(int i = header.data_start_address; i < (header.data_length)+header.data_start_address; i+=4) {
        memory.words[i] = bof_read_word(bf);
    }

    // make
    // make asm
    // ./asm vm_test3.asm
    // ./vm vm_test3.bof


    int length = header.text_length;

    fprintf(stdout,"Addr Instruction\n");

    while (PC < length)
    {
        print_instr();
        PC+=4;
    }
    print_data(header);
    
    PC = header.text_start_address;


    int flag = 1;

    while (1)
    {   
        // enforcing required register stuff
        if (!enforce_invarients())
            bail_with_error("test: uh oh");
        
        instruction = memory.instrs[PC/4];
        instr_type temp = instruction_type(instruction);


        if (flag)
        {
            print_reg();
            print_data(header);
            print_stack(header);
            fprintf(stdout,"==> addr:\t");
            print_instr();
        }

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
                        HI = (int) (mulNum >> 32);
                        mulNum = (mulNum << 32);
                        mulNum = (mulNum >> 32);
                        LO = (int) mulNum;
                        break;
                    case(DIV_F):
                        HI = (int)GPR[instruction.reg.rs] % GPR[instruction.reg.rt];
                        LO = (int)GPR[instruction.reg.rs] / GPR[instruction.reg.rt];
                        break;
                    case(MFHI_F):
                        GPR[instruction.reg.rd] = (int) HI;
                        break;
                    case(MFLO_F):
                        GPR[instruction.reg.rd] = (int) LO;
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
                break;
            case(syscall_instr_type):
                switch(instruction.syscall.code) { 
                    case(exit_sc):
                        exit(0);
                        break;
                    case(print_str_sc)://PSTR
                        GPR[2] = fprintf(stdout, "%s", &memory.bytes[GPR[4]]);
                        break;
                    case(print_char_sc)://PCH
                        GPR[2] = fputc(GPR[4], stdout);
                        break;
                    case(read_char_sc)://RCH
                        GPR[4] = fgetc(stdin);
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
                        GPR[instruction.immed.rt] = GPR[instruction.immed.rs] + machine_types_sgnExt((short int)instruction.immed.immed);
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
                        GPR[RA] = PC;
                        PC = machine_types_formAddress(PC,instruction.jump.addr);
                        break;
                }
                break;
            case(error_instr_type):
                bail_with_error("uh oh");
                break;
        };
    }
}
void print_reg() {
    fprintf(stdout, "\tPC: %d\t", PC);

    if (HI || LO)
    {
        fprintf(stdout, "HI: %d\t LO: %d\n",(int)HI, (int)LO);
    }
    else
        fprintf(stdout, "\n");

    fprintf(stdout, "\n");

    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        if (i % 6 == 0)
            fprintf(stdout, "\n");
        
        fprintf(stdout, "GPR[%s]: %d\t", regname_get(i), GPR[i]);
    }
    
    fprintf(stdout, "\n");
}

int enforce_invarients(){
    if (PC % BYTES_PER_WORD != 0 || GPR[GP] % BYTES_PER_WORD != 0 || GPR[SP] % BYTES_PER_WORD != 0
        || GPR[SP] % BYTES_PER_WORD != 0 || 0 > GPR[GP] || GPR[GP] >= GPR[SP] || GPR[SP] > GPR[FP]
        || GPR[FP] >= MEMORY_SIZE_IN_BYTES || 0 > PC || PC >= MEMORY_SIZE_IN_BYTES || GPR[0] != 0)
        return 0;
    else
        return 1;

}
void print_instr()
{
        bin_instr_t instruction = memory.instrs[PC/4];
        fprintf(stdout,"%4d ",PC);
        
        switch (instruction_type(instruction))
        {
           case(reg_instr_type):
                fprintf(stdout,"%s",instruction_func2name(instruction));
                switch(instruction.reg.func){
                    case(ADD_F): case(SUB_F): 
                        fprintf(stdout," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(MUL_F):
                        fprintf(stdout," %s, %s",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt));
                        break;
                    case(DIV_F):
                        fprintf(stdout," %s, %s",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt));
                        break;
                    case(MFHI_F):
                        fprintf(stdout," %s",regname_get(instruction.reg.rd));
                        break;
                    case(MFLO_F):
                        fprintf(stdout," %s",regname_get(instruction.reg.rd));
                        break;
                    case(AND_F):
                        fprintf(stdout," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(BOR_F):
                        fprintf(stdout," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(NOR_F):
                        fprintf(stdout," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(XOR_F):
                        fprintf(stdout," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(SLL_F):
                        fprintf(stdout," %s, %s, %d ",regname_get(instruction.reg.rt),regname_get(instruction.reg.rd),instruction.reg.shift);
                        break;
                    case(SRL_F):
                        fprintf(stdout," %s, %s, %d ",regname_get(instruction.reg.rt),regname_get(instruction.reg.rd),instruction.reg.shift);
                        break;
                    case(JR_F):
                        fprintf(stdout," %s",regname_get(instruction.reg.rs));
                        break;
                }
                break;
            case(syscall_instr_type):
                fprintf(stdout,"%s",instruction_syscall_mnemonic(instruction.syscall.code));
                break;
            case(immed_instr_type)://use for inner switchinstruction_mnemonic 
                fprintf(stdout,"%s",instruction_mnemonic(instruction));
                switch(instruction.immed.op){
                    case(ADDI_O):
                        fprintf(stdout," %s, %s, %hd ",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),(short int)instruction.immed.immed);
                        break;
                   case ANDI_O: case BORI_O: case XORI_O:
	                    fprintf(stdout, "%s, %s, 0x%hx",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed);
	                    break;
                    case(BEQ_O):
                        fprintf(stdout," %s, %s, %d #offset is %+d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BGEZ_O):
                        fprintf(stdout," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BGTZ_O):
                        fprintf(stdout," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BLEZ_O):
                        fprintf(stdout," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;    
                    case(BLTZ_O):
                        fprintf(stdout," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BNE_O):
                        fprintf(stdout," %s, %s, %d    #offset is +%d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                   case LBU_O: case LW_O: case SB_O: case SW_O:
	                    fprintf(stdout, " %s, %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),(short int) instruction.immed.immed,(short int)machine_types_formOffset(instruction.immed.immed));
	                    break;
                }
                break;
            case(jump_instr_type):
                fprintf(stdout,"%s",instruction_mnemonic(instruction));
                fprintf(stdout," %d # target is byte address %d",instruction.jump.addr,machine_types_formAddress(PC, instruction.jump.addr)); 
                break;
            case(error_instr_type):
                break;
        }
        fprintf(stdout,"\n");
    return;
}


void print_data(BOFHeader header){
    //fprintf(stdout,"      %d: %d",header.data_start_address,((header.data_length)+header.data_start_address));
    for(int i = header.data_start_address; i <= ((header.data_length)+header.data_start_address); i+=4)
        fprintf(stdout,"\t  %d: %d",i,memory.words[i]);
    fprintf(stdout," ...\n");
}


void print_stack(BOFHeader)
{
    for(int i = GPR[SP];i <= GPR[FP]; i+=4)
    {
        fprintf(stdout,"\t  %d: %d",i,memory.words[i]);
    } 
    fprintf(stdout," ...\n");
}


void initialize(BOFHeader header) {

    for (int i = 0; i < NUM_REGISTERS; i++)
        GPR[i] = 0;
    
    GPR[GP] = header.data_start_address;
    GPR[FP] = header.stack_bottom_addr;
    GPR[SP] = header.stack_bottom_addr;

    PC = header.text_start_address;

    HI = 0;
    LO = 0;
}

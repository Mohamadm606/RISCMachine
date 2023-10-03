#include <string.h>
#include <stdlib.h>
#include "bof.h"
#include "instruction.h"
#include "utilities.h"
#include "regname.h"
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)
int GPR[32];
long LO;
long HI;
int PC;

static union mem_u {
     byte_type bytes[MEMORY_SIZE_IN_BYTES];
     word_type words[MEMORY_SIZE_IN_WORDS];
     bin_instr_t instrs[MEMORY_SIZE_IN_WORDS];
}memory;

void print_instr(FILE* file);
void initialize(BOFHeader header);
void print_data(BOFHeader header, FILE* file);
void print_stack(BOFHeader header);
void print_reg();
int enforce_invarients();
int main(int argc,char* argv[])
{
    BOFFILE bf;
    
    int dashp = 0;
    if (argc < 3)
    {
        bf = bof_read_open(argv[1]);
    }
    else
    {
        dashp = 1;
        bf = bof_read_open(argv[2]);
    }

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


    int length = header.text_length;
    if (dashp)
    {

        fprintf(stdout,"Addr Instruction\n");

        while (PC < length)
        {
            print_instr(stdout);
            PC+=4;
        }
        print_data(header, stdout);

        fclose(stdout);
        exit(0);    
    }
    PC = header.text_start_address;
    int flag = 1;
    long mulNum = 0;
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
            print_data(header, stdout);
            print_stack(header);
            fprintf(stdout,"==> addr:");
            print_instr(stdout);
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
                        mulNum = GPR[instruction.reg.rs] * GPR[instruction.reg.rt];
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
                        fprintf(stdout,"\n");
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
                        GPR[instruction.immed.rt] = (unsigned short)machine_types_zeroExt((unsigned short)memory.words[(unsigned short)GPR[instruction.immed.rs] + (unsigned short)machine_types_formOffset(instruction.immed.immed)]);
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
    fprintf(stdout, "      PC: %-3d", PC);

    if (HI || LO)
    {
        fprintf(stdout, "\tHI: %d\t LO: %d",(int)HI, (int)LO);
    }

    fprintf(stdout, "\n");

    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        if (i % 6 == 0 && i!= 0)
            fprintf(stdout, "\n");
        
        fprintf(stdout, "GPR[%-3s]: %-12d", regname_get(i), GPR[i]);
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
void print_instr(FILE* file)
{
        bin_instr_t instruction = memory.instrs[PC/4];
        fprintf(file,"%4d ",PC);
        
        switch (instruction_type(instruction))
        {
           case(reg_instr_type):
                fprintf(file,"%s",instruction_func2name(instruction));
                switch(instruction.reg.func){
                    case(ADD_F): case(SUB_F): 
                        fprintf(file," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(MUL_F):
                        fprintf(file," %s, %s",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt));
                        break;
                    case(DIV_F):
                        fprintf(file," %s, %s",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt));
                        break;
                    case(MFHI_F):
                        fprintf(file," %s",regname_get(instruction.reg.rd));
                        break;
                    case(MFLO_F):
                        fprintf(file," %s",regname_get(instruction.reg.rd));
                        break;
                    case(AND_F):
                        fprintf(file," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(BOR_F):
                        fprintf(file," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(NOR_F):
                        fprintf(file," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(XOR_F):
                        fprintf(file," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(SLL_F):
                        fprintf(file," %s, %s, %d ",regname_get(instruction.reg.rt),regname_get(instruction.reg.rd),instruction.reg.shift);
                        break;
                    case(SRL_F):
                        fprintf(file," %s, %s, %d ",regname_get(instruction.reg.rt),regname_get(instruction.reg.rd),instruction.reg.shift);
                        break;
                    case(JR_F):
                        fprintf(file," %s",regname_get(instruction.reg.rs));
                        break;
                }
                break;
            case(syscall_instr_type):
                fprintf(file,"%s",instruction_syscall_mnemonic(instruction.syscall.code));
                break;
            case(immed_instr_type)://use for inner switchinstruction_mnemonic 
                fprintf(file,"%s",instruction_mnemonic(instruction));
                switch(instruction.immed.op){
                    case(ADDI_O):
                        fprintf(file," %s, %s, %hd ",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),(short int)instruction.immed.immed);
                        break;
                   case ANDI_O: case BORI_O: case XORI_O:
	                    fprintf(file, "%s, %s, 0x%hx",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed);
	                    break;
                    case(BEQ_O):
                        fprintf(file," %s, %s, %d #offset is %+d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BGEZ_O):
                        fprintf(file," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BGTZ_O):
                        fprintf(file," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BLEZ_O):
                        fprintf(file," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;    
                    case(BLTZ_O):
                        fprintf(file," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BNE_O):
                        fprintf(file," %s, %s, %d    #offset is +%d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                   case LBU_O: case LW_O: case SB_O: case SW_O:
	                    fprintf(file, " %s, %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),(short int) instruction.immed.immed,(short int)machine_types_formOffset(instruction.immed.immed));
	                    break;
                }
                break;
            case(jump_instr_type):
                fprintf(file,"%s",instruction_mnemonic(instruction));
                fprintf(file," %d # target is byte address %d",instruction.jump.addr,machine_types_formAddress(PC, instruction.jump.addr)); 
                break;
            case(error_instr_type):
                break;
        }
        fprintf(file,"\n");
    return;
}


void print_data(BOFHeader header, FILE* file){
    int count = 0;
    int dots = 0;
    for(int i = header.data_start_address; i <= ((header.data_length)+header.data_start_address); i+=4)
    {
        if (count % 5 == 0 && count != 0)
            fprintf(file, "\n");
        if(memory.words[i]!=0){
            fprintf(file,"\t  %d: %d",i,memory.words[i]);
            count++;
        }else if (dots == 0)
        {
            dots = 1;
            fprintf(file,"    %d: %d ...",i,memory.words[i]);
            count++;
        }
        
    }
    fprintf(stdout,"\n");
}


void print_stack(BOFHeader)
{
    int count = 0;
    int dots = 0;
    for(int i = GPR[SP];i <= GPR[FP]; i+=4)
    {
        if (count % 5 == 0 && count != 0){
            fprintf(stdout, "\n");
        }
        if(memory.words[i] != 0){
            fprintf(stdout,"\t  %d: %d",i,memory.words[i]);
            count++;
            dots = 0;
        }else if(dots == 0)
        {
            dots = 1;
            fprintf(stdout,"    %d: %d ...",i,memory.words[i]);
            count++;
        }
    } 
    fprintf(stdout,"\n");
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

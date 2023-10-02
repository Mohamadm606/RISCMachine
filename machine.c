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

void print_instr(FILE* f);
void initialize(BOFHeader header);
void print_data(BOFHeader header, FILE* f);
void print_stack(BOFHeader, FILE* f);
int main(int argc,char* argv[])
{
    BOFFILE bf = bof_read_open(argv[1]);

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


    int dataLength = header.data_length;
    int length = header.text_length;
    int dashp = strcmp(argv[1], "-p") == 0;
    FILE* myo; 
    FILE* myp;

    if (!dashp)
    {
        myp = fopen("out.myp", "w+");
        myo = fopen("out.myp", "w+");
    }

    if (dashp)
        fprintf(stdout,"Addr Instruction\n");
    else
        fprintf(myp,"Addr Instruction\n");

    while (PC < length)
    {
        if (!dashp)
            print_instr(myp);
        else
            print_instr(stdout);
        PC+=4;
    }
    
    if (dashp)
    {
        print_data(header, stdout);
    }
    else
        print_data(header, myp);

    PC = 0;

    if(!strcmp(argv[1], "-p"))
        exit(0);

    int flag = 1;
    while (PC < length)
    {   
        // enforcing required register stuff
        if (!enforce_invarients())
            bail_with_error("test: uh oh");
        
        instruction = memory.instrs[PC/4];
        instr_type temp = instruction_type(instruction);

        if (flag)
        {
            print_reg(myo);
            print_data(header, myo);
            print_stack(header, myo);
            fprintf(myo, "==> addr: ");
            print_instr(myo);
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

void print_reg(FILE* f) {
    printf("\tPC: %d\t", PC);

    if (HI || LO)
    {
        fprintf(f, "HI: %d\t LO: %d\n", HI, LO);
    }
    else
        fprintf(f, "\n");

    for (int i = 0; i < NUM_REGISTERS; i++)
    {
        if (i % 6 == 0)
            fprintf(f, "\n");
        
        fprintf(f, "GPR[%s]: %d\t", regname_get(i), GPR[i]);
    }
    
    fprintf(f, "\n");
}

int enforce_invarients(){
    if (PC % BYTES_PER_WORD != 0 || GPR[GP] % BYTES_PER_WORD != 0 || GPR[SP] % BYTES_PER_WORD != 0
        || GPR[SP] % BYTES_PER_WORD != 0 || 0 > GPR[GP] || GPR[GP] >= GPR[SP] || GPR[SP] > GPR[FP]
        || GPR[FP] >= MEMORY_SIZE_IN_BYTES || 0 > PC || PC >= MEMORY_SIZE_IN_BYTES || GPR[0] != 0)
        return 0;
    else
        return 1;

}
void print_instr(FILE* f)
{
        bin_instr_t instruction = memory.instrs[PC/4];
        fprintf(f,"%4d ",PC);
        
        switch (instruction_type(instruction))
        {
           case(reg_instr_type):
                fprintf(f,"%s",instruction_func2name(instruction));
                switch(instruction.reg.func){
                    case(ADD_F): case(SUB_F): 
                        fprintf(f," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(MUL_F):
                        fprintf(f," %s, %s",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt));
                        break;
                    case(DIV_F):
                        fprintf(f," %s, %s",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt));
                        break;
                    case(MFHI_F):
                        fprintf(f," %s",regname_get(instruction.reg.rd));
                        break;
                    case(MFLO_F):
                        fprintf(f," %s",regname_get(instruction.reg.rd));
                        break;
                    case(AND_F):
                        fprintf(f," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(BOR_F):
                        fprintf(f," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(NOR_F):
                        fprintf(f," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(XOR_F):
                        fprintf(f," %s, %s, %s ",regname_get(instruction.reg.rs),regname_get(instruction.reg.rt),regname_get(instruction.reg.rd));
                        break;
                    case(SLL_F):
                        fprintf(f," %s, %s, %d ",regname_get(instruction.reg.rt),regname_get(instruction.reg.rd),instruction.reg.shift);
                        break;
                    case(SRL_F):
                        fprintf(f," %s, %s, %d ",regname_get(instruction.reg.rt),regname_get(instruction.reg.rd),instruction.reg.shift);
                        break;
                    case(JR_F):
                        fprintf(f," %s",regname_get(instruction.reg.rs));
                        break;
                }
                break;
            case(syscall_instr_type):
                fprintf(f,"%s",instruction_syscall_mnemonic(instruction.syscall.code));
                break;
            case(immed_instr_type)://use for inner switchinstruction_mnemonic 
                fprintf(f,"%s",instruction_mnemonic(instruction));
                switch(instruction.immed.op){
                    case(ADDI_O):
                        fprintf(f," %s, %s, %hd ",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),(short int)instruction.immed.immed);
                        break;
                   case ANDI_O: case BORI_O: case XORI_O:
	                    fprintf(f, "%s, %s, 0x%hx",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed);
	                    break;
                    case(BEQ_O):
                        fprintf(f," %s, %s, %d #offset is %+d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BGEZ_O):
                        fprintf(f," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BGTZ_O):
                        fprintf(f," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BLEZ_O):
                        fprintf(f," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;    
                    case(BLTZ_O):
                        fprintf(f," %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                    case(BNE_O):
                        fprintf(f," %s, %s, %d    #offset is +%d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),instruction.immed.immed,machine_types_formOffset(instruction.immed.immed));
                        break;
                   case LBU_O: case LW_O: case SB_O: case SW_O:
	                    fprintf(f, " %s, %s, %d    #offset is %+d bytes",regname_get(instruction.immed.rs),regname_get(instruction.immed.rt),(short int) instruction.immed.immed,(short int)machine_types_formOffset(instruction.immed.immed));
	                    break;
                }
                break;
            case(jump_instr_type):
                fprintf(f,"%s",instruction_mnemonic(instruction));
                fprintf(f," %d # target is byte address %d",instruction.jump.addr,machine_types_formAddress(PC, instruction.jump.addr)); 
                break;
            case(error_instr_type):
                break;
        }
        fprintf(f,"\n");
    return;
}


void print_data(BOFHeader header, FILE* f){
    //fprintf(stdout,"      %d: %d",header.data_start_address,((header.data_length)+header.data_start_address));
    for(int i = header.data_start_address; i <= ((header.data_length)+header.data_start_address); i+=4)
        fprintf(f,"\t  %d: %d",i,memory.words[i]);
    fprintf(f," ...\n");
}


void print_stack(BOFHeader, FILE* f)
{
    for(int i = GPR[SP];i <= GPR[FP]; i+=4)
    {
        fprintf(f,"\t  %d: %d",i,memory.words[i]);
    } 
    fprintf(f," ...\n");
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

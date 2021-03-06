enum struct Inst : uint8_t
{
    LUI = 0b0110111,
    AUIPC = 0b0010111,
    JAL = 0b1101111,
    JALR = 0b1100111,
    BRANCH = 0b1100011,
    LOAD = 0b0000011,
    STORE = 0b0100011,
    ALUI = 0b0010011,
    ALU = 0b0110011,
    FENCE = 0b0001111, // not yet implemented
    // this is SYSTEM
    /*ECALL = 0b1110011,  // not yet implemented
    EBREAK = 0b1110011, // not yet implemented*/
    FLOAD = 0b0000111,
    FSTORE = 0b0100111,
    FPU = 0b1010011,
    SYSTEM = 0b1110011,
};

enum struct Branch_Inst : uint8_t
{
    BEQ = 0b000,
    BNE = 0b001,
    BLT = 0b100,
    BGE = 0b101,
    BLTU = 0b110,
    BGEU = 0b111,
};

enum struct Load_Inst : uint8_t
{
    LB = 0b000,
    LH = 0b001,
    LW = 0b010,
    LBU = 0b100,
    LHU = 0b101,
};

enum struct Store_Inst : uint8_t
{
    SB = 0b000,
    SH = 0b001,
    SW = 0b010
};

enum struct ALUI_Inst : uint8_t
{
    ADDI = 0b000,
    SLTI = 0b010,
    SLTIU = 0b011,
    XORI = 0b100,
    ORI = 0b110,
    ANDI = 0b111,
    SLLI = 0b001,
    SRI = 0b101,
};

enum struct ALUI_SRI_Inst : uint8_t
{
    SRLI = 0b0000000,
    SRAI = 0b0100000,
};

enum struct ALU_ADD_SUB_Inst : uint8_t
{
    ADD = 0b0000000,
    SUB = 0b0100000,
};

enum struct ALU_SR_Inst : uint8_t
{
    SRL = 0b0000000,
    SRA = 0b0100000,
};

enum struct ALU_Inst : uint8_t
{
    ADD_SUB = 0b000,
    SLL = 0b001,
    SLT = 0b010,
    SLTU = 0b011,
    XOR = 0b100,
    SR = 0b101,
    OR = 0b110,
    AND = 0b111,
};

//not yet implemented
enum struct Fence_Inst : uint8_t
{
    FENCE = 0b000,
    FENCEI = 0b001,
};

enum struct FLoad_Inst : uint8_t
{
    FLW = 0b010,
};

enum struct FStore_Inst : uint8_t
{
    FSW = 0b010,
};

enum struct FSgnj_Inst : uint8_t
{
    FSGNJ = 0b000,
    FSGNJN = 0b001,
    FSGNJX = 0b010
};

enum struct FComp_Inst : uint8_t
{
    FEQ = 0b010,
    FLT = 0b001,
    FLE = 0b000,
};

enum struct FPU_Inst : uint8_t
{
    FADD = 0b0000000,
    FSUB = 0b0000100,
    FMUL = 0b0001000,
    FDIV = 0b0001100,
    FSQRT = 0b0101100,
    FCVT_W_S = 0b1100000,
    FCOMP = 0b1010000,
    FMV_X_W = 0b1110000,
    FCVT_S_W = 0b1101000,
    FMV_W_X = 0b1111000,
    FSGNJ = 0b0010000,
};

enum struct Mul_Div_Inst : uint8_t
{
    MUL = 0b000,
    MULH = 0b001,
    MULHSU = 0b010,
    MULHU = 0b011,
    DIV = 0b100,
    DIVU = 0b101,
    REM = 0b110,
    REMU = 0b111,
};

enum struct System_Inst : uint8_t
{
    PRIV = 0b000,
    CSRRW = 0b001,
    CSRRS = 0b010,
    CSRRC = 0b011,
    CSRRWI = 0b101,
    CSRRSI = 0b110,
    CSRRCI = 0b111,
};

enum struct Priv_Inst : uint8_t
{
    URET = 0b0000000,
    SRET = 0b0001000,
    MRET = 0b0011000,
    ECALL = 0b0000000,
};

enum struct CSR : uint16_t
{
    SATP = 0x180,
    SSTATUS = 0x100,
    SEPC = 0x141,
    SSCRATCH = 0x140,
    SIE = 0x104,
    STVEC = 0x105,
    SCAUSE = 0x142,
    SIP = 0x144,
    STVAL = 0x143,
};
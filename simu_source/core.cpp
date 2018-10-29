class ALU
{
  public:
    static uint32_t add(uint32_t x, uint32_t y)
    {
        return x + y;
    }
    static uint32_t sub(uint32_t x, uint32_t y)
    {
        return x - y;
    }
    static uint32_t sll(uint32_t x, uint32_t y)
    {
        return x << (y & 0b11111);
    }
    static uint32_t srl(uint32_t x, uint32_t y)
    {
        return x >> (y & 0b11111);
    }
    static uint32_t sra(uint32_t x, uint32_t y)
    {
        int32_t a = (int32_t)x;
        return (uint32_t)(x >> (y & 0b11111));
    }
    static uint32_t slt(uint32_t x, uint32_t y)
    {
        int32_t a = (int32_t)x;
        int32_t b = (int32_t)y;
        return a > b;
    }
    static uint32_t sltu(uint32_t x, uint32_t y)
    {
        return x > y;
    }
    static uint32_t and_(uint32_t x, uint32_t y)
    {
        return x & y;
    }
    static uint32_t or_(uint32_t x, uint32_t y)
    {
        return x | y;
    }
    static uint32_t xor_(uint32_t x, uint32_t y)
    {
        return x ^ y;
    }
};

class FPU
{
  public:
    static float fadd(float x, float y)
    {
        return x + y;
    }
    static float fsub(float x, float y)
    {
        return x - y;
    }
    static float fmul(float x, float y)
    {
        return x * y;
    }
    static float fdiv(float x, float y)
    {
        return x / y;
    }
    static float fsqrt(float x)
    {
        return std::sqrt(x);
    }

    static uint32_t feq(float x, float y)
    {
        if (x == y) return 1;
        else return 0;
    }
    static uint32_t flt(float x, float y)
    {
        if (x < y) return 1;
        else return 0;
    }
    static uint32_t fle(float x, float y)
    {
        if (x <= y) return 1;
        else return 0;
    }
    static uint32_t float2int(float x)
    {
        int y = int(x);
        return y;
    }
    static float int2float(int x)
    {
        float y = float(x);
        return y;
    }
};


class Core
{
    const uint32_t instruction_load_address = 0;
    const int default_stack_pointer = 2;
    const int default_stack_dump_size = 48;
    Memory *m;
    Register *r;
    IO *io;
    Stat *stat;

    Settings *settings;

    void lui(Decoder *d)
    {
        uint32_t val = d->u_type_imm();
        r->set_ireg(d->rd(), val);
        (stat->stat_lui)++;
    }
    void auipc(Decoder *d)
    {
        // sign extended
        int32_t val = d->u_type_imm();
        val += (int32_t)(r->ip);
        r->set_ireg(d->rd(), val);
        (stat->stat_auipc)++;
    }

    void jal(Decoder *d)
    {
        int32_t imm = d->jal_imm();
        r->set_ireg(d->rd(), r->ip + 4);
        r->ip = (int32_t)r->ip + imm;
        (stat->stat_jal)++;
    }
    void jalr(Decoder *d)
    {
        // sign extended
        int32_t imm = d->i_type_imm();
        int32_t s = r->get_ireg(d->rs1());
        r->set_ireg(d->rd(), r->ip + 4);
        r->ip = s + imm;
        (stat->stat_jalr)++;
    }

    void branch_inner(Decoder *d, int flag)
    {
        if (flag)
        {
            r->ip = (int32_t)r->ip + d->b_type_imm();
        }
        else
        {
            r->ip += 4;
        }
    }
    void beq(Decoder *d)
    {
        branch_inner(d, r->get_ireg(d->rs1()) == r->get_ireg(d->rs2()));
        (stat->stat_beq)++;
    }
    void bne(Decoder *d)
    {
        branch_inner(d, r->get_ireg(d->rs1()) != r->get_ireg(d->rs2()));
        (stat->stat_bne)++;
    }
    void blt(Decoder *d)
    {
        branch_inner(d, (int32_t)r->get_ireg(d->rs1()) < (int32_t)r->get_ireg(d->rs2()));
        (stat->stat_blt)++;
    }
    void bge(Decoder *d)
    {
        branch_inner(d, (int64_t)r->get_ireg(d->rs1()) >= (int64_t)r->get_ireg(d->rs2()));
        (stat->stat_bge)++;
    }
    void bltu(Decoder *d)
    {
        branch_inner(d, r->get_ireg(d->rs1()) < r->get_ireg(d->rs2()));
        (stat->stat_bltu)++;
    }
    void bgeu(Decoder *d)
    {
        branch_inner(d, r->get_ireg(d->rs1()) >= r->get_ireg(d->rs2()));
        (stat->stat_bgeu)++;
    }

    void lb(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_1(addr);
        r->set_ireg(d->rd(), val);
        (stat->stat_lb)++;
    }
    void lh(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_2(addr);
        r->set_ireg(d->rd(), val);
        (stat->stat_lh)++;
    }
    void lw(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_4(addr);
        r->set_ireg(d->rd(), val);
        (stat->stat_lw)++;
    }
    void lbu(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t offset = d->i_type_imm();
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_1(addr);
        r->set_ireg(d->rd(), val);
        (stat->stat_lbu)++;
    }
    void lhu(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t offset = d->i_type_imm();
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_2(addr);
        r->set_ireg(d->rd(), val);
        (stat->stat_lhu)++;
    }

    void sb(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t src = r->get_ireg(d->rs2());
        src <<= 24;
        src >>= 24;
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src);
        (stat->stat_sb)++;
    }

    void sh(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t src = r->get_ireg(d->rs2());
        src <<= 16;
        src >>= 16;
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src);
        (stat->stat_sh)++;
    }

    void sw(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t src = r->get_ireg(d->rs2());
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src);
        (stat->stat_sw)++;
    }

    void addi(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::add(x, y));
        (stat->stat_addi)++;
    }
    void slti(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::slt(x, y));
        (stat->stat_slti)++;
    }
    void sltiu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::sltu(x, y));
        (stat->stat_sltiu)++;
    }
    void xori(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::xor_(x, y));
        (stat->stat_xori)++;
    }
    void ori(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::or_(x, y));
        (stat->stat_ori)++;
    }
    void andi(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::and_(x, y));
        (stat->stat_andi)++;
    }
    void slli(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::sll(x, y));
        (stat->stat_slli)++;
    }
    void srli(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::srl(x, y));
        (stat->stat_srli)++;
    }
    // void srai 
    
    void add(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::add(x, y));
        (stat->stat_add)++;
    }
    void sub(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sub(x, y));
        (stat->stat_sub)++;
    }
    void sll(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sll(x, y));
        (stat->stat_sll)++;
    }
    void slt(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::slt(x, y));
        (stat->stat_slt)++;
    }
    void sltu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sltu(x, y));
        (stat->stat_sltu)++;
    }
    void xor_(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::xor_(x, y));
        (stat->stat_xor)++;
    }
    void srl(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::srl(x, y));
        (stat->stat_srl)++;
    }
    void sra(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sra(x, y));
        (stat->stat_sra)++;
    }
    void or_(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::or_(x, y));
        (stat->stat_or)++;
    }
    void and_(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::and_(x, y));
        (stat->stat_and)++;
    }

    void branch(Decoder *d)
    {
        switch (static_cast<Branch_Inst>(d->funct3()))
        {
        case Branch_Inst::BEQ:
            beq(d);
            break;
        case Branch_Inst::BNE:
            bne(d);
            break;
        case Branch_Inst::BLT:
            blt(d);
            break;
        case Branch_Inst::BGE:
            bge(d);
            break;
        case Branch_Inst::BLTU:
            bltu(d);
            break;
        case Branch_Inst::BGEU:
            bgeu(d);
            break;
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
        }
    }

    void load(Decoder *d)
    {
        switch (static_cast<Load_Inst>(d->funct3()))
        {
        case Load_Inst::LB:
            lb(d);
            break;
        case Load_Inst::LH:
            lh(d);
            break;
        case Load_Inst::LW:
            lw(d);
            break;
        case Load_Inst::LBU:
            lbu(d);
            break;
        case Load_Inst::LHU:
            lhu(d);
            break;
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
            r->ip += 4;
            break;
        }
    }

    void store(Decoder *d)
    {
        switch (static_cast<Store_Inst>(d->funct3()))
        {
        case Store_Inst::SB:
            sb(d);
            break;
        case Store_Inst::SH:
            sh(d);
            break;
        case Store_Inst::SW:
            sw(d);
            break;
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
            r->ip += 4;
            break;
        }
    }

    void alui(Decoder *d)
    {
        switch (static_cast<ALUI_Inst>(d->funct3()))
        {
        case ALUI_Inst::ADDI:
            addi(d);
            break;
        case ALUI_Inst::SLTI:
            slti(d);
            break;
        case ALUI_Inst::SLTIU:
            sltiu(d);
            break;
        case ALUI_Inst::XORI:
            xori(d);
            break;
        case ALUI_Inst::ORI:
            ori(d);
            break;
        case ALUI_Inst::ANDI:
            andi(d);
            break;
        case ALUI_Inst::SLLI:
            slli(d);
            break;
        case ALUI_Inst::SRLI:
            srli(d);
            break;
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
        }
    }

    void add_sub(Decoder *d)
    {
        switch (static_cast<ALU_ADD_SUB_Inst>(d->funct7()))
        {
        case ALU_ADD_SUB_Inst::ADD:
            add(d);
            break;
        case ALU_ADD_SUB_Inst::SUB:
            sub(d);
            break;
        default:
            error_dump("対応していないfunct7が使用されました: %x\n", d->funct7());
        }
    }

    void sr(Decoder *d)
    {
        switch (static_cast<ALU_SR_Inst>(d->funct7()))
        {
        case ALU_SR_Inst::SRA:
            sra(d);
            break;
        case ALU_SR_Inst::SRL:
            srl(d);
            break;
        default:
            error_dump("対応していないfunct7が使用されました: %x\n", d->funct7());
        }
    }

    void alu(Decoder *d)
    {
        switch (static_cast<ALU_Inst>(d->funct3()))
        {
        case ALU_Inst::ADD_SUB:
            add_sub(d);
            break;
        case ALU_Inst::SLL:
            sll(d);
            break;
        case ALU_Inst::SLT:
            slt(d);
            break;
        case ALU_Inst::SLTU:
            sltu(d);
            break;
        case ALU_Inst::XOR:
            xor_(d);
            break;
        case ALU_Inst::SR:
            sr(d);
            break;
        case ALU_Inst::OR:
            or_(d);
            break;
        case ALU_Inst::AND:
            and_(d);
            break;
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
        }
    }

    void flw(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_4(addr);
        r->set_freg_raw(d->rd(), val);
        (stat->stat_flw)++;
    }

    void fsw(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t src = r->get_freg_raw(d->rs2());
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src);
        (stat->stat_fsw)++;
    }

    void fadd(Decoder *d)
    {
        if(d->rm() != 0){
          error_dump("丸め型がおかしいです\n");
        }
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), FPU::fadd(x, y));
        (stat->stat_fadd)++;
    }
    void fsub(Decoder *d)
    {
        if(d->rm() != 0){
          error_dump("丸め型がおかしいです\n");
        }
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), FPU::fsub(x, y));
        (stat->stat_fsub)++;
    }
    void fmul(Decoder *d)
    {
        if(d->rm() != 0){
          error_dump("丸め型がおかしいです\n");
        }
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), FPU::fmul(x, y));
        (stat->stat_fmul)++;
    }
    void fdiv(Decoder *d)
    {
        if(d->rm() != 0){
          error_dump("丸め型がおかしいです\n");
        }
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), FPU::fdiv(x, y));
        (stat->stat_fdiv)++;
    }
    void fsqrt(Decoder *d)
    {
        if(d->rm() != 0){
          error_dump("丸め型がおかしいです\n");
        }
        if(d->rs2() != 0){
          error_dump("命令フォーマットがおかしいです(fsqrtではrs2()は0になる)\n");
        }
        float x = r->get_freg(d->rs1());
        r->set_freg(d->rd(), FPU::fsqrt(x));
        (stat->stat_fsqrt)++;
    }

    void _fsgnj(Decoder *d) 
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), x * y > 0 ? x : -x);
        (stat->stat_fsgnj)++;
    }
    void fsgnjn(Decoder *d) 
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), x * y > 0 ? -x : x);
        (stat->stat_fsgnjn)++;
    }

    void fcvt_w_s(Decoder *d)
    {
        if(d->rm() != 0){
          error_dump("丸め型がおかしいです\n");
        }
        if(d->rs2() != 0){
          error_dump("命令フォーマットがおかしいです(fcvt_w_sではrs2()は0になる)\n");
        }
        float x = r->get_freg(d->rs1());
        r->set_ireg(d->rd(), FPU::float2int(x));
        (stat->stat_fcvt_w_s)++;
    }
    void fcvt_s_w(Decoder *d)
    {
        if(d->rm() != 0){
          error_dump("丸め型がおかしいです\n");
        }
        if(d->rs2() != 0){
          error_dump("命令フォーマットがおかしいです(fcvt_w_sではrs2()は0になる)\n");
        }
        uint32_t x = r->get_ireg(d->rs1());
        r->set_freg(d->rd(), FPU::int2float(x));
        (stat->stat_fcvt_s_w)++;
    }

    void feq(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_ireg(d->rd(),FPU::feq(x,y));
        (stat->stat_feq)++;
    }
    void flt(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_ireg(d->rd(),FPU::flt(x,y));
        (stat->stat_flt)++;
    }
    void fle(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_ireg(d->rd(),FPU::fle(x,y));
        (stat->stat_fle)++;
    }

    void fload(Decoder *d)
    {
        switch (static_cast<FLoad_Inst>(d->funct3()))
        {
        case FLoad_Inst::FLW:
            flw(d);
            break;
        default:
            error_dump("widthがおかしいです(仕様書p112): %x\n", d->funct3());
            r->ip += 4;
            break;
        }
    }

    void fstore(Decoder *d)
    {
        switch (static_cast<FStore_Inst>(d->funct3()))
        {
        case FStore_Inst::FSW:
            fsw(d);
            break;
        default:
            error_dump("widthがおかしいです(仕様書p112): %x\n", d->funct3());
            r->ip += 4;
            break;
        }
    }

    void fsgnj(Decoder *d) 
    {
        switch (static_cast<FSgnj_Inst>(d->funct3())) 
        {
        case FSgnj_Inst::FSGNJ:
            _fsgnj(d);
            break;
        case FSgnj_Inst::FSGNJN:
            fsgnjn(d);
            break;
        case FSgnj_Inst::FSGNJX:
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
        }
    }

    void fcomp(Decoder *d)
    {
        switch (static_cast<FComp_Inst>(d->funct3()))
        {
        case FComp_Inst::FEQ:
            feq(d);
            break;
        case FComp_Inst::FLT:
            flt(d);
            break;
        case FComp_Inst::FLE:
            fle(d);
            break;
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
        }
    }

    void fpu(Decoder *d)
    {
        switch (static_cast<FPU_Inst>(d->funct5_fmt()))
        {
        case FPU_Inst::FADD:
            fadd(d);
            break;
        case FPU_Inst::FSUB:
            fsub(d);
            break;
        case FPU_Inst::FMUL:
            fmul(d);
            break;
        case FPU_Inst::FDIV:
            fdiv(d);
            break;
        case FPU_Inst::FSQRT:
            fsqrt(d);
            break;
        case FPU_Inst::FCOMP:
            fcomp(d);
            break;
        case FPU_Inst::FCVT_W_S:
            fcvt_w_s(d);
            break;
        case FPU_Inst::FCVT_S_W:
            fcvt_s_w(d);
            break;
        case FPU_Inst::FSGNJ:
            fsgnj(d);
            break;
        default:
            error_dump("対応していないfunct3が使用されました: %x\n", d->funct3());
        }
    }

    void run(Decoder *d)
    {
        switch (static_cast<Inst>(d->opcode()))
        {
        case Inst::LUI:
            lui(d);
            r->ip += 4;
            break;
        case Inst::AUIPC:
            auipc(d);
            r->ip += 4;
            break;
        case Inst::JAL:
            jal(d);
            break;
        case Inst::JALR:
            jalr(d);
            break;
        case Inst::BRANCH:
            branch(d);
            break;
        case Inst::LOAD:
            load(d);
            r->ip += 4;
            break;
        case Inst::STORE:
            store(d);
            r->ip += 4;
            break;
        case Inst::ALUI:
            alui(d);
            r->ip += 4;
            break;
        case Inst::ALU:
            alu(d);
            r->ip += 4;
            break;
        case Inst::FLOAD:
            fload(d);
            r->ip += 4;
            break;
        case Inst::FSTORE:
            fstore(d);
            r->ip += 4;
            break;
        case Inst::FPU:
            fpu(d);
            r->ip += 4;
            break;
        default:
            error_dump("対応していないopcodeが使用されました: %x\n", d->opcode());
            r->ip += 4;
            break;
        }
    }

  public:
    Core(std::string filename, Settings *settings)
    {
        r = new Register;
        io = new IO;
        m = new Memory(io);
        stat = new Stat;

        this->settings = settings;

        char buf[512];
        std::ifstream ifs(filename);
        uint32_t addr = instruction_load_address;
        while (!ifs.eof())
        {
            ifs.read(buf, 512);
            int read_bytes = ifs.gcount();
            m->mmap(addr, (uint8_t *)buf, read_bytes);
            addr += read_bytes;
        }
    }
    ~Core()
    {
        delete r;
        delete m;
        delete io;
    }
    void show_stack_from_top()
    {
        std::cout << "Stack" << std::endl;
        m->show_data(r->get_ireg(default_stack_pointer), default_stack_dump_size);
    }
    void info()
    {
        r->info();
        show_stack_from_top();
        io->show_status();
        stat->show_stats();
    }
    void main_loop()
    {
        while (1)
        {
            uint32_t ip = r->ip;
            Decoder d = Decoder(m->get_inst(ip));
            run(&d);
            if (settings->show_inst_value) {
                printf("instr:%x %x\n", r->ip, d.code);
            }
            if (settings->show_registers) {
                r->info();
            }
            if (settings->show_stack) {
                show_stack_from_top();
            }
            if (settings->show_io) {
                io->show_status();
            }
            if (settings->step_execution) {
                std::string s;
                std::getline(std::cin, s);
            }
        }
    }
};

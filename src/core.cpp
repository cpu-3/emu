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
        return (uint32_t)(a >> (y & 0b11111));
    }
    static uint32_t slt(uint32_t x, uint32_t y)
    {
        int32_t a = (int32_t)x;
        int32_t b = (int32_t)y;
        return a < b;
    }
    static uint32_t sltu(uint32_t x, uint32_t y)
    {
        return x < y;
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
    static uint32_t mul(int32_t x, int32_t y)
    {
        return x * y;
    }
    static uint32_t mulh(int32_t x, int32_t y)
    {
        int64_t z = (int64_t)x * (int64_t)y;
        return (uint64_t)z >> 32;
    }
    static uint32_t mulhu(uint32_t x, uint32_t y)
    {
        uint64_t z = (uint64_t)x * (uint64_t)y;
        return (uint64_t)z >> 32;
    }
    static uint32_t mulhsu(int32_t x, uint32_t y)
    {
        int64_t z = (int64_t)x * (int64_t)y;
        return (uint64_t)z >> 32;
    }
    static uint32_t div(int32_t x, int32_t y)
    {
        return x / y;
    }
    static uint32_t rem(int32_t x, int32_t y)
    {
        return x % y;
    }
    static uint32_t mulu(uint32_t x, uint32_t y)
    {
        return x * y;
    }
    static uint32_t divu(uint32_t x, uint32_t y)
    {
        return x / y;
    }
    static uint32_t remu(uint32_t x, uint32_t y)
    {
        return x % y;
    }
};

typedef enum
{
    User,
    Supervisor,
    Machine
} Mode;

class Core
{
    const uint32_t instruction_load_address = 0;
    const int default_stack_pointer = 2;
    const int default_stack_dump_size = 48;
    Memory *m;
    Register *r;
    IO *io;
    MTIMER *mtimer;
    Stat *stat;
    Disasm *disasm;
    Mode cpu_mode;
    unsigned int long long inst_count;

    Settings *settings;

    Permission mode_perm()
    {
        if (cpu_mode == User)
        {
            return Permission().user_on().read_on();
        }
        return Permission().read_on();
    }

    void lui(Decoder *d)
    {
        uint32_t imm = d->u_type_imm();
        r->set_ireg(d->rd(), imm);
        (stat->lui.stat)++;
        disasm->type = "u";
        disasm->inst_name = "lui";
        disasm->dest = d->rd();
        disasm->imm = d->u_type_imm();
    }
    void auipc(Decoder *d)
    {
        // sign extended
        int32_t imm = d->u_type_imm();
        imm += (int32_t)(r->ip);
        r->set_ireg(d->rd(), imm);
        (stat->auipc.stat)++;
        disasm->type = "u";
        disasm->inst_name = "auipc";
        disasm->dest = d->rd();
        disasm->imm = d->u_type_imm();
    }

    void jal(Decoder *d)
    {
        int32_t imm = d->jal_imm();
        r->set_ireg(d->rd(), r->ip + 4);
        r->ip = (int32_t)r->ip + imm;
        (stat->jal.stat)++;
        disasm->type = "j";
        disasm->inst_name = "jal";
        disasm->dest = d->rd();
        disasm->imm = d->jal_imm();
    }
    void jalr(Decoder *d)
    {
        // sign extended
        int32_t imm = d->i_type_imm();
        int32_t s = r->get_ireg(d->rs1());
        r->set_ireg(d->rd(), r->ip + 4);
        r->ip = s + imm;
        (stat->jalr.stat)++;
        disasm->type = "i";
        disasm->inst_name = "jalr";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
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
        (stat->beq.stat)++;
        disasm->type = "b";
        disasm->inst_name = "beq";
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
        disasm->imm = d->b_type_imm();
    }
    void bne(Decoder *d)
    {
        branch_inner(d, r->get_ireg(d->rs1()) != r->get_ireg(d->rs2()));
        (stat->bne.stat)++;
        disasm->type = "b";
        disasm->inst_name = "bne";
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
        disasm->imm = d->b_type_imm();
    }
    void blt(Decoder *d)
    {
        branch_inner(d, (int32_t)r->get_ireg(d->rs1()) < (int32_t)r->get_ireg(d->rs2()));
        (stat->blt.stat)++;
        disasm->type = "b";
        disasm->inst_name = "blt";
        disasm->src1 = (int32_t)d->rs1();
        disasm->src2 = (int32_t)d->rs2();
        disasm->imm = d->b_type_imm();
    }
    void bge(Decoder *d)
    {
        branch_inner(d, (int64_t)r->get_ireg(d->rs1()) >= (int64_t)r->get_ireg(d->rs2()));
        (stat->bge.stat)++;
        disasm->type = "b";
        disasm->inst_name = "bge";
        disasm->src1 = (int64_t)d->rs1();
        disasm->src2 = (int64_t)d->rs2();
        disasm->imm = d->b_type_imm();
    }
    void bltu(Decoder *d)
    {
        branch_inner(d, r->get_ireg(d->rs1()) < r->get_ireg(d->rs2()));
        (stat->bltu.stat)++;
        disasm->type = "b";
        disasm->inst_name = "bltu";
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
        disasm->imm = d->b_type_imm();
    }
    void bgeu(Decoder *d)
    {
        branch_inner(d, r->get_ireg(d->rs1()) >= r->get_ireg(d->rs2()));
        (stat->bgeu.stat)++;
        disasm->type = "b";
        disasm->inst_name = "bgeu";
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
        disasm->imm = d->b_type_imm();
    }

    void lb(Decoder *d)
    {
        Permission perm = mode_perm();
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        int32_t val = m->read_mem_1(addr, perm);
        val <<= 24;
        val >>= 24;
        r->set_ireg(d->rd(), val);
        (stat->lb.stat)++;
        disasm->type = "i";
        disasm->inst_name = "lb";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void lh(Decoder *d)
    {
        Permission perm = mode_perm();
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        int32_t val = m->read_mem_2(addr, perm);
        val <<= 16;
        val >>= 16;
        r->set_ireg(d->rd(), val);
        (stat->lh.stat)++;
        disasm->type = "i";
        disasm->inst_name = "lh";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void lw(Decoder *d)
    {
        Permission perm = mode_perm();
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_4(addr, perm);
        r->set_ireg(d->rd(), val);
        (stat->lw.stat)++;
        disasm->type = "i";
        disasm->inst_name = "lw";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void lbu(Decoder *d)
    {
        Permission perm = mode_perm();
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t offset = d->i_type_imm();
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_1(addr, perm);
        r->set_ireg(d->rd(), val);
        (stat->lbu.stat)++;
        disasm->type = "i";
        disasm->inst_name = "lbu";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void lhu(Decoder *d)
    {
        Permission perm = mode_perm();
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t offset = d->i_type_imm();
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_2(addr, perm);
        r->set_ireg(d->rd(), val);
        (stat->lhu.stat)++;
        disasm->type = "i";
        disasm->inst_name = "lhu";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }

    void sb(Decoder *d)
    {
        Permission perm = mode_perm().write_on();
        uint32_t base = r->get_ireg(d->rs1());
        uint8_t src = r->get_ireg(d->rs2()) & 0xff;
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src, perm);
        (stat->sb.stat)++;
        disasm->type = "s";
        disasm->inst_name = "sb";
        disasm->src = d->rs2();
        disasm->base = d->rs1();
        disasm->imm = d->s_type_imm();
    }

    void sh(Decoder *d)
    {
        Permission perm = mode_perm().write_on();
        uint32_t base = r->get_ireg(d->rs1());
        uint16_t src = r->get_ireg(d->rs2()) & 0xffff;
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src, perm);
        (stat->sh.stat)++;
        disasm->type = "s";
        disasm->inst_name = "sh";
        disasm->src = d->rs2();
        disasm->base = d->rs1();
        disasm->imm = d->s_type_imm();
    }

    void sw(Decoder *d)
    {
        Permission perm = mode_perm().write_on();
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t src = r->get_ireg(d->rs2());
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src, perm);
        (stat->sw.stat)++;
        disasm->type = "s";
        disasm->inst_name = "sw";
        disasm->src = d->rs2();
        disasm->base = d->rs1();
        disasm->imm = d->s_type_imm();
    }

    void addi(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::add(x, y));
        (stat->addi.stat)++;
        disasm->type = "i";
        disasm->inst_name = "addi";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void slti(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::slt(x, y));
        (stat->slti.stat)++;
        disasm->type = "i";
        disasm->inst_name = "slti";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void sltiu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::sltu(x, y));
        (stat->sltiu.stat)++;
        disasm->type = "i";
        disasm->inst_name = "sltiu";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void xori(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::xor_(x, y));
        (stat->xori.stat)++;
        disasm->type = "i";
        disasm->inst_name = "xori";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void ori(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        y &= 0b111111111111;
        r->set_ireg(d->rd(), ALU::or_(x, y));
        (stat->ori.stat)++;
        disasm->type = "i";
        disasm->inst_name = "ori";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void andi(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm();
        r->set_ireg(d->rd(), ALU::and_(x, y));
        (stat->andi.stat)++;
        disasm->type = "i";
        disasm->inst_name = "andi";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void slli(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm() & 0x1f;
        r->set_ireg(d->rd(), ALU::sll(x, y));
        (stat->slli.stat)++;
        disasm->type = "i";
        disasm->inst_name = "slli";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void srli(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm() & 0x1f;
        r->set_ireg(d->rd(), ALU::srl(x, y));
        (stat->srli.stat)++;
        disasm->type = "i";
        disasm->inst_name = "srli";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }
    void srai(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = d->i_type_imm() & 0x1f;
        r->set_ireg(d->rd(), ALU::sra(x, y));
        //(stat->srai.stat)++;
        disasm->type = "i";
        disasm->inst_name = "srai";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }

    void sri(Decoder *d)
    {
        switch (static_cast<ALUI_SRI_Inst>(d->funct7()))
        {
        case ALUI_SRI_Inst::SRLI:
            srli(d);
            break;
        case ALUI_SRI_Inst::SRAI:
            srai(d);
            break;
        default:
            error_dump("対応していないfunct7が使用されました(sri)");
        }
    }

    void add(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::add(x, y));
        (stat->add.stat)++;
        disasm->type = "r";
        disasm->inst_name = "add";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void sub(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sub(x, y));
        (stat->sub.stat)++;
        disasm->type = "r";
        disasm->inst_name = "sub";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void sll(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sll(x, y));
        (stat->sll.stat)++;
        disasm->type = "r";
        disasm->inst_name = "sll";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void slt(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::slt(x, y));
        (stat->slt.stat)++;
        disasm->type = "r";
        disasm->inst_name = "slt";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void sltu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sltu(x, y));
        (stat->sltu.stat)++;
        disasm->type = "r";
        disasm->inst_name = "sltu";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void xor_(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::xor_(x, y));
        (stat->xor_.stat)++;
        disasm->type = "r";
        disasm->inst_name = "xor";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void srl(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::srl(x, y));
        (stat->srl.stat)++;
        disasm->type = "r";
        disasm->inst_name = "srl";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void sra(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::sra(x, y));
        (stat->sra.stat)++;
        disasm->type = "r";
        disasm->inst_name = "sra";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void or_(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::or_(x, y));
        (stat->or_.stat)++;
        disasm->type = "r";
        disasm->inst_name = "or";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void and_(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::and_(x, y));
        (stat->and_.stat)++;
        disasm->type = "r";
        disasm->inst_name = "and";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }

    void mul(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::mul(x, y));
    }

    void mulh(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::mulh(x, y));
    }

    void mulhu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::mulhu(x, y));
    }

    void mulhsu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::mulhsu(x, y));
    }

    void div(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::div(x, y));
    }

    void rem(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::rem(x, y));
    }

    void divu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::divu(x, y));
    }

    void remu(Decoder *d)
    {
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t y = r->get_ireg(d->rs2());
        r->set_ireg(d->rd(), ALU::remu(x, y));
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
        try
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
        catch (Exception e)
        {
            switch (e.cause)
            {
            case Cause::PageFault:
                scause = 1 << 13; // LOAD PGFAULT
                break;
            case Cause::AccessFault:
                scause = 1 << 5;
                break;
            }
            stval = e.stval;
            trap = true;
        }
    }

    void
    store(Decoder *d)
    {
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        if (addr == 0x50004 || addr == 0x50040)
        {
            //printf("%x\n", addr);
            /*if (inst_count % 2 == 0)
            {
                printf("trap %x %x\n", stvec >> 2, r->ip);
                scause = 1 << 17;
                stval = addr;
                trap = true;
                return;
            }*/
        }
        try
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
        catch (Exception e)
        {
            switch (e.cause)
            {
            case Cause::PageFault:
                scause = 1 << 15; // STORE PGFAULT
                break;
            case Cause::AccessFault:
                scause = 1 << 7;
                break;
            }
            stval = e.stval;
            trap = true;
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
        case ALUI_Inst::SRI:
            sri(d);
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

    void mul_div(Decoder *d)
    {
        switch (static_cast<Mul_Div_Inst>(d->funct3()))
        {
        case Mul_Div_Inst::MUL:
            mul(d);
            break;
        case Mul_Div_Inst::MULH:
            mulh(d);
            break;
        case Mul_Div_Inst::MULHU:
            mulhu(d);
            break;
        case Mul_Div_Inst::MULHSU:
            mulhsu(d);
            break;
        case Mul_Div_Inst::DIV:
            div(d);
            break;
        case Mul_Div_Inst::REM:
            rem(d);
            break;
        case Mul_Div_Inst::DIVU:
            divu(d);
            break;
        case Mul_Div_Inst::REMU:
            remu(d);
            break;
        default:
            error_dump("プログラムのバグです(mul_div)");
        }
    }

    void alu(Decoder *d)
    {
        // mul/div
        if (d->funct7() == 1)
        {
            mul_div(d);
            return;
        }

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
        Permission perm = mode_perm();
        uint32_t base = r->get_ireg(d->rs1());
        int32_t offset = d->i_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        uint32_t val = m->read_mem_4(addr, perm);
        r->set_freg_raw(d->rd(), val);
        (stat->flw.stat)++;
        disasm->type = "fi";
        disasm->inst_name = "flw";
        disasm->dest = d->rd();
        disasm->base = d->rs1();
        disasm->imm = d->i_type_imm();
    }

    void fsw(Decoder *d)
    {
        Permission perm = mode_perm().write_on();
        uint32_t base = r->get_ireg(d->rs1());
        uint32_t src = r->get_freg_raw(d->rs2());
        int32_t offset = d->s_type_imm();
        offset <<= 20;
        offset >>= 20;
        uint32_t addr = base + offset;
        m->write_mem(addr, src, perm);
        (stat->fsw.stat)++;
        disasm->type = "fs";
        disasm->inst_name = "fsw";
        disasm->src = d->rs2();
        disasm->base = d->rs1();
        disasm->imm = d->s_type_imm();
    }

    void fadd(Decoder *d)
    {
        if (d->rm() != 0)
        {
            error_dump("丸め型がおかしいです\n");
        }
        uint32_t x = r->get_freg_raw(d->rs1());
        uint32_t y = r->get_freg_raw(d->rs2());
        r->set_freg_raw(d->rd(), FPU::fadd(x, y));
        (stat->fadd.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "fadd";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void fsub(Decoder *d)
    {
        if (d->rm() != 0)
        {
            error_dump("丸め型がおかしいです\n");
        }
        uint32_t x = r->get_freg_raw(d->rs1());
        uint32_t y = r->get_freg_raw(d->rs2());
        r->set_freg_raw(d->rd(), FPU::fsub(x, y));
        (stat->fsub.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "fsub";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void fmul(Decoder *d)
    {
        if (d->rm() != 0)
        {
            error_dump("丸め型がおかしいです\n");
        }
        uint32_t x = r->get_freg_raw(d->rs1());
        uint32_t y = r->get_freg_raw(d->rs2());
        r->set_freg_raw(d->rd(), FPU::fmul(x, y));
        (stat->fmul.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "fmul";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void fdiv(Decoder *d)
    {
        if (d->rm() != 0)
        {
            error_dump("丸め型がおかしいです\n");
        }
        uint32_t x = r->get_freg_raw(d->rs1());
        uint32_t y = r->get_freg_raw(d->rs2());
        r->set_freg_raw(d->rd(), FPU::fdiv(x, y));
        (stat->fdiv.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "fdiv";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void fsqrt(Decoder *d)
    {
        if (d->rm() != 0)
        {
            error_dump("丸め型がおかしいです\n");
        }
        if (d->rs2() != 0)
        {
            error_dump("命令フォーマットがおかしいです(fsqrtではrs2()は0になる)\n");
        }
        uint32_t x = r->get_freg_raw(d->rs1());
        r->set_freg_raw(d->rd(), FPU::fsqrt(x));
        (stat->fsqrt.stat)++;
        disasm->type = "fR";
        disasm->inst_name = "fsqrt";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
    }

    void _fsgnj(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), x * y > 0 ? x : -x);
        (stat->fsgnj.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "fsgnj";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void fsgnjn(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_freg(d->rd(), x * y > 0 ? -x : x);
        (stat->fsgnjn.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "fsgnjn";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }

    void fcvt_w_s(Decoder *d)
    {
        if (d->rm() != 0)
        {
            error_dump("丸め型がおかしいです\n");
        }
        if (d->rs2() != 0)
        {
            error_dump("命令フォーマットがおかしいです(fcvt_w_sではrs2()は0になる)\n");
        }
        float x = r->get_freg(d->rs1());
        r->set_ireg(d->rd(), FPU::float2int(x));
        (stat->fcvt_w_s.stat)++;
        disasm->type = "fR";
        disasm->inst_name = "fcvt_w_s";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
    }
    void fcvt_s_w(Decoder *d)
    {
        if (d->rm() != 0)
        {
            error_dump("丸め型がおかしいです\n");
        }
        if (d->rs2() != 0)
        {
            error_dump("命令フォーマットがおかしいです(fcvt_w_sではrs2()は0になる)\n");
        }
        uint32_t x = r->get_ireg(d->rs1());
        r->set_freg(d->rd(), FPU::int2float(x));
        (stat->fcvt_s_w.stat)++;
        disasm->type = "fR";
        disasm->inst_name = "fcvt_s_w";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
    }

    void feq(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_ireg(d->rd(), FPU::feq(x, y));
        (stat->feq.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "feq";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void flt(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_ireg(d->rd(), FPU::flt(x, y));
        (stat->flt.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "flt";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }
    void fle(Decoder *d)
    {
        float x = r->get_freg(d->rs1());
        float y = r->get_freg(d->rs2());
        r->set_ireg(d->rd(), FPU::fle(x, y));
        (stat->fle.stat)++;
        disasm->type = "fr";
        disasm->inst_name = "fle";
        disasm->dest = d->rd();
        disasm->src1 = d->rs1();
        disasm->src2 = d->rs2();
    }

    void fload(Decoder *d)
    {
        try
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
        catch (Exception e)
        {
            switch (e.cause)
            {
            case Cause::PageFault:
                scause = 1 << 13; // LOAD PGFAULT
                break;
            case Cause::AccessFault:
                scause = 1 << 5;
                break;
            }
            stval = e.stval;
            trap = true;
        }
    }

    void fstore(Decoder *d)
    {
        try
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
        catch (Exception e)
        {
            switch (e.cause)
            {
            case Cause::PageFault:
                scause = 1 << 15; // STORE PGFAULT
                break;
            case Cause::AccessFault:
                scause = 1 << 7;
                break;
            }
            stval = e.stval;
            trap = true;
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

    uint32_t sscratch;
    uint32_t sepc;
    uint32_t stvec;
    uint32_t sie;
    uint32_t sip;

    void check_supervisor()
    {
        if (cpu_mode == Mode::User)
        {
            error_dump("check supervisor\n");
        }
    }

    void csrrw(Decoder *d)
    {
        check_supervisor();
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t csr;
        switch (static_cast<CSR>(d->i_type_imm()))
        {
        case CSR::SATP:
            csr = m->read_satp();
            m->write_satp(x);
            break;
        case CSR::SEPC:
            csr = sepc;
            sepc = x;
            break;
        case CSR::SSCRATCH:
            csr = sscratch;
            sscratch = x;
            break;
        case CSR::STVEC:
            csr = stvec;
            stvec = x;
            break;
        case CSR::SCAUSE:
            csr = scause;
            scause = x;
            break;
        case CSR::SSTATUS:
            csr = sstatus;
            sstatus = x;
            break;
        case CSR::SIE:
            csr = sie;
            sie = x;
            break;
        case CSR::STVAL:
            csr = stval;
            stval = x;
            break;
        case CSR::SIP:
            csr = sip;
            stval = x;
            break;
        default:
            error_dump("対応していないstatusレジスタ番号です");
        }
        r->set_ireg(d->rd(), csr);
    }

    void csrrs(Decoder *d)
    {
        check_supervisor();
        uint32_t x = r->get_ireg(d->rs1());
        uint32_t csr;
        switch (static_cast<CSR>(d->i_type_imm()))
        {
        case CSR::SATP:
            csr = m->read_satp();
            m->write_satp(x | csr);
            break;
        case CSR::SEPC:
            csr = sepc;
            sepc = x | csr;
            break;
        case CSR::SSCRATCH:
            csr = sscratch;
            sscratch = x | csr;
            break;
        case CSR::STVEC:
            csr = stvec;
            stvec = x | csr;
            break;
        case CSR::SCAUSE:
            csr = scause;
            scause = x | csr;
            break;
        case CSR::SSTATUS:
            csr = sstatus;
            sstatus = x | csr;
            break;
        case CSR::SIE:
            csr = sie;
            sie = x | csr;
            break;
        case CSR::STVAL:
            csr = stval;
            stval = x | csr;
            break;
        case CSR::SIP:
            csr = sip;
            sip = x | csr;
            break;
        default:
            error_dump("対応していないstatusレジスタ番号です");
        }
        r->set_ireg(d->rd(), csr);
    }

    void csrrc(Decoder *d)
    {
        check_supervisor();
        uint32_t x = r->get_ireg(d->rs1());

        uint32_t csr;
        switch (static_cast<CSR>(d->i_type_imm()))
        {
        case CSR::SATP:
            csr = m->read_satp();
            m->write_satp((~x) & csr);
            break;
        case CSR::SEPC:
            csr = sepc;
            sepc = ~x & csr;
            break;
        case CSR::SSCRATCH:
            csr = sscratch;
            sscratch = ~x & csr;
            break;
        case CSR::STVEC:
            csr = stvec;
            stvec = ~x & csr;
            break;
        case CSR::SCAUSE:
            csr = scause;
            scause = ~x & csr;
            break;
        case CSR::SSTATUS:
            csr = sstatus;
            sstatus = ~x & csr;
            break;
        case CSR::SIE:
            csr = sie;
            sie = ~x & csr;
            break;
        case CSR::STVAL:
            csr = stval;
            stval = ~x & csr;
            break;
        case CSR::SIP:
            csr = sip;
            sip = ~x & csr;
            break;
        default:
            error_dump("対応していないstatusレジスタ番号です: %x", d->i_type_imm());
        }
        r->set_ireg(d->rd(), csr);
    }

    void csrrwi(Decoder *d)
    {
        // not implemented
    }

    void csrrsi(Decoder *d)
    {
        // not implemented
    }

    void csrrci(Decoder *d)
    {
        // not implemented
    }

    bool sret_flag;
    bool csr_unprivileged;
    uint32_t sstatus;
    void sret(Decoder *d)
    {
        if (cpu_mode != Mode::User)
        {
            uint32_t sstatus5 = (sstatus >> 5) & 1;
            sstatus = (1 << 5) | (sstatus5 << 1);
        }
        else
        {
            csr_unprivileged = true;
        }
        uint32_t sstatus8 = (sstatus >> 8) & 1;
        cpu_mode = sstatus8 == 1 ? Mode::Supervisor : Mode::User;
        r->ip = sepc;
        sret_flag = true;
    }

    uint32_t scause;
    uint32_t stval;

    bool trap;

    void ecall(Decoder *d)
    {
        r->ip -= 4;
        if (cpu_mode == User)
        {
            scause = 1 << 8;
            stval = 0;
        }
        else if (cpu_mode == Supervisor)
        {
            scause = 1 << 9;
            stval = 0;
        }
        trap = true;
    }

    void priv(Decoder *d)
    {
        switch (static_cast<Priv_Inst>(d->funct7()))
        {
        case Priv_Inst::SRET:
            sret(d);
            break;
        case Priv_Inst::ECALL:
            ecall(d);
            break;
        default:
            error_dump("対応していないPRIV命令です");
        }
    }

    void sys(Decoder *d)
    {
        switch (static_cast<System_Inst>(d->funct3()))
        {
        case System_Inst::CSRRW:
            csrrw(d);
            break;
        case System_Inst::CSRRS:
            csrrs(d);
            break;
        case System_Inst::CSRRC:
            csrrc(d);
            break;
        case System_Inst::CSRRWI:
            csrrwi(d);
            break;
        case System_Inst::CSRRSI:
            csrrsi(d);
            break;
        case System_Inst::CSRRCI:
            csrrci(d);
            break;
        case System_Inst::PRIV:
            priv(d);
            break;
        default:
            // system instrs other than csr
            error_dump("Systemで未対応のものが使われました");
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
            mtimer->incr_time(40);
            load(d);
            if (!trap)
                r->ip += 4;
            break;
        case Inst::STORE:
            mtimer->incr_time(40);
            store(d);
            if (!trap)
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
            mtimer->incr_time(40);
            fload(d);
            if (!trap)
                r->ip += 4;
            break;
        case Inst::FSTORE:
            mtimer->incr_time(40);
            fstore(d);
            if (!trap)
                r->ip += 4;
            break;
        case Inst::FPU:
            fpu(d);
            r->ip += 4;
            break;
        case Inst::SYSTEM:
            sys(d);
            if (sret_flag)
            {
                //printf("out: %x\n", r->ip);
                sret_flag = false;
            }
            else
            {
                r->ip += 4;
            }
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
        mtimer = new MTIMER();
        m = new Memory(io, mtimer);
        stat = new Stat;
        disasm = new Disasm;
        cpu_mode = Mode::Supervisor;
        inst_count = 0;
        sret_flag = false;
        csr_unprivileged = false;
        sscratch = 0;
        sepc = 0;
        stvec = 0;
        scause = 0;
        stval = 0;
        sie = 0;
        sip = 0;

        sstatus = 0;

        trap = false;

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
        delete mtimer;
        delete io;
        delete stat;
        delete disasm;
    }
    void show_stack_from_top()
    {
        std::cout << "Stack" << std::endl;
        m->show_data(r->get_ireg(default_stack_pointer), default_stack_dump_size);
    }
    void info()
    {
        if (!settings->hide_error_dump)
        {
            printf("inst_count: %llx\n", inst_count);
            r->info();
            show_stack_from_top();
            io->show_status();
            stat->show_stats();
        }
    }
    void main_loop()
    {
        int mycount = 0;
        while (1)
        {
            Permission perm = mode_perm().read_on().exec_on();
            uint32_t ip = r->ip;
            uint32_t inst;

            // timer intr
            bool occur_intr = ((sstatus >> 1) & 1) && (((sie & sip) >> 5) & 1);
            if (occur_intr)
            {
                uint32_t sstatus1 = (sstatus >> 1) & 1;
                sstatus = (cpu_mode == Mode::Supervisor ? 1 << 8 : 0) | (sstatus1 << 5);
                cpu_mode = Mode::Supervisor;
                // always Direct Mode
                sepc = r->ip;
                stval = 0;
                scause = (1 << 31) | (1 << 5);
                //printf("intr in %x \n", r->ip);
                r->ip = stvec >> 2;
                continue;
            }
            try
            {
                mtimer->incr_time(40);
                inst = m->get_inst(ip, perm);
            }
            catch (Exception e)
            {
                switch (e.cause)
                {
                case Cause::PageFault:
                    scause = 1 << 12; // INST PGFAULT
                    break;
                case Cause::AccessFault:
                    scause = 1 << 1;
                    break;
                }
                stval = e.stval;
                trap = true;
            }
            Decoder d = Decoder(inst);
            if (!trap)
            {
                run(&d);
            }
            if (trap)
            {
                // always delegate
                uint32_t sstatus1 = (sstatus >> 1) & 1;
                sstatus = (cpu_mode == Mode::Supervisor ? 1 << 8 : 0) | (sstatus1 << 5);
                cpu_mode = Mode::Supervisor;
                // always Direct Mode
                sepc = r->ip;
                //printf("in %x \n", r->ip);
                r->ip = stvec >> 2;
                trap = false;
            }

            // intr check
            if (mtimer->is_timer_intr())
            {
                sip = sip | (1 << 5);
            }

            csr_unprivileged = false;
            inst_count++;
            if (settings->show_inst_value)
            {
                printf("inst_count: %llx\n", inst_count);
                printf("ip: %x\n", ip);
                std::cout << "inst: " << std::bitset<32>(d.code) << std::endl;
                disasm->print_inst(disasm->type);
            }
            if (inst_count < settings->wait)
            {
                continue;
            }
            if (settings->show_registers)
            {
                r->info();
            }
            if (settings->show_stack)
            {
                show_stack_from_top();
            }
            if (settings->show_io)
            {
                io->show_status();
            }
            if (settings->step_execution)
            {
                std::string s;
                std::getline(std::cin, s);
                if (settings->break_point && s == "c")
                {
                    settings->step_execution = false;
                }
            }
            if (settings->break_point)
            {
                if (ip == settings->ip)
                {
                    std::string s;
                    std::getline(std::cin, s);
                    if (s != "c")
                    {
                        settings->step_execution = true;
                    }
                }
            }
        }
    }
};

typedef enum
{
    PageFault,
    AccessFault,
} Cause;

class Exception
{
  public:
    Cause cause;
    uint32_t stval;
    Exception(Cause cause, uint32_t stval)
    {
        this->cause = cause;
        this->stval = stval;
    }
};

class Permission
{
  public:
    bool read;
    bool write;
    bool exec;
    bool user;
    Permission(bool read, bool write, bool exec, bool user)
    {
        this->read = read;
        this->write = write;
        this->exec = exec;
        this->user = user;
    }
    Permission()
    {
        this->read = false;
        this->write = false;
        this->exec = false;
        this->user = false;
    }
    Permission read_on()
    {
        this->read = true;
        return *this;
    }
    Permission write_on()
    {
        this->write = true;
        return *this;
    }
    Permission exec_on()
    {
        this->exec = true;
        return *this;
    }
    Permission user_on()
    {
        this->user = true;
        return *this;
    }
};

class Memory
{
    static const uint32_t memory_size = 1 << 31;

    static const uint32_t uart_rx_addr = 0x80000000;
    static const uint32_t uart_tx_addr = 0x80000004;
    static const uint32_t led_addr = 0x80000008;
    static const uint32_t mtime_addr = 0x80001000;
    static const uint32_t mtimeh_addr = 0x80001004;
    static const uint32_t mtimecmp_addr = 0x80001008;
    static const uint32_t mtimecmph_addr = 0x8000100C;
    static const uint32_t PGSIZE = 1 << 12;
    static const uint32_t PTESIZE = 4;
    static const uint32_t LEVELS = 2;

    uint8_t memory[memory_size];
    IO *io;
    MTIMER *mtimer;
    Permission perm;

    uint32_t satp;

    bool is_addressing_on()
    {
        return (satp >> 31) & 1;
    }

    uint32_t base_table()
    {
        return (satp & 0x3fffff) * PGSIZE;
    }

    uint32_t vpn1(uint32_t addr)
    {
        return addr >> 22;
    }

    uint32_t vpn0(uint32_t addr)
    {
        return (addr >> 12) & 0x3ff;
    }

    uint32_t offset(uint32_t addr)
    {
        return addr & 0xfff;
    }

    uint64_t pte_ppn(uint32_t pte)
    {
        return pte >> 10;
    }

    uint64_t ppn0(uint32_t addr)
    {
        return (addr >> 10) & 0x3ff;
    }

    uint64_t ppn1(uint32_t addr)
    {
        return (addr >> 20);
    }

    // DAGUXWRV
    // 76543210
    bool is_valid(uint32_t addr)
    {
        return addr & 1;
    }

    bool is_read(uint32_t addr)
    {
        return (addr >> 1) & 1;
    }

    bool is_write(uint32_t addr)
    {
        return (addr >> 2) & 1;
    }

    bool is_exec(uint32_t addr)
    {
        return (addr >> 3) & 1;
    }

    bool is_user(uint32_t addr)
    {
        return (addr >> 4) & 1;
    }

    void print_table(uint64_t table)
    {
        printf("table: %08x\n", table);
        uint32_t *m = (uint32_t *)memory;
        for (int i = 0; i < 1024; i++)
        {
            printf("%08x\t", m[table / 4 + i]);
            if (i % 8 == 7)
                printf("\n");
        }
    }

    uint64_t va2pa(uint32_t addr, Permission perm)
    {
        //printf("va2pa: %x\n", addr);
        //printf("%x\n", satp);
        int32_t i = LEVELS - 1;
        uint32_t vpns[2] = {vpn0(addr), vpn1(addr)};

        uint64_t a = base_table();
        //print_table(a);
        uint64_t pte;

        uint32_t *m = (uint32_t *)memory;
        while (i >= 0)
        {
            // printf("i: %x, vpn: %x\n", i, vpns[i]);
            pte = m[(a + vpns[i] * PTESIZE) / 4];
            //printf("%x\n", pte);
            // TODO: PMA/PTE check
            if (!is_valid(pte) || (!is_read(pte) && is_write(pte)))
            {
                printf("va2pa: %x\n", addr);
                printf("pte invalid. i = %d\n", i);
                printf("vpns %d %d\n", vpns[1], vpns[0]);
                print_table(base_table());
                //print_table(a);
                throw Exception(Cause::PageFault, addr);
            }
            if (is_read(pte) || is_exec(pte))
                break;
            i -= 1;
            a = pte_ppn(pte) * PGSIZE;
            // print_table(a);
            //print_table(a);
            if (i < 0)
            {
                printf("i invalid\n");
                printf("va2pa: %x\n", addr);
                printf("vpns %d %d\n", vpns[1], vpns[0]);
                print_table(base_table());
                print_table(a);
                throw Exception(Cause::PageFault, addr);
            }
        }
        //printf("%x\n", pte);
        if ((perm.read && !is_read(pte)) ||
            (perm.write && !is_write(pte)) ||
            (perm.exec && !is_exec(pte)) ||
            (perm.user && !is_user(pte)))
        {
            /*printf("pagefault\n");
            printf("%x\n", pte);
            printf("        r w e u\n");
            printf("need    %d %d %d %d\n", perm.read, perm.write, perm.exec, perm.user);
            printf("current %d %d %d %d\n", is_read(pte), is_write(pte), is_exec(pte), is_user(pte));
            printf("va2pa: %x\n", addr);
            printf("vpns %d %d\n", vpns[1], vpns[0]);
            print_table(base_table());*/
            //print_table(a);
            throw Exception(Cause::PageFault, addr);
        }
        // TODO: check SUM/MXR

        // misaligned superpage
        if (i > 0 && ppn0(pte) != 0)
            throw Exception(Cause::PageFault, addr);

        // TODO: PMA PMP check / access/dirty check

        // physical addr is 34 bit
        uint64_t pa;

        // clear flag bits
        if (i > 0)
        {
            pa = (ppn1(pte) << 22) | (vpn0(addr) << 12) | (offset(addr));
        }
        else
        {
            pa = (ppn1(pte) << 22) | (ppn0(pte) << 12) | (offset(addr));
        }
        //printf("[debug] %x -> %lx\n", addr, pa);
        int x;
        //std::cin >> x;
        return pa;
    }

    void alignment_check(uint32_t addr, uint8_t size)
    {
        if (addr % size != 0)
        {
            //error_dump("メモリアドレスのアラインメントがおかしいです: %x\n", addr);
            throw Exception(Cause::AccessFault, addr);
        }
    }

    bool is_mtimer_addr(uint32_t addr)
    {
        return addr == mtime_addr || addr == mtimeh_addr ||
               addr == mtimecmp_addr || addr == mtimecmph_addr;
    }

    bool hook_io_write(uint32_t addr, uint8_t val)
    {
        if (addr == uart_rx_addr)
        {
            error_dump("uartの読み込みポートに書き込みを試みました");
            return true;
        }
        else if (addr == uart_tx_addr)
        {
            io->transmit_uart(val);
        }
        else if (addr == led_addr)
        {
            io->write_led(val);
        }
        else
        {
            return false;
        }
        return true;
    }

    bool hook_io_read(uint32_t addr, uint8_t *v)
    {
        if (addr == uart_rx_addr)
        {
            *v = io->receive_uart();
        }
        else if (addr == uart_tx_addr)
        {
            error_dump("uartの書き込みポートを読み込もうとしました");
        }
        else if (addr == led_addr)
        {
            error_dump("ledの値を読み取ろうとしました");
        }
        else
        {
            return false;
        }
        return true;
    }

    bool hook_mtimer_write(uint32_t addr, uint32_t val)
    {
        if (addr == mtime_addr)
        {
            mtimer->write_mtimel(val);
        }
        else if (addr == mtimeh_addr)
        {
            mtimer->write_mtimeh(val);
        }
        else if (addr == mtimecmp_addr)
        {
            mtimer->write_mtimecmpl(val);
        }
        else if (addr == mtimecmph_addr)
        {
            mtimer->write_mtimecmph(val);
        }
        else
        {
            return false;
        }
        return true;
    }

    bool hook_mtimer_read(uint32_t addr, uint32_t *v)
    {
        if (addr == mtime_addr)
        {
            *v = mtimer->read_mtimel();
        }
        else if (addr == mtimeh_addr)
        {
            *v = mtimer->read_mtimeh();
        }
        else if (addr == mtimecmp_addr)
        {
            *v = mtimer->read_mtimecmpl();
        }
        else if (addr == mtimecmph_addr)
        {
            *v = mtimer->read_mtimecmph();
        }
        else
        {
            return false;
        }
        return true;
    }

    uint64_t mmu(uint32_t addr, Permission perm)
    {
        if (!is_addressing_on())
        {
            return addr;
        }
        return va2pa(addr, perm);
    }

  public:
    Memory(IO *io, MTIMER *mtimer)
    {
        this->io = io;
        this->mtimer = mtimer;
    }

    void write_mem(uint32_t addr, uint8_t val, Permission perm)
    {
        addr = mmu(addr, perm);
        if (is_mtimer_addr(addr))
        {
            puts("mtimerにはwordアクセスしてください");
            while (1)
            {
            }
        }
        if (!hook_io_write(addr, val))
        {
            alignment_check(addr, 1);
            memory[addr] = val;
        }
    }

    void write_mem(uint32_t addr, uint16_t val, Permission perm)
    {
        addr = mmu(addr, perm);
        if (is_mtimer_addr(addr))
        {
            puts("mtimerにはwordアクセスしてください");
            while (1)
            {
            }
        }
        if (!hook_io_write(addr, val))
        {
            alignment_check(addr, 2);
            uint16_t *m = (uint16_t *)memory;
            m[addr / 2] = val;
        }
    }

    void write_mem(uint32_t addr, uint32_t val, Permission perm)
    {
        addr = mmu(addr, perm);
        if (!hook_io_write(addr, val) && !hook_mtimer_write(addr, val))
        {
            alignment_check(addr, 4);
            uint32_t *m = (uint32_t *)memory;
            m[addr / 4] = val;
        }
    }

    uint8_t read_mem_1(uint32_t addr, Permission perm)
    {
        addr = mmu(addr, perm);
        if (is_mtimer_addr(addr))
        {
            puts("mtimerにはwordアクセスしてください");
            while (1)
            {
            }
        }
        uint8_t v;
        if (hook_io_read(addr, &v))
        {
            return v;
        }
        alignment_check(addr, 1);
        return memory[addr];
    }

    uint16_t read_mem_2(uint32_t addr, Permission perm)
    {
        addr = mmu(addr, perm);
        if (is_mtimer_addr(addr))
        {
            puts("mtimerにはwordアクセスしてください");
            while (1)
            {
            }
        }
        uint8_t v;
        if (hook_io_read(addr, &v))
        {
            return v;
        }
        alignment_check(addr, 2);
        uint16_t *m = (uint16_t *)memory;
        return m[addr / 2];
    }

    uint32_t read_mem_4(uint32_t addr, Permission perm)
    {
        addr = mmu(addr, perm);
        uint8_t v;
        if (hook_io_read(addr, &v))
        {
            return v;
        }
        uint32_t timerv;
        if (hook_mtimer_read(addr, &timerv))
        {
            return timerv;
        }
        alignment_check(addr, 4);
        uint32_t *m = (uint32_t *)memory;
        return m[addr / 4];
    }

    uint32_t get_inst(uint32_t addr, Permission perm)
    {
        addr = mmu(addr, perm);
        alignment_check(addr, 4);
        uint32_t *m = (uint32_t *)memory;
        return m[addr / 4];
    }

    // set instructions to memory
    // inst_memが満杯になって死ぬとかないのかな(wakarazu)
    void mmap(uint32_t addr, uint8_t *data, uint32_t length)
    {
        alignment_check(addr, 4);
        alignment_check(length, 4);
        for (int i = 0; i < length; i++)
        {
            memory[addr + i] = data[i];
        }
    }

    void write_satp(uint32_t val)
    {
        satp = val;
    }

    uint32_t read_satp()
    {
        return satp;
    }

    void show_data(uint32_t addr, uint32_t length)
    {
        int cnt = 0;
        for (uint32_t ad = addr; ad < addr + length; ad += 4)
        {
            if (ad + 4 >= memory_size)
            {
                break;
            }
            uint32_t v = read_mem_4(ad, Permission());
            printf("%08x: %08x\n", ad, v);
        }
        std::cout << std::endl;
    }
};

typedef union {
    uint32_t i;
    float f;
} float_int;

int f2i(float x)
{
    float_int data;
    data.f = x;
    return data.i;
}

float i2f(int x)
{
    float_int data;
    data.i = x;
    return data.f;
}

class Register
{
    static const int ireg_size = 32;
    static const int freg_size = 32;
    uint32_t i_registers[ireg_size] = {0};
    uint32_t f_registers[freg_size] = {0};

    static void check_ireg_name(int name, int write)
    {
        if (name < 0 || name > ireg_size)
        {
            error_dump("レジスタの番号が不正です: %d\n", name);
        }
    }
    static void check_freg_name(int name)
    {
        if (name < 0 || name > freg_size)
        {
            error_dump("レジスタの番号が不正です: %d\n", name);
        }
    }

  public:
    uint32_t ip;
    Register() : ip(0) {}
    void set_ireg(int name, uint32_t val)
    {
        check_ireg_name(name, 1);

        if (name == 0)
        {
            return;
        }
        i_registers[name] = val;
    }
    void set_freg(int name, float val)
    {
        check_freg_name(name);

        f_registers[name] = f2i(val);
    }
    void set_freg_raw(int name, uint32_t val)
    {
        check_freg_name(name);

        f_registers[name] = val;
    }
    uint32_t get_ireg(int name)
    {
        check_ireg_name(name, 0);
        if (name == 0)
        {
            return 0;
        }
        return i_registers[name];
    }
    float get_freg(int name)
    {
        check_freg_name(name);
        return i2f(f_registers[name]);
    }
    uint32_t get_freg_raw(int name)
    {
        check_freg_name(name);
        return f_registers[name];
    }

    void info()
    {
        std::cout << "iRegister: " << std::endl;
        std::cout << std::hex;
        std::cout << "ip: " << ip << std::endl;
        for (int i = 0; i < ireg_size; i++)
        {
            std::cout << std::dec << "x" << i << std::hex << ": " << i_registers[i] << " ";
            if (i % 6 == 5)
            {
                std::cout << std::endl;
            }
        }
        std::cout << std::endl
                  << std::endl;
        std::cout << "fRegister: " << std::endl;
        for (int i = 0; i < freg_size; i++)
        {
            std::cout << std::dec << "f" << i << std::hex << ": " << i2f(f_registers[i]) << " ";
            if (i % 6 == 5)
            {
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
        std::cout << std::dec;
    }
};

typedef enum
{
    PageFault
} Cause;

class Exception
{
  public:
    Cause cause;
    Exception(Cause cause)
    {
        this->cause = cause;
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
};

class Memory
{
    static const uint32_t memory_size = 1 << 31;

    static const uint32_t uart_rx_addr = 0x10000;
    static const uint32_t uart_tx_addr = 0x10004;
    static const uint32_t led_addr = 0x10008;
    static const uint32_t PGSIZE = 1 << 12;
    static const uint32_t PTESIZE = 4;
    static const uint32_t LEVELS = 2;

    uint8_t memory[memory_size];
    IO *io;

    uint32_t satp;

    bool is_addressing_on()
    {
        return (satp >> 31) & 1;
    }

    uint32_t base_table()
    {
        return (satp & 0x7fffff) * PGSIZE;
    }

    uint32_t vpn1(uint32_t addr)
    {
        return addr >> 22;
    }

    uint32_t vpn0(uint32_t addr)
    {
        return (addr >> 12) & 0x7ff;
    }

    uint32_t offset(uint32_t addr)
    {
        return addr & 0xfff;
    }

    uint64_t ppn(uint64_t addr)
    {
        return addr / PGSIZE;
    }

    uint64_t ppn0(uint64_t addr)
    {
        return (addr >> 12) & 0x7ff;
    }

    uint64_t ppn1(uint64_t addr)
    {
        return (addr >> 22);
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

    uint32_t va2pa(uint32_t addr, Permission perm)
    {
        int32_t i = LEVELS - 1;
        uint32_t vpns[2] = {vpn1(addr), vpn0(addr)};

        uint64_t a = base_table();
        uint64_t pte;
        while (i >= 0)
        {
            pte = a + vpns[i] * PTESIZE;
            // TODO: PMA/PTE check
            if (!is_valid(pte) || (is_read(pte) && is_write(pte)))
                throw Exception(Cause::PageFault);
            if (is_read(pte) || is_exec(pte))
                break;
            i -= 1;
            a = ppn(pte) * PGSIZE;
            if (i < 0)
            {
                throw Exception(Cause::PageFault);
            }
        }
        if ((is_read(pte) != perm.read) ||
            (is_write(pte) != perm.write) ||
            (is_exec(pte) != perm.exec) ||
            (is_user(pte) != perm.user))
        {
            throw Exception(Cause::PageFault);
        }
        // TODO: check SUM/MXR

        // misaligned superpage
        if (i > 0 && ppn0(pte) != 0)
            throw Exception(Cause::PageFault);

        // TODO: PMA PMP check / access/dirty check

        // physical addr is 34 bit
        uint64_t pa;
        if (i > 0)
        {
            pa = (ppn1(pte) << 22) | (vpn0(addr) << 12) | (offset(addr));
        }
        else
        {
            pa = (ppn1(pte) << 22) | (ppn0(pte) << 12) | (offset(addr));
        }
        return pa;
    }

    void alignment_check(uint32_t addr, uint8_t size)
    {
        if (addr % size != 0)
        {
            error_dump("メモリアドレスのアラインメントがおかしいです: %x\n", addr);
        }
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

  public:
    Memory(IO *io)
    {
        this->io = io;
    }

    void write_mem(uint32_t addr, uint8_t val)
    {
        if (!hook_io_write(addr, val))
        {
            alignment_check(addr, 1);
            memory[addr] = val;
        }
    }

    void write_mem(uint32_t addr, uint16_t val)
    {
        if (!hook_io_write(addr, val))
        {
            alignment_check(addr, 2);
            uint16_t *m = (uint16_t *)memory;
            m[addr / 2] = val;
        }
    }

    void write_mem(uint32_t addr, uint32_t val)
    {
        if (!hook_io_write(addr, val))
        {
            alignment_check(addr, 4);
            uint32_t *m = (uint32_t *)memory;
            m[addr / 4] = val;
        }
    }

    uint8_t read_mem_1(uint32_t addr)
    {
        uint8_t v;
        if (hook_io_read(addr, &v))
        {
            return v;
        }
        alignment_check(addr, 1);
        return memory[addr];
    }

    uint16_t read_mem_2(uint32_t addr)
    {
        uint8_t v;
        if (hook_io_read(addr, &v))
        {
            return v;
        }
        alignment_check(addr, 2);
        uint16_t *m = (uint16_t *)memory;
        return m[addr / 2];
    }

    uint32_t read_mem_4(uint32_t addr)
    {
        uint8_t v;
        if (hook_io_read(addr, &v))
        {
            return v;
        }
        alignment_check(addr, 4);
        uint32_t *m = (uint32_t *)memory;
        return m[addr / 4];
    }

    uint32_t get_inst(uint32_t addr)
    {
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
            uint32_t v = read_mem_4(ad);
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

class MTIMER
{
    uint64_t mtime;
    uint64_t mtimecmp;

  public:
    MTIMER()
    {
        mtime = 0;
        mtimecmp = 0;
    }

    void incr_time(uint64_t tm)
    {
        mtime += tm;
    }

    bool is_timer_intr()
    {
        return mtime >= mtimecmp;
    }

    void write_mtimel(uint32_t val)
    {
        uint64_t upper = (mtime >> 32);
        upper <<= 32;
        mtime = upper | (uint64_t)val;
    }

    void write_mtimeh(uint32_t val)
    {
        mtime = (mtime & 0xFFFFFFFF) | (((uint64_t)val) << 32);
    }

    uint32_t read_mtimel()
    {
        return (uint32_t)(mtime & 0xFFFFFFFF);
    }

    uint32_t read_mtimeh()
    {
        return (uint32_t)(mtime >> 32);
    }

    void write_mtimecmpl(uint32_t val)
    {
        uint64_t upper = (mtime >> 32);
        upper <<= 32;
        mtimecmp = upper | (uint64_t)val;
    }

    void write_mtimecmph(uint32_t val)
    {
        mtimecmp = (mtimecmp & 0xFFFFFFFF) | (((uint64_t)val) << 32);
    }

    uint32_t read_mtimecmpl()
    {
        return (uint32_t)(mtimecmp & 0xFFFFFFFF);
    }

    uint32_t read_mtimecmph()
    {
        return (uint32_t)(mtimecmp >> 32);
    }
};

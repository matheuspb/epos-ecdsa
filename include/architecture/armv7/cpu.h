// EPOS ARMv7 CPU Mediator Declarations

#ifndef __armv7_h
#define __armv7_h

#include <cpu.h>

__BEGIN_SYS

class ARMv7_M: public CPU_Common
{
public:
    // CPU Native Data Types
    using CPU_Common::Reg8;
    using CPU_Common::Reg16;
    using CPU_Common::Reg32;
    using CPU_Common::Reg64;
    using CPU_Common::Log_Addr;
    using CPU_Common::Phy_Addr;

    static const bool thumb = true;

    // CPU Flags
    typedef Reg32 Flags;
    enum {
        FLAG_THUMB      = 1 << 24,      // Thumb state
        FLAG_Q          = 1 << 27,      // DSP Overflow
        FLAG_V          = 1 << 28,      // Overflow
        FLAG_C          = 1 << 29,      // Carry
        FLAG_Z          = 1 << 30,      // Zero
        FLAG_N          = 1 << 31,      // Negative
        FLAG_DEFAULTS   = FLAG_THUMB
    };

    // Exceptions
    typedef Reg32 Exception_Id;
    enum {                      // Priority
        EXC_RESET       = 1,    // -3 (highest)
        EXC_NMI         = 2,    // -2
        EXC_HARD        = 3,    // -1
        EXC_MPU         = 4,    // programmable
        EXC_BUS         = 5,    // programmable
        EXC_USAGE       = 6,    // programmable
        EXC_SVCALL      = 11,   // programmable
        EXC_DEBUG       = 12,   // programmable
        EXC_PENDSV      = 14,   // programmable
        EXC_SYSTICK     = 15    // programmable
    };

protected:
    ARMv7_M() {};

public:
    static Flags flags() {
        register Reg32 value;
        ASM("mrs %0, xpsr" : "=r"(value) :);
        return value;
    }
    static void flags(const Flags & flags) {
        ASM("msr xpsr, %0" : : "r"(flags) : "cc");
    }

    static void int_enable() { ASM("cpsie i"); }
    static void int_disable() { ASM("cpsid i"); }

    static bool int_disabled() {
        bool disabled;
        ASM("mrs %0, primask" : "=r"(disabled));
        return disabled;
    }

    static void mrs12() { ASM("mrs r12, xpsr" : : : "r12"); }
    static void msr12() { ASM("msr xpsr, r12" : : : "cc"); }
};

class ARMv7_A: public CPU_Common
{
public:
    static const bool thumb = false;

    // CPU Flags
    typedef Reg32 Flags;
    enum {
        FLAG_M          = 0x1f << 0,       // Processor Mode
        FLAG_T          = 1    << 5,       // Thumb state
        FLAG_F          = 1    << 6,       // FIQ disable
        FLAG_I          = 1    << 7,       // IRQ disable
        FLAG_A          = 1    << 8,       // Imprecise Abort disable
        FLAG_E          = 1    << 9,       // Endianess (0 ->> little, 1 -> big)
        FLAG_GE         = 0xf  << 16,      // SIMD Greater than or Equal (4 bits)
        FLAG_J          = 1    << 24,      // Jazelle state
        FLAG_Q          = 1    << 27,      // Underflow and/or DSP saturation
        FLAG_V          = 1    << 28,      // Overflow
        FLAG_C          = 1    << 29,      // Carry
        FLAG_Z          = 1    << 30,      // Zero
        FLAG_N          = 1    << 31,      // Negative

        // FLAG_M values
        FLAG_USER       = 0x10,      // User mode
        FLAG_FIQ        = 0x11,      // FIQ mode
        FLAG_IRQ        = 0x12,      // IRQ mode
        FLAG_SVC        = 0x13,      // SVC mode
        FLAG_ABORT      = 0x17,      // Abort mode
        FLAG_UNDEFINED  = 0x1b,      // Undefined mode
        FLAG_SYSTEM     = 0x1f,      // System mode

        FLAG_DEFAULTS   = FLAG_SVC
    };

    // Exceptions
    typedef Reg32 Exception_Id;
    enum {
        EXC_START                   = 1,
        EXC_UNDEFINED_INSTRUCTION   = 2,
        EXC_SWI                     = 3,
        EXC_PREFETCH_ABORT          = 4,
        EXC_DATA_ABORT              = 5,
        EXC_RESERVED                = 6,
        EXC_IRQ                     = 7,
        EXC_FIQ                     = 8
    };

protected:
    ARMv7_A() {};

public:
    static Flags flags() {
        register Reg32 value;
        ASM("mrs %0, cpsr" : "=r"(value) :);
        return value;
    }
    static void flags(const Flags & flags) {
        ASM("msr cpsr, %0" : : "r"(flags) : "cc");
    }

    static void int_enable() { flags(flags() & ~0xC0); }
    static void int_disable() { flags(flags() | 0xC0); }
    static bool int_disabled() { return flags() & 0xC0; }

    static void mrs12() { ASM("mrs r12, cpsr" : : : "r12"); }
    static void msr12() { ASM("msr cpsr, r12" : : : "cc"); }

    static unsigned int int_id() { return 0; }
};

class CPU: private IF<Traits<Build>::MODEL == Traits<Build>::Zynq, ARMv7_A, ARMv7_M>::Result
{
    friend class Init_System;

private:
    typedef IF<Traits<Build>::MODEL == Traits<Build>::Zynq, ARMv7_A, ARMv7_M>::Result Base;

public:
    // CPU Native Data Types
    using CPU_Common::Reg8;
    using CPU_Common::Reg16;
    using CPU_Common::Reg32;
    using CPU_Common::Reg64;
    using CPU_Common::Log_Addr;
    using CPU_Common::Phy_Addr;

    // CPU Context
    class Context
    {
    public:
        Context(const Log_Addr & entry, const Log_Addr & exit): _flags(FLAG_DEFAULTS), _lr(exit | (thumb ? 1 : 0)), _pc(entry | (thumb ? 1 : 0)) {}
//        _r0(0), _r1(1), _r2(2), _r3(3), _r4(4), _r5(5), _r6(6), _r7(7), _r8(8), _r9(9), _r10(10), _r11(11), _r12(12),

        void save() volatile  __attribute__ ((naked));
        void load() const volatile;

        friend Debug & operator<<(Debug & db, const Context & c) {
            db << hex
               << "{r0="  << c._r0
               << ",r1="  << c._r1
               << ",r2="  << c._r2
               << ",r3="  << c._r3
               << ",r4="  << c._r4
               << ",r5="  << c._r5
               << ",r6="  << c._r6
               << ",r7="  << c._r7
               << ",r8="  << c._r8
               << ",r9="  << c._r9
               << ",r10=" << c._r10
               << ",r11=" << c._r11
               << ",r12=" << c._r12
               << ",sp="  << &c
               << ",lr="  << c._lr
               << ",pc="  << c._pc
               << ",psr=" << c._flags
               << "}" << dec;
            return db;
        }

    public:
        Reg32 _flags;
        Reg32 _r0;
        Reg32 _r1;
        Reg32 _r2;
        Reg32 _r3;
        Reg32 _r4;
        Reg32 _r5;
        Reg32 _r6;
        Reg32 _r7;
        Reg32 _r8;
        Reg32 _r9;
        Reg32 _r10;
        Reg32 _r11;
        Reg32 _r12;
        Reg32 _lr;
        Reg32 _pc;
    };

    // I/O ports
    typedef Reg16 IO_Irq;

    // Interrupt Service Routines
    typedef void (ISR)();

    // Fault Service Routines (exception handlers)
    typedef void (FSR)();

public:
    CPU() {}

    static Hertz clock() { return _cpu_clock; }
    static Hertz bus_clock() { return _bus_clock; }

    using Base::flags;

    using Base::int_enable;
    using Base::int_disable;
    static bool int_enabled() { return !int_disabled(); }
    using Base::int_disabled;

    static void halt() { ASM("wfi"); }

    static void switch_context(Context * volatile * o, Context * volatile n) __attribute__ ((naked));

    static int syscall(void * message);
    static void syscalled();

    static Reg32 sp() {
        Reg32 value;
        ASM("mov %0, sp" : "=r"(value) :);
        return value;
    }
    static void sp(const Reg32 & sp) {
        ASM("mov sp, %0" : : "r"(sp) : "sp");
        ASM("isb");
    }

    static Reg32 fr() {
        Reg32 value;
        ASM("mov %0, r0" : "=r"(value));
        return value;
    }
    static void fr(const Reg32 & fr) {
        ASM("mov r0, %0" : : "r"(fr) : "r0");
    }

    static Log_Addr ip() { // due to RISC pipelining PC is read with a +8 (4 for thumb) offset
        Reg32 value;
        ASM("mov %0, pc" : "=r"(value) :);
        return value;
    }

    static Reg32 pdp() { return 0; }
    static void pdp(const Reg32 & pdp) {}

    template<typename T>
    static T tsl(volatile T & lock) {
        register T old;
        register T one = 1;
        ASM("1: ldrexb  %0, [%1]        \n"
            "   strexb  r3, %2, [%1]    \n"
            "   cmp     r3, #0          \n"
            "   bne     1b              \n" : "=&r"(old) : "r"(&lock), "r"(one) : "r3", "cc");
        return old;
    }

    template<typename T>
    static T finc(volatile T & value) {
        register T old;
        if(sizeof(T) == sizeof(Reg8))
            ASM("1: ldrexb  %0, [%1]        \n"
                "   add     %0, #1          \n"
                "   strexb  r3, %0, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n" : "=&r"(old) : "r"(&value) : "r3", "cc");
        else if(sizeof(T) == sizeof(Reg16))
            ASM("1: ldrexh  %0, [%1]        \n"
                "   add     %0, #1          \n"
                "   strexh  r3, %0, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n" : "=&r"(old) : "r"(&value) : "r3", "cc");
        else
            ASM("1: ldrex   %0, [%1]        \n"
                "   add     %0, #1          \n"
                "   strex   r3, %0, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n" : "=&r"(old) : "r"(&value) : "r3", "cc");
        return old - 1;
    }

    template<typename T>
    static T fdec(volatile T & value) {
        register T old;
        if(sizeof(T) == sizeof(Reg8))
            ASM("1: ldrexb  %0, [%1]        \n"
                "   sub     %0, #1          \n"
                "   strexb  r3, %0, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n" : "=&r"(old) : "r"(&value) : "r3", "cc");
        else if(sizeof(T) == sizeof(Reg16))
            ASM("1: ldrexh  %0, [%1]        \n"
                "   sub     %0, #1          \n"
                "   strexh  r3, %0, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n" : "=&r"(old) : "r"(&value) : "r3", "cc");
        else
            ASM("1: ldrex   %0, [%1]        \n"
                "   sub     %0, #1          \n"
                "   strex   r3, %0, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n" : "=&r"(old) : "r"(&value) : "r3", "cc");
        return old + 1;
    }

    template <typename T>
    static T cas(volatile T & value, T compare, T replacement) {
        register T old;
        if(sizeof(T) == sizeof(Reg8))
            ASM("1: ldrexb  %0, [%1]        \n"
                "   cmp     %0, %2          \n"
                "   bne     2f              \n"
                "   strexb  r3, %3, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n"
                "2:                         \n" : "=&r"(old) : "r"(&value), "r"(compare), "r"(replacement) : "r3", "cc");
        else if(sizeof(T) == sizeof(Reg16))
            ASM("1: ldrexh  %0, [%1]        \n"
                "   cmp     %0, %2          \n"
                "   bne     2f              \n"
                "   strexh  r3, %3, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n"
                "2:                         \n" : "=&r"(old) : "r"(&value), "r"(compare), "r"(replacement) : "r3", "cc");
        else
            ASM("1: ldrex   %0, [%1]        \n"
                "   cmp     %0, %2          \n"
                "   bne     2f              \n"
                "   strex   r3, %3, [%1]    \n"
                "   cmp     r3, #0          \n"
                "   bne     1b              \n"
                "2:                         \n" : "=&r"(old) : "r"(&value), "r"(compare), "r"(replacement) : "r3", "cc");
        return old;
    }

    template<typename ... Tn>
    static Context * init_stack(const Log_Addr & usp, Log_Addr sp, void (* exit)(), int (* entry)(Tn ...), Tn ... an) {
        sp -= sizeof(Context);
        Context * ctx = new(sp) Context(entry, exit);
        init_stack_helper(&ctx->_r0, an ...);
        return ctx;
    }

    template<typename ... Tn>
    static Log_Addr init_user_stack(Log_Addr sp, void (* exit)(), Tn ... an) {
        sp -= sizeof(Context);
        Context * ctx = new(sp) Context(0, exit);
        init_stack_helper(&ctx->_r0, an ...);
        return sp;
    }

    using CPU_Common::htole64;
    using CPU_Common::htole32;
    using CPU_Common::htole16;
    using CPU_Common::letoh64;
    using CPU_Common::letoh32;
    using CPU_Common::letoh16;

    using CPU_Common::htobe64;
    using CPU_Common::htobe32;
    using CPU_Common::htobe16;
    using CPU_Common::betoh64;
    using CPU_Common::betoh32;
    using CPU_Common::betoh16;

    using CPU_Common::htonl;
    using CPU_Common::htons;
    using CPU_Common::ntohl;
    using CPU_Common::ntohs;

private:
    template<typename Head, typename ... Tail>
    static void init_stack_helper(Log_Addr sp, Head head, Tail ... tail) {
        *static_cast<Head *>(sp) = head;
        init_stack_helper(sp + sizeof(Head), tail ...);
    }
    static void init_stack_helper(Log_Addr sp) {}

    static void init();

private:
    static unsigned int _cpu_clock;
    static unsigned int _bus_clock;
};

inline CPU::Reg64 htole64(CPU::Reg64 v) { return CPU::htole64(v); }
inline CPU::Reg32 htole32(CPU::Reg32 v) { return CPU::htole32(v); }
inline CPU::Reg16 htole16(CPU::Reg16 v) { return CPU::htole16(v); }
inline CPU::Reg64 letoh64(CPU::Reg64 v) { return CPU::letoh64(v); }
inline CPU::Reg32 letoh32(CPU::Reg32 v) { return CPU::letoh32(v); }
inline CPU::Reg16 letoh16(CPU::Reg16 v) { return CPU::letoh16(v); }

inline CPU::Reg64 htobe64(CPU::Reg64 v) { return CPU::htobe64(v); }
inline CPU::Reg32 htobe32(CPU::Reg32 v) { return CPU::htobe32(v); }
inline CPU::Reg16 htobe16(CPU::Reg16 v) { return CPU::htobe16(v); }
inline CPU::Reg64 betoh64(CPU::Reg64 v) { return CPU::betoh64(v); }
inline CPU::Reg32 betoh32(CPU::Reg32 v) { return CPU::betoh32(v); }
inline CPU::Reg16 betoh16(CPU::Reg16 v) { return CPU::betoh16(v); }

inline CPU::Reg32 htonl(CPU::Reg32 v) { return CPU::htonl(v); }
inline CPU::Reg16 htons(CPU::Reg16 v) { return CPU::htons(v); }
inline CPU::Reg32 ntohl(CPU::Reg32 v) { return CPU::ntohl(v); }
inline CPU::Reg16 ntohs(CPU::Reg16 v) { return CPU::ntohs(v); }

__END_SYS

#endif

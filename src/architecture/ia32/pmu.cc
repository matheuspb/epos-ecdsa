// EPOS IA32 PMU Mediator Implementation

#include <architecture/ia32/pmu.h>

__BEGIN_SYS

// Class attributes
const CPU::Reg32 Intel_PMU_V1::_events[EVENTS] = {
                         /* CLOCK              */ UNHALTED_CORE_CYCLES,
                         /* DVS_CLOCK          */ UNHALTED_REFERENCE_CYCLES,
                         /* INSTRUCTIONS       */ INSTRUCTIONS_RETIRED,
                         /* BRANCHES           */ BRANCH_INSTRUCTIONS_RETIRED,
                         /* BRANCH_MISSES      */ BRANCH_MISSES_RETIRED,
                         /* L1_HIT             */ 0,
                         /* L2_HIT             */ 0,
                         /* L3_HIT             */ LLC_REFERENCES,
                         /* L1_MISS            */ 0,
                         /* L2_MISS            */ 0,
                         /* L3_MISS            */ LLC_MISSES,
};


Handler * Intel_PMU_V2::_handlers[];

// Class methods
void PMU::int_handler(const Interrupt_Id & i)
{
//
//        //db< Init, Intel_PMU_V1 >(WRN) << "Intel_PMU_V1::PMU_int_handler()\n";
//
//        Reg64 perf_ovf_msr;// pebs_enable_msr, pmcs_enable_msr;
//
//        // Get overflow status register value
//        perf_ovf_msr = global_overflow_status();
//        // kout << read(FIXED) << "    canal:FIXED   cpu:" << "  overflow:" << overflow() << "   thread:" << endl;
//
//        // PMC Handlers
//        if (overflow())
//        {
//           // kout << "OVERFLOW" << endl;
//
//            for (Channel i=0; i<CHANNELS;i++)
//            {
//                if (perf_ovf_msr&(1ULL << i))
//                {
//                    stop(i+FIXED);
//                    clear_overflow(i+FIXED);
//                    start(i+FIXED);
//                }
//            }
//
//            db<Init, Intel_PMU_V1>(INF) << "Intel_PMU_V1::PMC_overflow(" << APIC::id() << ");" << endl;
//        }
//
//        // Overflow Changed flag
//        clear_cond_changed();
//
//        // Clearing the mask bit in the performance counter LVT entry.
//        APIC::enable_perf();
//        // kout << read(FIXED) << "    canal:FIXED   cpu:"  << "  overflow:" << overflow() << "   thread:" << endl;
//
//        for (Reg32 i = 0; i < CHANNELS; i++)
//        {
//            // For each PMC: verify if it's handler is set and an overflow occurred
//            if ((_pmc_handler[i] != 0) && ((perf_ovf_msr&(1ULL << i)) != 0))
//            {
//                // Then, call this handler
//                _pmc_handler[i]();
//            }
//        }
//        // kout << read(FIXED) << "    canal:FIXED   cpu:" << "  overflow:" << overflow() << "   thread:" << endl;

}

__END_SYS

#pragma once
#include "apic_def.h"

#if defined(_MSC_VER) && !defined(__clang__)
#define _APIC_INTRIN_IS_PURE_MSVC
#include <intrin.h>
#endif

namespace apic
{
    namespace intrin
    {
        uint64_t rdmsr(uint32_t msr)
        {
#ifdef _APIC_INTRIN_IS_PURE_MSVC
            return __readmsr(msr);
#else
            parted_uint64_t parted_result = { };

            asm("rdmsr" : "=a"(parted_result.low_part), "=d"(parted_result.high_part) : "c"(msr));

            return parted_result.value;
#endif
        }

        void wrmsr(uint32_t msr, uint64_t value)
        {
#ifdef _APIC_INTRIN_IS_PURE_MSVC
            __writemsr(msr, value);
#else
            parted_uint64_t parted_value = { };

            parted_value.value = value;

            asm("wrmsr" :: "c"(msr), "a"(parted_value.low_part), "d"(parted_value.high_part));
#endif
        }

        void cpuid(int32_t info[4], int32_t leaf)
        {
#ifdef _APIC_INTRIN_IS_PURE_MSVC
            __cpuid(info, leaf);
#else
            asm volatile("cpuid" : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3]) : "a"(leaf));
#endif
        }
    }
}

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
		[[nodiscard]] inline uint64_t rdmsr(const uint32_t msr) noexcept
		{
#ifdef _APIC_INTRIN_IS_PURE_MSVC
			return __readmsr(msr);
#else
			parted_uint64 parted_result = { };

			asm("rdmsr" : "=a"(parted_result.low_part), "=d"(parted_result.high_part) : "c"(msr));

			return parted_result.value;
#endif
		}

		inline void wrmsr(const uint32_t msr, const uint64_t value) noexcept
		{
#ifdef _APIC_INTRIN_IS_PURE_MSVC
			__writemsr(msr, value);
#else
			parted_uint64 parted_value = { };

			parted_value.value = value;

			asm("wrmsr" :: "c"(msr), "a"(parted_value.low_part), "d"(parted_value.high_part));
#endif
		}

		inline void cpuid(int32_t info[4], const int32_t leaf) noexcept
		{
#ifdef _APIC_INTRIN_IS_PURE_MSVC
			__cpuid(info, leaf);
#else
			asm volatile("cpuid" : "=a"(info[0]), "=b"(info[1]), "=c"(info[2]), "=d"(info[3]) : "a"(leaf));
#endif
		}
	}
}

#pragma once
#include "apic_crt.h"

enum class icr_delivery_mode_t : uint32_t
{
	fixed = 0b000,
	lowest_priority = 0b001,
	smi = 0b101,
	nmi = 0b100,
	init = 0b101,
	start_up = 0b110
};

enum class icr_destination_mode_t : uint32_t
{
	physical = 0b0,
	logical = 0b1
};

enum class icr_delivery_status_t : uint32_t
{
	idle = 0b0,
	send_pending = 0b1
};

enum class icr_level_t : uint32_t
{
	de_assert = 0b0,
	assert = 0b1
};

enum class icr_trigger_mode_t : uint32_t
{
	edge = 0b0,
	level = 0b1
};

enum class icr_destination_shorthand_t : uint32_t
{
	no_shorthand = 0b00,
	self = 0b01,
	all_including_self = 0b10,
	all_but_self = 0b11
};

#pragma warning(push)
#pragma warning(disable: 4201)

// Intel SDM Volume 3: 12.6.1 Interrupt Command Register (ICR)
union apic_icr_low_t
{
	uint32_t flags;

	struct
	{
		uint32_t vector : 8;
		icr_delivery_mode_t delivery_mode : 3;
		icr_destination_mode_t destination_mode : 1;
		icr_delivery_status_t delivery_status : 1;
		uint32_t reserved1 : 1;
		icr_level_t level : 1;
		icr_trigger_mode_t trigger_mode : 1;
		uint32_t reserved2 : 2;
		icr_destination_shorthand_t destination_shorthand : 2;
		uint32_t reserved3 : 10;
	};
};

union apic_icr_high_t
{
	uint32_t flags;

	struct
	{
		uint32_t reserved1 : 24;
		uint32_t destination_field : 8;
	} xapic;

	struct
	{
		uint32_t destination_field : 32;
	} x2apic;
};

union apic_full_icr_t
{
	uint64_t flags;

	struct
	{
		apic_icr_low_t low;
		apic_icr_high_t high;
	};
};

// END OF SDM SUBCHAPTER

// Intel SDM Volume 3: 12.4.4 Local APIC Status and Location
union apic_base_t
{
	uint64_t flags;

	struct
	{
		uint64_t reserved1 : 8;
		uint64_t is_boot_strap_processor : 1;
		uint64_t reserved2 : 1;
		uint64_t is_x2apic : 1;
		uint64_t is_apic_globally_enabled : 1; // permanent until reset
		uint64_t apic_pfn : 24; // apply left shift of 12
		uint64_t reserved3 : 28;
	};
};

// END OF SDM SUBCHAPTER

struct cpuid_01_t
{
	uint32_t eax;

	struct
	{
		uint32_t reserved1 : 24;
		uint32_t initial_apic_id : 8;
	} ebx;

	struct
	{
		uint32_t reserved1 : 21;
		uint32_t x2apic_supported : 1;
		uint32_t reserved2 : 10;
	} ecx;

	uint32_t edx;
};

#pragma warning(pop)

class apic_field_t
{
protected:
	const uint16_t xapic_offset;

public:
	constexpr apic_field_t(uint16_t xapic_offset_in) : xapic_offset(xapic_offset_in) {}

	constexpr uint16_t get_xapic() const
	{
		return this->xapic_offset;
	}

	constexpr uint16_t get_x2apic() const
	{
		return 0x800 + (this->xapic_offset / 0x10);
	}
};

namespace apic
{
	constexpr uint32_t apic_base_msr = 0x1B;
	constexpr apic_field_t icr(0x300);
}

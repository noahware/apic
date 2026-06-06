#pragma once
#include <stdint.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201)
#endif

namespace apic
{
	enum class icr_delivery_mode : uint32_t
	{
		fixed = 0b000,
		lowest_priority = 0b001,
		smi = 0b010,
		nmi = 0b100,
		init = 0b101,
		start_up = 0b110
	};

	enum class icr_destination_mode : uint32_t
	{
		physical = 0b0,
		logical = 0b1
	};

	enum class icr_delivery_status : uint32_t
	{
		idle = 0b0,
		send_pending = 0b1
	};

	enum class icr_level : uint32_t
	{
		de_assert = 0b0,
		assert = 0b1
	};

	enum class icr_trigger_mode : uint32_t
	{
		edge = 0b0,
		level = 0b1
	};

	enum class icr_destination_shorthand : uint32_t
	{
		no_shorthand = 0b00,
		self = 0b01,
		all_including_self = 0b10,
		all_but_self = 0b11
	};

	// Intel SDM Volume 3: 12.6.1 Interrupt Command Register (ICR)

	union icr_low
	{
		uint32_t flags;

		struct
		{
			uint32_t vector : 8;
			icr_delivery_mode delivery_mode : 3;
			icr_destination_mode destination_mode : 1;
			icr_delivery_status delivery_status : 1;
			uint32_t reserved1 : 1;
			icr_level level : 1;
			icr_trigger_mode trigger_mode : 1;
			uint32_t reserved2 : 2;
			icr_destination_shorthand destination_shorthand : 2;
			uint32_t reserved3 : 10;
		};
	};

	union icr_high
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

	union icr
	{
		uint64_t flags;

		struct
		{
			icr_low low;
			icr_high high;
		};
	};

	// Intel SDM Volume 3: 12.4.4 Local APIC Status and Location

	union base
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

	struct cpuid_01
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

	class field
	{
	public:
		explicit constexpr field(const uint16_t xapic_offset)
				:	xapic_offset_(xapic_offset) {}

		[[nodiscard]] constexpr uint16_t xapic() const noexcept
		{
			return xapic_offset_;
		}

		[[nodiscard]] constexpr uint16_t x2apic() const noexcept
		{
			return 0x800 + (xapic_offset_ / 0x10);
		}

	protected:
		const uint16_t xapic_offset_;
	};

	constexpr uint32_t base_msr = 0x1B;
	constexpr field icr_reg(0x300);
	constexpr field apic_id_reg(0x20);

	namespace intrin
	{
		union parted_uint64
		{
			struct
			{
				uint32_t low_part;
				uint32_t high_part;
			};

			uint64_t value;
		};
	}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

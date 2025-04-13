#include "apic.h"

#include <intrin.h>

// map_physical_address and unmap_physical_address is up to you to implement
extern void* map_physical_address(uint64_t physical_address);
extern void unmap_physical_address(void* map_base_address);

constexpr uint64_t needed_apic_class_instance_size = sizeof(xapic_t) < sizeof(x2apic_t) ? sizeof(x2apic_t) : sizeof(xapic_t);

#ifdef APIC_COMPILE_TIME_INSTANCE_ALLOCATION
static char apic_class_instance_allocation[needed_apic_class_instance_size] = { };
#else
// allocate_memory is up to you to implement
extern void* allocate_memory(uint64_t size);
#endif

cpuid_01_t perform_cpuid_01()
{
	cpuid_01_t cpuid_01 = { };
	__cpuid(reinterpret_cast<int32_t*>(&cpuid_01), 1);

	return cpuid_01;
}

uint8_t apic_t::enable(uint8_t use_x2apic)
{
	apic_base_t apic_base = read_apic_base();

	if (apic_base.apic_pfn == 0)
	{
		apic_base.apic_pfn = 0xFEE00;
	}

	apic_base.is_apic_globally_enabled = 1;
	apic_base.is_x2apic = use_x2apic;

	__writemsr(apic::apic_base_msr, apic_base.flags);

	return 1;
}

uint8_t apic_t::is_any_enabled(apic_base_t apic_base)
{
	return apic_base.is_apic_globally_enabled;
}

uint8_t apic_t::is_x2apic_enabled(apic_base_t apic_base)
{
	return is_any_enabled(apic_base) && apic_base.is_x2apic;
}

apic_base_t apic_t::read_apic_base()
{
	apic_base_t apic_base = { };

	apic_base.flags = __readmsr(apic::apic_base_msr);

	return apic_base;
}

uint32_t apic_t::current_apic_id()
{
	cpuid_01_t cpuid_01 = perform_cpuid_01();
	
	return cpuid_01.ebx.initial_apic_id;
}

uint8_t apic_t::is_x2apic_supported()
{
	cpuid_01_t cpuid_01 = perform_cpuid_01();

	return cpuid_01.ecx.x2apic_supported == 1;
}

apic_full_icr_t apic_t::make_base_icr(uint32_t vector, icr_delivery_mode_t delivery_mode, icr_destination_mode_t destination_mode)
{
	apic_full_icr_t icr = { };

	icr.low.vector = vector;
	icr.low.delivery_mode = delivery_mode;
	icr.low.destination_mode = destination_mode;
	icr.low.trigger_mode = icr_trigger_mode_t::edge;
	icr.low.level = icr_level_t::assert;

	return icr;
}

void apic_t::send_ipi(uint32_t vector, uint32_t apic_id)
{
	apic_full_icr_t icr = make_base_icr(vector, icr_delivery_mode_t::fixed, icr_destination_mode_t::physical);

	this->set_icr_longhand_destination(icr, apic_id);
	this->write_icr(icr);
}

void apic_t::send_ipi(uint32_t vector, icr_destination_shorthand_t destination_shorthand)
{
	apic_full_icr_t icr = make_base_icr(vector, icr_delivery_mode_t::fixed, icr_destination_mode_t::physical);

	icr.low.destination_shorthand = destination_shorthand;

	this->write_icr(icr);
}

void apic_t::send_nmi(uint32_t apic_id)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::nmi, icr_destination_mode_t::physical);

	this->set_icr_longhand_destination(icr, apic_id);
	this->write_icr(icr);
}

void apic_t::send_nmi(icr_destination_shorthand_t destination_shorthand)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::nmi, icr_destination_mode_t::physical);

	icr.low.destination_shorthand = destination_shorthand;

	this->write_icr(icr);
}

xapic_t::xapic_t()
{
	apic_base_t apic_base = read_apic_base();

	uint64_t apic_physical_address = apic_base.apic_pfn << 12;

	this->mapped_apic_base = static_cast<uint8_t*>(map_physical_address(apic_physical_address));
}

uint32_t xapic_t::do_read(uint16_t offset) const
{
	return *reinterpret_cast<uint32_t*>(this->mapped_apic_base + offset);
}

void xapic_t::do_write(uint16_t offset, uint32_t value) const
{
	*reinterpret_cast<uint32_t*>(this->mapped_apic_base + offset) = value;
}

void xapic_t::write_icr(apic_full_icr_t icr)
{
	constexpr uint16_t xapic_icr = apic::icr.get_xapic();

	do_write(xapic_icr, icr.low.flags);
	do_write(xapic_icr + 0x10, icr.high.flags);
}

void xapic_t::set_icr_longhand_destination(apic_full_icr_t& icr, uint32_t destination)
{
	icr.high.xapic.destination_field = destination;
}

uint64_t x2apic_t::do_read(uint32_t msr)
{
	return __readmsr(msr);
}

void x2apic_t::do_write(uint32_t msr, uint64_t value)
{
	__writemsr(msr, value);
}

void x2apic_t::write_icr(apic_full_icr_t icr)
{
	do_write(apic::icr.get_x2apic(), icr.flags);
}

void x2apic_t::set_icr_longhand_destination(apic_full_icr_t& icr, uint32_t destination)
{
	icr.high.x2apic.destination_field = destination;
}

apic_t* apic_t::create_instance()
{
#ifdef APIC_COMPILE_TIME_INSTANCE_ALLOCATION
	static uint8_t has_used_allocation = 0;

	if (has_used_allocation++ != 0)
	{
		return nullptr;
	}

	void* apic_allocation = &apic_class_instance_allocation;
#else
	void* apic_allocation = allocate_memory(needed_apic_class_size);
#endif

	apic_base_t apic_base = xapic_t::read_apic_base();

	uint8_t is_any_apic_enabled = xapic_t::is_any_enabled(apic_base);

	uint8_t use_x2apic = 0;

	if (is_any_apic_enabled == 1)
	{
		use_x2apic = x2apic_t::is_x2apic_enabled(apic_base);
	}
	else
	{
		use_x2apic = apic_t::is_x2apic_supported();

		apic_t::enable(use_x2apic);
	}

	apic_t* apic = nullptr;

	if (use_x2apic == 1)
	{
		apic = new (apic_allocation) x2apic_t();
	}
	else
	{
		apic = new (apic_allocation) xapic_t();
	}

	return apic;
}


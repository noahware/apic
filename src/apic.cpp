#include "apic.h"
#include "apic_intrin.h"

// map_physical_address and unmap_physical_address is up to you to implement
extern void* map_physical_address(uint64_t physical_address);
extern void unmap_physical_address(void* map_base_address);

constexpr uint64_t needed_apic_class_instance_size = sizeof(xapic_t) < sizeof(x2apic_t) ? sizeof(x2apic_t) : sizeof(xapic_t);

#ifdef APIC_RUNTIME_INSTANCE_ALLOCATION
// allocate_memory and free is up to you to implement
extern void* allocate_memory(uint64_t size);
extern void free_memory(void* p, uint64_t size);
#else
static char apic_class_instance_allocation[needed_apic_class_instance_size] = { };
#endif

cpuid_01_t perform_cpuid_01()
{
	cpuid_01_t cpuid_01;

	apic::intrin::cpuid(reinterpret_cast<int32_t*>(&cpuid_01), 1);

	return cpuid_01;
}

uint8_t apic_t::enable(const uint8_t use_x2apic)
{
	apic_base_t apic_base = read_apic_base();

	if (apic_base.apic_pfn == 0)
	{
		apic_base.apic_pfn = 0xFEE00;
	}

	apic_base.is_apic_globally_enabled = 1;
	apic_base.is_x2apic = use_x2apic;

	apic::intrin::wrmsr(apic::apic_base_msr, apic_base.flags);

	return 1;
}

uint8_t apic_t::is_any_enabled(const apic_base_t apic_base)
{
	return apic_base.is_apic_globally_enabled;
}

uint8_t apic_t::is_x2apic_enabled(const apic_base_t apic_base)
{
	return is_any_enabled(apic_base) == 1 && apic_base.is_x2apic == 1;
}

apic_base_t apic_t::read_apic_base()
{
	apic_base_t base;

	base.flags = apic::intrin::rdmsr(apic::apic_base_msr);

	return base;
}

uint32_t apic_t::current_apic_id()
{
	const cpuid_01_t cpuid_01 = perform_cpuid_01();
	
	return cpuid_01.ebx.initial_apic_id;
}

uint8_t apic_t::is_x2apic_supported()
{
	const cpuid_01_t cpuid_01 = perform_cpuid_01();

	return cpuid_01.ecx.x2apic_supported == 1;
}

apic_full_icr_t apic_t::make_base_icr(const uint32_t vector, const icr_delivery_mode_t delivery_mode, const icr_destination_mode_t destination_mode)
{
	apic_full_icr_t icr = { };

	icr.low.vector = vector;
	icr.low.delivery_mode = delivery_mode;
	icr.low.destination_mode = destination_mode;
	icr.low.trigger_mode = icr_trigger_mode_t::edge;
	icr.low.level = icr_level_t::assert;

	return icr;
}

void apic_t::send_ipi(const uint32_t vector, const uint32_t apic_id, const uint8_t is_lowest_priority)
{
	const icr_delivery_mode_t delivery_mode = is_lowest_priority == 1 ? icr_delivery_mode_t::lowest_priority : icr_delivery_mode_t::fixed;

	apic_full_icr_t icr = make_base_icr(vector, delivery_mode, icr_destination_mode_t::physical);

	set_icr_longhand_destination(icr, apic_id);
	write_icr(icr);
}

void apic_t::send_ipi(const uint32_t vector, const icr_destination_shorthand_t destination_shorthand, const uint8_t is_lowest_priority)
{
	const icr_delivery_mode_t delivery_mode = is_lowest_priority == 1 ? icr_delivery_mode_t::lowest_priority : icr_delivery_mode_t::fixed;

	apic_full_icr_t icr = make_base_icr(vector, delivery_mode, icr_destination_mode_t::physical);

	icr.low.destination_shorthand = destination_shorthand;

	write_icr(icr);
}

void apic_t::send_nmi(const uint32_t apic_id)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::nmi, icr_destination_mode_t::physical);

	set_icr_longhand_destination(icr, apic_id);
	write_icr(icr);
}

void apic_t::send_nmi(const icr_destination_shorthand_t destination_shorthand)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::nmi, icr_destination_mode_t::physical);

	icr.low.destination_shorthand = destination_shorthand;

	write_icr(icr);
}

void apic_t::send_smi(const uint32_t apic_id)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::smi, icr_destination_mode_t::physical);

	set_icr_longhand_destination(icr, apic_id);
	write_icr(icr);
}

void apic_t::send_smi(const icr_destination_shorthand_t destination_shorthand)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::smi, icr_destination_mode_t::physical);

	icr.low.destination_shorthand = destination_shorthand;

	write_icr(icr);
}

void apic_t::send_init_ipi(const uint32_t apic_id)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::init, icr_destination_mode_t::physical);

	set_icr_longhand_destination(icr, apic_id);
	write_icr(icr);
}

void apic_t::send_init_ipi(const icr_destination_shorthand_t destination_shorthand)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::init, icr_destination_mode_t::physical);

	icr.low.destination_shorthand = destination_shorthand;

	write_icr(icr);
}

void apic_t::send_startup_ipi(const uint32_t apic_id)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::start_up, icr_destination_mode_t::physical);

	set_icr_longhand_destination(icr, apic_id);
	write_icr(icr);
}

void apic_t::send_startup_ipi(const icr_destination_shorthand_t destination_shorthand)
{
	apic_full_icr_t icr = make_base_icr(0, icr_delivery_mode_t::start_up, icr_destination_mode_t::physical);

	icr.low.destination_shorthand = destination_shorthand;

	write_icr(icr);
}

void* apic_t::operator new(const uint64_t size, void* const p)
{
	(void)size;

	return p;
}

void apic_t::operator delete(void* const p, const uint64_t size)
{
#ifdef APIC_RUNTIME_INSTANCE_ALLOCATION
	free_memory(p, size);
#else
	(void)p;
	(void)size;
#endif
}

xapic_t::xapic_t()
{
	const apic_base_t apic_base = read_apic_base();

	if (apic_base.flags != 0)
	{
		const uint64_t apic_physical_address = apic_base.apic_pfn << 12;

		mapped_base_ = static_cast<uint8_t*>(map_physical_address(apic_physical_address));
	}
}

xapic_t::~xapic_t()
{
	if (mapped_base_ != nullptr)
	{
		unmap_physical_address(mapped_base_);
	}
}

uint32_t xapic_t::do_read(const uint16_t offset) const
{
	if (mapped_base_ == nullptr)
	{
		return 0;
	}

	return *reinterpret_cast<uint32_t*>(mapped_base_ + offset);
}

void xapic_t::do_write(const uint16_t offset, const uint32_t value) const
{
	if (mapped_base_ != nullptr)
	{
		*reinterpret_cast<uint32_t*>(mapped_base_ + offset) = value;
	}
}

void xapic_t::write_icr(const apic_full_icr_t icr)
{
	constexpr uint16_t xapic_icr = apic::icr.xapic();

	do_write(xapic_icr, icr.low.flags);
	do_write(xapic_icr + 0x10, icr.high.flags);
}

void xapic_t::set_icr_longhand_destination(apic_full_icr_t& icr, const uint32_t destination)
{
	icr.high.xapic.destination_field = destination;
}

uint64_t x2apic_t::do_read(const uint32_t msr)
{
	return apic::intrin::rdmsr(msr);
}

void x2apic_t::do_write(const uint32_t msr, const  uint64_t value)
{
	apic::intrin::wrmsr(msr, value);
}

void x2apic_t::write_icr(const apic_full_icr_t icr)
{
	do_write(apic::icr.x2apic(), icr.flags);
}

void x2apic_t::set_icr_longhand_destination(apic_full_icr_t& icr, const uint32_t destination)
{
	icr.high.x2apic.destination_field = destination;
}

void apic_t::write_icr(const apic_full_icr_t icr)
{
	(void)icr;
}

void apic_t::set_icr_longhand_destination(apic_full_icr_t& icr, const uint32_t destination)
{
	(void)icr;
	(void)destination;
}

apic_t* apic_t::create_instance()
{
#ifdef APIC_RUNTIME_INSTANCE_ALLOCATION
	void* apic_allocation = allocate_memory(needed_apic_class_instance_size);
#else
	static uint8_t has_used_allocation = 0;

	if (has_used_allocation != 0)
	{
		return nullptr;
	}

	has_used_allocation = 1;

	void* const apic_allocation = &apic_class_instance_allocation;
#endif

	const apic_base_t apic_base = read_apic_base();

	const uint8_t is_any_apic_enabled = is_any_enabled(apic_base);

	uint8_t use_x2apic;

	if (is_any_apic_enabled == 1)
	{
		use_x2apic = is_x2apic_enabled(apic_base);
	}
	else
	{
		use_x2apic = is_x2apic_supported();

		enable(use_x2apic);
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


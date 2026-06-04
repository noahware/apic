#include "apic.h"
#include "apic_intrin.h"

extern void* map_physical_address(uint64_t physical_address);
extern void unmap_physical_address(void* map_base_address);

#ifdef APIC_RUNTIME_INSTANCE_ALLOCATION
extern void* allocate_memory(uint64_t size);
extern void free_memory(void* p, uint64_t size);
#endif

namespace apic
{

constexpr uint64_t needed_instance_size = sizeof(xapic) < sizeof(x2apic) ? sizeof(x2apic) : sizeof(xapic);
constexpr uint64_t needed_instance_alignment = alignof(xapic) > alignof(x2apic) ? alignof(xapic) : alignof(x2apic);

#ifndef APIC_RUNTIME_INSTANCE_ALLOCATION
alignas(needed_instance_alignment) static char instance_allocation[needed_instance_size] = { };
#endif

namespace
{
	cpuid_01 perform_cpuid_01() noexcept
	{
		cpuid_01 result;

		intrin::cpuid(reinterpret_cast<int32_t*>(&result), 1);

		return result;
	}
}

bool controller::enable(const bool use_x2apic)
{
	base apic_base = read_apic_base();

	if (apic_base.apic_pfn == 0)
	{
		apic_base.apic_pfn = 0xFEE00;
	}

	apic_base.is_apic_globally_enabled = 1;
	apic_base.is_x2apic = use_x2apic;

	intrin::wrmsr(base_msr, apic_base.flags);

	return true;
}

bool controller::is_any_enabled(const base apic_base) noexcept
{
	return apic_base.is_apic_globally_enabled;
}

bool controller::is_x2apic_enabled(const base apic_base) noexcept
{
	return is_any_enabled(apic_base) && apic_base.is_x2apic;
}

base controller::read_apic_base() noexcept
{
	base apic_base;

	apic_base.flags = intrin::rdmsr(base_msr);

	return apic_base;
}

uint32_t controller::current_apic_id() noexcept
{
	const cpuid_01 cpuid = perform_cpuid_01();

	return cpuid.ebx.initial_apic_id;
}

bool controller::is_x2apic_supported() noexcept
{
	const cpuid_01 cpuid = perform_cpuid_01();

	return cpuid.ecx.x2apic_supported;
}

icr controller::make_base_icr(const uint32_t vector, const icr_delivery_mode delivery_mode, const icr_destination_mode destination_mode) noexcept
{
	icr command = { };

	command.low.vector = vector;
	command.low.delivery_mode = delivery_mode;
	command.low.destination_mode = destination_mode;
	command.low.trigger_mode = icr_trigger_mode::edge;
	command.low.level = icr_level::assert;

	return command;
}

void controller::send_ipi(const uint32_t vector, const uint32_t apic_id, const bool is_lowest_priority)
{
	const icr_delivery_mode delivery_mode = is_lowest_priority ? icr_delivery_mode::lowest_priority : icr_delivery_mode::fixed;

	icr command = make_base_icr(vector, delivery_mode, icr_destination_mode::physical);

	set_icr_longhand_destination(command, apic_id);
	write_icr(command);
}

void controller::send_ipi(const uint32_t vector, const icr_destination_shorthand destination_shorthand, const bool is_lowest_priority)
{
	const icr_delivery_mode delivery_mode = is_lowest_priority ? icr_delivery_mode::lowest_priority : icr_delivery_mode::fixed;

	icr command = make_base_icr(vector, delivery_mode, icr_destination_mode::physical);

	command.low.destination_shorthand = destination_shorthand;

	write_icr(command);
}

void controller::send_nmi(const uint32_t apic_id)
{
	icr command = make_base_icr(0, icr_delivery_mode::nmi, icr_destination_mode::physical);

	set_icr_longhand_destination(command, apic_id);
	write_icr(command);
}

void controller::send_nmi(const icr_destination_shorthand destination_shorthand)
{
	icr command = make_base_icr(0, icr_delivery_mode::nmi, icr_destination_mode::physical);

	command.low.destination_shorthand = destination_shorthand;

	write_icr(command);
}

void controller::send_smi(const uint32_t apic_id)
{
	icr command = make_base_icr(0, icr_delivery_mode::smi, icr_destination_mode::physical);

	set_icr_longhand_destination(command, apic_id);
	write_icr(command);
}

void controller::send_smi(const icr_destination_shorthand destination_shorthand)
{
	icr command = make_base_icr(0, icr_delivery_mode::smi, icr_destination_mode::physical);

	command.low.destination_shorthand = destination_shorthand;

	write_icr(command);
}

void controller::send_init_ipi(const uint32_t apic_id)
{
	icr command = make_base_icr(0, icr_delivery_mode::init, icr_destination_mode::physical);

	set_icr_longhand_destination(command, apic_id);
	write_icr(command);
}

void controller::send_init_ipi(const icr_destination_shorthand destination_shorthand)
{
	icr command = make_base_icr(0, icr_delivery_mode::init, icr_destination_mode::physical);

	command.low.destination_shorthand = destination_shorthand;

	write_icr(command);
}

void controller::send_startup_ipi(const uint8_t vector, const uint32_t apic_id)
{
	icr command = make_base_icr(vector, icr_delivery_mode::start_up, icr_destination_mode::physical);

	set_icr_longhand_destination(command, apic_id);
	write_icr(command);
}

void controller::send_startup_ipi(const uint8_t vector, const icr_destination_shorthand destination_shorthand)
{
	icr command = make_base_icr(vector, icr_delivery_mode::start_up, icr_destination_mode::physical);

	command.low.destination_shorthand = destination_shorthand;

	write_icr(command);
}

void* controller::operator new(const uint64_t, void* const p)
{
	return p;
}

void controller::operator delete([[maybe_unused]] void* const p, [[maybe_unused]] const uint64_t size)
{
#ifdef APIC_RUNTIME_INSTANCE_ALLOCATION
	::free_memory(p, size);
#endif
}

xapic::xapic()
{
	const base apic_base = read_apic_base();

	if (apic_base.flags != 0)
	{
		const uint64_t apic_physical_address = apic_base.apic_pfn << 12;

		mapped_base_ = static_cast<uint8_t*>(::map_physical_address(apic_physical_address));
	}
}

xapic::~xapic()
{
	if (mapped_base_ != nullptr)
	{
		::unmap_physical_address(mapped_base_);
	}
}

uint32_t xapic::do_read(const uint16_t offset) const noexcept
{
	if (mapped_base_ == nullptr)
	{
		return 0;
	}

	return *reinterpret_cast<uint32_t*>(mapped_base_ + offset);
}

void xapic::do_write(const uint16_t offset, const uint32_t value) const noexcept
{
	if (mapped_base_ != nullptr)
	{
		*reinterpret_cast<uint32_t*>(mapped_base_ + offset) = value;
	}
}

void xapic::write_icr(const icr command) noexcept
{
	constexpr uint16_t xapic_icr = icr_reg.xapic();

	do_write(xapic_icr + 0x10, command.high.flags);
	do_write(xapic_icr, command.low.flags);
}

void xapic::set_icr_longhand_destination(icr& command, const uint32_t destination) noexcept
{
	command.high.xapic.destination_field = destination;
}

uint64_t x2apic::do_read(const uint32_t msr) noexcept
{
	return intrin::rdmsr(msr);
}

void x2apic::do_write(const uint32_t msr, const uint64_t value) noexcept
{
	intrin::wrmsr(msr, value);
}

void x2apic::write_icr(const icr command) noexcept
{
	do_write(icr_reg.x2apic(), command.flags);
}

void x2apic::set_icr_longhand_destination(icr& command, const uint32_t destination) noexcept
{
	command.high.x2apic.destination_field = destination;
}

void controller::write_icr(const icr) noexcept
{
}

void controller::set_icr_longhand_destination(icr&, const uint32_t) noexcept
{
}

controller* controller::create_instance()
{
#ifdef APIC_RUNTIME_INSTANCE_ALLOCATION
	void* allocation = ::allocate_memory(needed_instance_size);
#else
	static bool has_used_allocation = false;

	if (has_used_allocation)
	{
		return nullptr;
	}

	has_used_allocation = true;

	void* const allocation = &instance_allocation;
#endif

	const base apic_base = read_apic_base();

	const bool is_any_apic_enabled = is_any_enabled(apic_base);

	bool use_x2apic;

	if (is_any_apic_enabled)
	{
		use_x2apic = is_x2apic_enabled(apic_base);
	}
	else
	{
		use_x2apic = is_x2apic_supported();

		enable(use_x2apic);
	}

	controller* result = nullptr;

	if (use_x2apic)
	{
		result = new (allocation) x2apic();
	}
	else
	{
		result = new (allocation) xapic();
	}

	return result;
}

} // namespace apic

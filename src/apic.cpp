#include "apic.h"
#include "apic_intrin.h"

extern void* apic_lib_map_physical_address(uint64_t physical_address);
extern void apic_lib_unmap_physical_address(void* map_base_address);

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

uint32_t controller::current_apic_id() const noexcept
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

void controller::send_mult_ipis(const uint32_t vector, const uint64_t apic_id_mask, const bool is_lowest_priority)
{
	const icr_delivery_mode delivery_mode = is_lowest_priority ? icr_delivery_mode::lowest_priority : icr_delivery_mode::fixed;

	const icr command = make_base_icr(vector, delivery_mode, icr_destination_mode::logical);

	write_icr_to_mask(command, apic_id_mask);
}

void controller::send_mult_nmis(const uint64_t apic_id_mask)
{
	const icr command = make_base_icr(0, icr_delivery_mode::nmi, icr_destination_mode::logical);

	write_icr_to_mask(command, apic_id_mask);
}

void controller::send_mult_smis(const uint64_t apic_id_mask)
{
	const icr command = make_base_icr(0, icr_delivery_mode::smi, icr_destination_mode::logical);

	write_icr_to_mask(command, apic_id_mask);
}

void controller::send_mult_init_ipis(const uint64_t apic_id_mask)
{
	const icr command = make_base_icr(0, icr_delivery_mode::init, icr_destination_mode::logical);

	write_icr_to_mask(command, apic_id_mask);
}

void controller::send_mult_startup_ipis(const uint8_t vector, const uint64_t apic_id_mask)
{
	const icr command = make_base_icr(vector, icr_delivery_mode::start_up, icr_destination_mode::logical);

	write_icr_to_mask(command, apic_id_mask);
}

void controller::configure_timer(const uint8_t vector, const timer_mode mode, const timer_divide divide, const bool masked) noexcept
{
	divide_config dcr = { };

	const uint32_t raw = static_cast<uint32_t>(divide);

	dcr.divide_low = raw & 0b11;
	dcr.divide_high = (raw >> 3) & 0b1;

	write_register(divide_config_reg, dcr.flags);

	lvt_timer lvt = { };

	lvt.vector = vector;
	lvt.mode = mode;
	lvt.mask = masked ? 1u : 0u;

	write_register(lvt_timer_reg, lvt.flags);
}

void controller::set_timer_initial_count(const uint32_t count) noexcept
{
	write_register(initial_count_reg, count);
}

void controller::stop_timer() noexcept
{
	lvt_timer lvt = { };

	lvt.flags = read_register(lvt_timer_reg);
	lvt.mask = 1;

	write_register(lvt_timer_reg, lvt.flags);
	write_register(initial_count_reg, 0);
}

uint32_t controller::read_timer_current_count() const noexcept
{
	return read_register(current_count_reg);
}

void controller::configure_lint0(const uint8_t vector, const lvt_delivery_mode mode, const lvt_trigger_mode trigger, const lvt_pin_polarity polarity, const bool masked) noexcept
{
	lvt_lint lvt = { };

	lvt.vector = vector;
	lvt.delivery_mode = mode;
	lvt.trigger_mode = trigger;
	lvt.pin_polarity = polarity;
	lvt.mask = masked ? 1u : 0u;

	write_register(lvt_lint0_reg, lvt.flags);
}

void controller::configure_lint1(const uint8_t vector, const lvt_delivery_mode mode, const lvt_trigger_mode trigger, const lvt_pin_polarity polarity, const bool masked) noexcept
{
	lvt_lint lvt = { };

	lvt.vector = vector;
	lvt.delivery_mode = mode;
	lvt.trigger_mode = trigger;
	lvt.pin_polarity = polarity;
	lvt.mask = masked ? 1u : 0u;

	write_register(lvt_lint1_reg, lvt.flags);
}

void controller::signal_eoi() noexcept
{
	write_register(eoi_reg, 0);
}

void controller::software_enable(const uint8_t spurious_vector) noexcept
{
	uint32_t svr = read_register(svr_reg);
	svr |= (1u << 8);
	svr = (svr & ~0xFFu) | spurious_vector;
	write_register(svr_reg, svr);
}

void* controller::operator new(const uint64_t, void* const p)
{
	return p;
}

void controller::operator delete(void* const p, const uint64_t size)
{
#ifdef APIC_RUNTIME_INSTANCE_ALLOCATION
	::free_memory(p, size);
#else
	(void)p;
	(void)size;
#endif
}

xapic::xapic()
{
	const base apic_base = read_apic_base();

	if (apic_base.flags != 0)
	{
		const uint64_t apic_physical_address = static_cast<uint64_t>(apic_base.apic_pfn) << 12;

		mapped_base_ = static_cast<uint8_t*>(::apic_lib_map_physical_address(apic_physical_address));
	}
}

xapic::~xapic()
{
	if (mapped_base_ != nullptr)
	{
		::apic_lib_unmap_physical_address(mapped_base_);
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

void xapic::write_icr_to_mask(icr command_template, const uint64_t apic_id_mask) noexcept
{
	// xAPIC flat LDR model: 8-bit destination field, mask is matched against each core's 8-bit LDR.
	// Higher bits cannot be addressed by the hardware; caller-supplied bits >= 8 are ignored.
	command_template.high.xapic.destination_field = static_cast<uint32_t>(apic_id_mask & 0xFF);

	write_icr(command_template);
}

uint32_t xapic::current_apic_id() const noexcept
{
	// xAPIC IDs are 8 bits — CPUID.01h:EBX[31:24] is the canonical source.
	const cpuid_01 cpuid = perform_cpuid_01();

	return cpuid.ebx.initial_apic_id;
}

uint32_t xapic::read_register(const field reg) const noexcept
{
	return do_read(reg.xapic());
}

void xapic::write_register(const field reg, const uint32_t value) noexcept
{
	do_write(reg.xapic(), value);
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

void x2apic::write_icr_to_mask(icr command_template, const uint64_t apic_id_mask) noexcept
{
	// x2APIC LDR layout (Intel SDM Vol. 3 10.12.10.2):
	//     LDR = (cluster_id << 16) | (1 << (apic_id & 0xF)),  cluster_id = apic_id >> 4
	// One ICR write per non-empty 16-core cluster covered by apic_id_mask.
	// uint64_t covers APIC IDs 0..63 -> up to 4 writes.

	uint64_t remaining_mask = apic_id_mask;

	for (uint32_t cluster_id = 0; remaining_mask != 0; cluster_id++, remaining_mask >>= 16)
	{
		const uint32_t cluster_bits = static_cast<uint32_t>(remaining_mask & 0xFFFF);

		if (cluster_bits == 0)
		{
			continue;
		}

		command_template.high.x2apic.destination_field = (cluster_id << 16) | cluster_bits;

		write_icr(command_template);
	}
}

uint32_t x2apic::current_apic_id() const noexcept
{
	// In x2APIC mode the APIC ID is the full 32-bit value in IA32_X2APIC_APICID.
	// CPUID.01h's 8-bit field would alias on systems with > 256 logical processors.
	return static_cast<uint32_t>(do_read(apic_id_reg.x2apic()));
}

uint32_t x2apic::read_register(const field reg) const noexcept
{
	return static_cast<uint32_t>(do_read(reg.x2apic()));
}

void x2apic::write_register(const field reg, const uint32_t value) noexcept
{
	do_write(reg.x2apic(), static_cast<uint64_t>(value));
}

void controller::write_icr(const icr) noexcept
{
}

void controller::set_icr_longhand_destination(icr&, const uint32_t) noexcept
{
}

void controller::write_icr_to_mask(icr, const uint64_t) noexcept
{
}

uint32_t controller::read_register(field) const noexcept
{
	return 0;
}

void controller::write_register(field, const uint32_t) noexcept
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

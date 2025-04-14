#pragma once
#include "apic_def.h"

// if you want apic_t::create_instance to use runtime memory allocation for the instance(s) it creates
// then make sure to #define APIC_RUNTIME_INSTANCE_ALLOCATION

class apic_t
{
public:
	apic_t() = default;
	virtual ~apic_t() = default;

	virtual void write_icr(apic_full_icr_t icr) = 0;
	virtual void set_icr_longhand_destination(apic_full_icr_t& icr, uint32_t destination) = 0;

	static apic_t* create_instance();

	static uint8_t enable(uint8_t use_x2apic);
	static uint8_t is_any_enabled(apic_base_t apic_base);
	static uint8_t is_x2apic_enabled(apic_base_t apic_base);

	static uint32_t current_apic_id();
	static uint8_t is_x2apic_supported();

	static apic_base_t read_apic_base();

	static apic_full_icr_t make_base_icr(uint32_t vector, icr_delivery_mode_t delivery_mode, icr_destination_mode_t destination_mode);

	void send_ipi(uint32_t vector, uint32_t apic_id);
	void send_ipi(uint32_t vector, icr_destination_shorthand_t destination_shorthand);

	void send_nmi(uint32_t apic_id);
	void send_nmi(icr_destination_shorthand_t destination_shorthand);

	void* operator new(uint64_t size, void* p);
	void operator delete(void* p, uint64_t size);
};

class xapic_t : public apic_t
{
protected:
	uint8_t* mapped_apic_base = nullptr;

	uint32_t do_read(uint16_t offset) const;
	void do_write(uint16_t offset, uint32_t value) const;

public:
	xapic_t();
	~xapic_t() override;

	void write_icr(apic_full_icr_t icr) override;
	void set_icr_longhand_destination(apic_full_icr_t& icr, uint32_t destination) override;
};

class x2apic_t : public apic_t
{
protected:
	static uint64_t do_read(uint32_t msr);
	static void do_write(uint32_t msr, uint64_t value);

public:
	void write_icr(apic_full_icr_t icr) override;
	void set_icr_longhand_destination(apic_full_icr_t& icr, uint32_t destination) override;
};

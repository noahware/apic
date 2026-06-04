#pragma once
#include "apic_def.h"

namespace apic
{
	class controller
	{
	public:
		controller() = default;

		// non-pure to avoid _purecall CRT dependency
		virtual void write_icr(icr command) noexcept;
		virtual void set_icr_longhand_destination(icr& command, uint32_t destination) noexcept;
		virtual void write_icr_to_mask(icr command_template, uint64_t apic_id_mask) noexcept;

		void send_ipi(uint32_t vector, uint32_t apic_id, bool is_lowest_priority = false);
		void send_ipi(uint32_t vector, icr_destination_shorthand destination_shorthand, bool is_lowest_priority = false);

		void send_nmi(uint32_t apic_id);
		void send_nmi(icr_destination_shorthand destination_shorthand);

		void send_smi(uint32_t apic_id);
		void send_smi(icr_destination_shorthand destination_shorthand);

		void send_init_ipi(uint32_t apic_id);
		void send_init_ipi(icr_destination_shorthand destination_shorthand);

		void send_startup_ipi(uint8_t vector, uint32_t apic_id);
		void send_startup_ipi(uint8_t vector, icr_destination_shorthand destination_shorthand);

		void send_mult_ipis(uint32_t vector, uint64_t apic_id_mask, bool is_lowest_priority = false);
		void send_mult_nmis(uint64_t apic_id_mask);
		void send_mult_smis(uint64_t apic_id_mask);
		void send_mult_init_ipis(uint64_t apic_id_mask);
		void send_mult_startup_ipis(uint8_t vector, uint64_t apic_id_mask);

		void* operator new(uint64_t size, void* p);
		void operator delete(void* p, uint64_t size);

		[[nodiscard]] static controller* create_instance();

		static bool enable(bool use_x2apic);
		[[nodiscard]] static bool is_any_enabled(base apic_base) noexcept;
		[[nodiscard]] static bool is_x2apic_enabled(base apic_base) noexcept;

		[[nodiscard]] virtual uint32_t current_apic_id() const noexcept;
		[[nodiscard]] static bool is_x2apic_supported() noexcept;

		[[nodiscard]] static base read_apic_base() noexcept;

		[[nodiscard]] static icr make_base_icr(uint32_t vector, icr_delivery_mode delivery_mode, icr_destination_mode destination_mode) noexcept;
	};

	class xapic : public controller
	{
	protected:
		uint8_t* mapped_base_ = nullptr;

		[[nodiscard]] uint32_t do_read(uint16_t offset) const noexcept;
		void do_write(uint16_t offset, uint32_t value) const noexcept;

	public:
		xapic();
		~xapic();

		void write_icr(icr command) noexcept override;
		void set_icr_longhand_destination(icr& command, uint32_t destination) noexcept override;
		void write_icr_to_mask(icr command_template, uint64_t apic_id_mask) noexcept override;

		[[nodiscard]] uint32_t current_apic_id() const noexcept override;
	};

	class x2apic : public controller
	{
	protected:
		[[nodiscard]] static uint64_t do_read(uint32_t msr) noexcept;
		static void do_write(uint32_t msr, uint64_t value) noexcept;

	public:
		x2apic() {};

		void write_icr(icr command) noexcept override;
		void set_icr_longhand_destination(icr& command, uint32_t destination) noexcept override;
		void write_icr_to_mask(icr command_template, uint64_t apic_id_mask) noexcept override;

		[[nodiscard]] uint32_t current_apic_id() const noexcept override;
	};
}

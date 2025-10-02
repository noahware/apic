#include <ntifs.h>

#include "..\..\..\src\apic.h"

#define d_log(format, ...) DbgPrintEx(77, 0, format, __VA_ARGS__)

void driver_unload(PDRIVER_OBJECT driver_object)
{
	UNREFERENCED_PARAMETER(driver_object);
}

void* map_physical_address(uint64_t physical_address)
{
	PHYSICAL_ADDRESS address_to_map = { };
	address_to_map.QuadPart = physical_address;

	return MmMapIoSpace(address_to_map, 0x1000, MmNonCached);
}

void unmap_physical_address(void* map_base_address)
{
	return MmUnmapIoSpace(map_base_address, 0x1000);
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);

	driver_object->DriverUnload = driver_unload;

	apic_t* apic = apic_t::create_instance();

	if (apic == nullptr)
	{
		d_log("[apic] unable to create instance of apic class\n");

		return STATUS_MEMORY_NOT_ALLOCATED;
	}

	uint32_t interrupt_vector = 0xE1; // vector points to nt!KiIpiInterrupt

	d_log("[apic] current apic id: %x\n", apic_t::current_apic_id());
	d_log("[apic] sending interrupt of vector 0x%x\n", interrupt_vector);

	apic->send_ipi(interrupt_vector, icr_destination_shorthand_t::all_but_self);

	delete apic;
	apic = nullptr;

	return STATUS_SUCCESS;
}
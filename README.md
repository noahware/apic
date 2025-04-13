# apic
C++ library for sending processor interrupts via x2apic & xapic.

# Usage
The library is OS independent, it doesn't rely on any operating system specific libraries. This means you are easily able to implement this into your own: operating system, hypervisor, kernel driver, etc.

To get an instance of the apic class, call the static routine `apic_t::create_instance()`, but I assume you are wondering, how is it allocating memory for the class? To specify the allocation of the class instance, you can either `#define APIC_COMPILE_TIME_INSTANCE_ALLOCATION` - which will reserve enough memory for an instance of either `xapic_t` or `x2apic_t` at compile time, so it wont rely on any runtime memory allocation.
If you do not define `APIC_COMPILE_TIME_INSTANCE_ALLOCATION`, then the library will expect you to define a routine: `allocate_memory(uint64_t size)` (which will then be invoked to allocate memory for class at runtime if you have opted out of compile time allocation).
The aforementioned routine: `apic_t::create_instance()` checks whether apic has already been enabled, and if it has, then it uses the apic version which is already running. If apic is not already enabled, it will enable it with x2apic if it is supported by the CPU (if x2apic is not supported, then it will enable xapic only).

Once you have an instance, sending interrupts to processors is simple. The library exposes 4 routines for this:

```cpp
void send_ipi(uint32_t vector, uint32_t apic_id);
void send_ipi(uint32_t vector, icr_destination_shorthand_t destination_shorthand);

void send_nmi(uint32_t apic_id);
void send_nmi(icr_destination_shorthand_t destination_shorthand);
```

These routines allow you to either specify a specific logical processors's apic id - you can read the current logical processor's apic id by invoking the static routine `apic_t::current_apic_id()` - or to specify a shorthand identifier.
What are shorthand identifiers, you ask? Well this is the way the Intel SDM has provided to describe these following destinations: `self` (current logical processor), `all_including_self` (all logical processors including the sender), and `all_but_self` (all logical processors except the sender).

Heres an example on how to send a NMI (non maskable interrupt) to all logical processors except the logical processor we are currently executing on:

```cpp
apic_t* apic = apic_t::create_instance();

apic->send_nmi(icr_destination_shorthand_t::all_but_self);
```

Heres another example on how to send an interrupt with vector 0xE1 to the apic id `3`:

```cpp
apic_t* apic = apic_t::create_instance();

uint32_t interrupt_vector = 0xE1;
uint32_t apic_id = 3;

apic->send_ipi(interrupt_vector, apic_id);
```

# Compilable examples
Compilable examples are provided in the [examples](examples) directory, where there is currently an example for a [windows kernel driver](examples/windows/apic-kernel-static).

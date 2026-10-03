# apic
C++ library for sending processor interrupts via x2apic & xapic.

# Usage
The library is OS independent, it doesn't rely on any operating system specific libraries. This means you are easily able to implement this into your own: operating system, hypervisor, kernel driver, etc.

To get an instance of the apic class, call the static routine `apic::controller::create_instance()`, but I assume you are wondering, how is it allocating memory for the class?
To specify the allocation of the class instance, you can either `#define APIC_RUNTIME_INSTANCE_ALLOCATION` - which will make the library expect you to define
two routines: `allocate_memory(uint64_t size)` and `free_memory(void* p, uint64_t size)` (which will then be invoked to allocate and deallocate memory for class instances at runtime if you have opted out of compile time allocation).
If you do not define `APIC_RUNTIME_INSTANCE_ALLOCATION`, then the library will reserve enough memory for an instance of either `apic::xapic` or `apic::x2apic` at compile time, so it won't rely on any runtime memory allocation.
The aforementioned routine: `apic::controller::create_instance()` checks whether apic has already been enabled, and if it has, then it uses the apic version which is already running. 
If apic is not already enabled, it will enable it with x2apic if it is supported by the CPU (if x2apic is not supported, then it will enable xapic only).

Once you have an instance, sending interrupts to processors is simple. The library exposes such routines for this:

```cpp
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
```

These routines allow you to either specify a specific logical processors's apic id - you can read the current logical processor's apic id by invoking `controller::current_apic_id()` on your instance - or to specify a shorthand identifier.
What are shorthand identifiers, you ask? Well this is the way the Intel SDM has provided to describe these following destinations: `self` (current logical processor), `all_including_self` (all logical processors including the sender), and `all_but_self` (all logical processors except the sender).

Heres an example on how to send a NMI (non maskable interrupt) to all logical processors except the logical processor we are currently executing on:

```cpp
apic::controller* lapic = apic::controller::create_instance();

lapic->send_nmi(apic::icr_destination_shorthand::all_but_self);
```

Heres another example on how to send an interrupt with vector `0xE1` to the apic id `3`:

```cpp
apic::controller* lapic = apic::controller::create_instance();

uint32_t interrupt_vector = 0xE1;
uint32_t apic_id = 3;

lapic->send_ipi(interrupt_vector, apic_id);
```

Finally, heres another example on how to send an interrupt with vector `0xE1` to the apic id `3` with the `lowest priority`:

```cpp
apic::controller* lapic = apic::controller::create_instance();

uint32_t interrupt_vector = 0xE1;
uint32_t apic_id = 3;

lapic->send_ipi(interrupt_vector, apic_id, true);
```

Once you are ready to free the apic instance, then just use the delete operator on the object. An example is linked below:

```cpp
apic::controller* lapic = apic::controller::create_instance();

delete lapic;
lapic = nullptr; // optional, but good practice to do so
```

# Compilable examples
Compilable examples are provided in the [examples](examples) directory, where there is currently an two examples of a windows kernel driver. One uses [compile time instance allocation](examples/windows/apic-kernel-static), and the other uses [runtime instance allocation](examples/windows/apic-kernel-dynamic).

# Showcase
Both of the videos below are using the [windows kernel driver example](examples/windows/apic-kernel-static). They show me placing a breakpoint on the Windows kernel's ipi handler (of vector 0xE1) right before sending an ipi with 0xE1 on the current logical processor. I then allow the ipi to be sent by the example, which is then led to a breakpoint being hit on nt!KiIpiInterrupt. I am then able to read the stack pointer, and find that the `interrupted instruction pointer` has just been pushed on the stack (which is within our example driver - showing that the self ipi was sent successfully).

X2apic:

https://github.com/user-attachments/assets/dd30bf92-f3c8-4a28-88ea-6d67782bb591

Xapic:

https://github.com/user-attachments/assets/b10a9f32-f17e-478c-872e-57c82c6415a4

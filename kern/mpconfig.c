#include <inc/types.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/env.h>
#include <kern/cpu.h>
#include <kern/pmap.h>
#include <inc/uefi.h>
#include <inc/acpi.h>

struct CpuInfo cpus[NCPU];
struct CpuInfo *bootcpu;
int ismp;
int ncpu;

// Per-CPU kernel stacks
unsigned char percpu_kstacks[NCPU][KERN_STACK_SIZE]
__attribute__ ((aligned(PAGE_SIZE)));
unsigned char percpu_pfstacks[NCPU][KERN_PF_STACK_SIZE]
__attribute__ ((aligned(PAGE_SIZE)));

static uint8_t
sum(void *addr, int len)
{
    int i, sum;

    sum = 0;
    for (i = 0; i < len; i++)
        sum += ((uint8_t *)addr)[i];
    return sum;
}

static int
check_rsdp(struct acpi_rsdp *rsdp){

    //Check is rsdp signature is equals to ACPI RSDP signature
    if(memcmp(rsdp->signature, ACPI_RSDP_SIG, sizeof(rsdp->signature)) != 0)
        return -1;

    //If yes, calculates rsdp checksum and check It
    uint32_t checksum = sum((void *)rsdp, sizeof(*rsdp));

    if(checksum != 0)
        return -1;

    return 0;
}

static int
check_rsdt(struct acpi_rsdt *rsdt){

    //Check if rsdt signature is equal to ACPI RSDT signature
    if(memcmp(rsdt->header.signature, ACPI_RSDT_SIG,
                sizeof(rsdt->header.signature)) != 0)
        return -1;
    uint32_t checksum = sum((void *)rsdt, rsdt->header.length);

    if(checksum != 0)
        return -1;

    return 0;
}

static int
check_apic(struct acpi_apic *apic){
    if (apic == 0)
        return -1;
    uint32_t checksum = sum((void *)apic, apic->header.length);
    if(checksum != 0)
        return -1;

    return 0;
}

void
mp_init(void)
{
    struct acpi_rsdp* rsdp = (struct acpi_rsdp*) KADDR(uefi_lp->ACPIRoot);
    if(check_rsdp(rsdp))
        return;

    //Get rsdt address from rsdp
    struct acpi_rsdt* rsdt = (struct acpi_rsdt*) KADDR(rsdp->rsdt_addr);
    if(check_rsdt(rsdt))
        return;

    //Calculated number of elements stored in rsdt
    int acpi_rsdt_n = (rsdt->header.length - sizeof(rsdt->header))
        / sizeof(rsdt->entry[0]);
    
    //Search APIC entries in rsdt array
    struct acpi_apic *apic = 0;
    int i;
    struct acpi_dhdr *descr_header;
    for(i = 0; i < acpi_rsdt_n; i++){
        descr_header = (struct acpi_dhdr*) KADDR(rsdt->entry[i]);

        //Check if the entry contains an APIC
        if(memcmp(descr_header->signature, ACPI_APIC_SIG,
                    sizeof(descr_header->signature)) == 0){

            //If yes, store the entry in apic
            apic = (struct acpi_apic*) KADDR(rsdt->entry[i]);
        }
    }
    if(check_apic(apic))
        return;

    bootcpu = &cpus[0];
    ncpu = 0;
    ismp = 1;
    lapicaddr = apic->lapic_addr;
    struct acpi_apic_dhdr *apic_entry = apic->entry;
    uintptr_t end = (uintptr_t) apic + apic->header.length;
    while((uintptr_t)apic_entry < end){
        struct acpi_apic_lapic *lapic_entry;

        switch(apic_entry->type){

            //If APIC entry is a CPU lapic
            case ACPI_APIC_ENTRY_LAPIC:
                //Store lapic
                lapic_entry = (struct acpi_apic_lapic*) apic_entry;
                //If cpu flag is correct, and the maximum number of CPUs is not reached
                if((lapic_entry->flags & 0x1) && ncpu < NCPU){
                    assert(lapic_entry->apic_id == lapic_entry->processor_id);
                    cpus[ncpu].cpu_id = lapic_entry->processor_id;

                    //Increase number of CPU
                    ncpu++;
                }
                break;
        }
        //Get next APIC entry
        apic_entry = (struct acpi_apic_dhdr*)((uintptr_t) apic_entry
                + apic_entry->length);
    }


    bootcpu->cpu_status = CPU_STARTED;
    if (!ismp) {
        // Didn't like what we found; fall back to no MP.
        ncpu = 1;
        lapicaddr = 0;
        cprintf("SMP: configuration not found, SMP disabled\n");
        return;
    }
    cprintf("SMP: CPU %d found %d CPU(s)\n", bootcpu->cpu_id,  ncpu);

    //if (mp->imcrp) {
        // [MP 3.2.6.1] If the hardware implements PIC mode,
        // switch to getting interrupts from the LAPIC.
        // cprintf("SMP: Setting IMCR to switch from PIC mode to symmetric I/O mode\n");
        // outb(0x22, 0x70);   // Select IMCR
        // outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
    //}
}
#ifndef JOS_INC_ACPI_H
#define JOS_INC_ACPI_H

#define ACPI_RSDP_ALIGN 16
#define ACPI_RSDP_SIG "RSD PTR "


struct acpi_rsdp
{
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t revision[1];
    uint32_t rsdt_addr;
} __attribute__((__packed__));


/* RSDT Entry Header 
 * 
 * Header which stores the descriptors of tables pointed from RDSP's Entry Field
 * Includes the signature of the table, to identify each table
 * 
 * In MADT, the signature is 'APIC'
 */ 
struct acpi_dhdr
{
    uint8_t signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[6];
    uint8_t oem_table_id[8];
    uint32_t oem_revision;
    uint8_t creator_id[4];
    uint32_t creator_revision;
} __attribute__((__packed__));


#define ACPI_RSDT_SIG "RSDT"

struct acpi_rsdt
{
    struct acpi_dhdr header;
    uint32_t entry[0];
} __attribute__((__packed__));

//APIC table signature
#define ACPI_APIC_SIG "APIC"

//Types value for Local APIC and I/O APIC ACPI's structures
#define ACPI_APIC_ENTRY_LAPIC  0
#define ACPI_APIC_ENTRY_IOAPIC 1

/* APIC descriptor header 
 * Define the type of the structure (Local APIC, I/O APIC or others)
 * Type: Local APIC (0), I/O APIC (1)
 */
struct acpi_apic_dhdr
{
    uint8_t type;
    uint8_t length;
} __attribute__((__packed__));


/* Multiple APIC Description Table (MADT)
 *
 * Describes the APIC structures which exist in the machine
 * Includes the common address where Local APIC is mapped in main memory
 * 
 * Entry field stores the descriptors of APIC structures
 */
struct acpi_apic
{
    struct acpi_dhdr header; //Header, which stores the descriptor for RDST's Entry field
    uint32_t lapic_addr; //Local Interrupt Controller Address
    uint32_t flags;
    struct acpi_apic_dhdr entry[0]; //Interrupt Controller Structure 
} __attribute__((__packed__));

/* Processor Local APIC Structure
 *
 * Stores information about APIC ID, flags and ACPI Processor UID
 */

struct acpi_apic_lapic
{
    struct acpi_apic_dhdr header;
    uint8_t processor_id; //ACPI Processor UID
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((__packed__));


/* I/O APIC Structure 
 *
 * Stores information about APIC ID, and I/O APIC tables
 */

struct acpi_apic_ioapic
{
    struct acpi_apic_dhdr header;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t addr;
    uint32_t base;
} __attribute__((__packed__));


#endif /* JOS_INC_ACPI_H */
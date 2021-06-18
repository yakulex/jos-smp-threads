.code64
.text

.globl mp64entry
mp64entry:
    cli
    # Set up kernel stack
    movq mpentry_kstack(%rip),%rsp
    xor %ebp, %ebp

    # Invoke C code
    call mp_main
    # Should never reach this place
    jmp .

.data
Notes to be taken into account for the documentation
====================================================

calling procedures
------------------

```
; R.0-15 are clobbered by a call
; R.16-30 are maintained by the callee
; R.31 should be the same or adjusted to account for return values by the callee

RP.30               ; save previous return address
PI :param1          ; push parameters
PI :param2
NO                  ; etc
PI :proc_label      ; address of the proc
CA                  ; call it
PP                  ; purge return address, if applicable
PR.30               ; restore previous return address
RT
```

```
:proc_label
AD                  ; do something with the parameters
RT                  ; return
```

```
; stack switch instruction RW
    PI -40  ; compute alternative stack address
    RP.31   ;
    AD      ;
    PR.30   ; load alternative swap address in R30
    RW      ; swap R30 (NS) and R31 (OriginalStack)
    PR.0    ; save desired value (OS-40)
    RW      ; swap R30 (OS) and R31 (NS) again
```

```
; reserve stack space
    RP.30   ; save return address
    RP.31
    PI  40  ; reserve 40 words
    AD
    PR.30
    RW      ; done!
    ; do stuff...
    RP.31   ; swap stacks again, with -40 this time
    PI  -40
    AD
    PR.30
    RW
    PR.30   ; restore return address
    RT
```

```
; stack frame:
;   caller pushes his precious data
;   caller pushes parameters right to left
;   caller pushes faddr
;   calls CA
;   caller pops return values, if applicable
;   caller pops his precious data

; callee:
;   pushes don't touch registers, if applicable
;   does stuff
;   pushes return values, if applicable, or restores stack
;   pops don't touch registers, if applicable

; tail call :D
    ; ... some end condition
    PI :tailcall
    JZ
    RT      ; ...otherwise
:tailcall
    PI :faddr
    JP      ; faddr will return to my caller
```

registers
---------

R.31 is reserved as the stack pointer, though it _can_ be manipulated
R.30 has special semantics. For CA and RT, it is the return address.
     For RW, it swaps the value with R.31

```
    ;     R.0-15 are scratch (use should be minimal, clobbered)
    ;     R.16-29 are don't touch registers (saved/restored by the callee)
    ;     R.30 is the return address (operated by CA/RT)
    ;     R.30 should be saved by the caller since CA will overwrite it
    ;     R.31 is the stack pointer (operated by push/pop instructions)
    ;     R.31 should be restored by the callee like so:
    ;       if it doesn't return a value, to the value it had at the time of
    ;           call
    ;       if it returns values, these values will be in the contractual
    ;           order starting at the address the stack was at the time
    ;           of the call, and the SP adjusted accordingly
    ;     
    ;     R.30&31 are swapped by RW

    ;     R0-15 could be used to accessing stack variables using the RW
    ;       trickery
```

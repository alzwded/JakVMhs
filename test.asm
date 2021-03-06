; TODO CA and RT operations have changed behaviour; update these examples

.data   ; TODO store strings in unicode 16be
:myVal   7          42, 1, 2, 3, 4, 5, 42     ; comment
:oneWord 1          42              ; yeah
:str     6          'hello', 0      ;huh?

; begin code

.code
    PI  :myVal
    PI  :oneWord
    LD
    PI  :str
    PI  10              ; x (r0) = 10
    PR.0                ;

    RP.0                ; with(x)
:loop
    PI  :print_num      ; call print_num
    CA                  ; call
    
    RD.0                ; x--
    RP.0                ; with(x)
    PI  :done           ; if(x == 0) goto done
    JZ                  ;
    RP.0
    PI  :loop           ; loop
    JP                  ;
:done
    HL
    PI  :done           ; halt
    JP                  ;

:print_num
    ; setup parameters
    PR.29               ; test
    RP.29
    PR.0                ; n (r0) = param1
    RP.30               ; store previous call state
    ; operate on parameters
    PI  11              ; n = 11 - n
    RP.0                ;
    SU                  ; 
    PI  :sysprintint
    CA                  ; call system routine
    PR.30               ; restore previous return address
    RT                  ; return

:sysprintint
    PI  3               ; call log_word
    IN                  ;
    RT                  ; return
; do some magic to get this onscreen

;==========================================
:memcmp
    PR.0                ; num
    PR.1                ; p2
    PR.2                ; p1
    PI  0               ; count = 0
    PR.3                ;

:memcmp_loop
    PR.0                ; compare count & num
    PR.3                ;
    SU                  ;
    PI  :memcmp_eq      ; if(count == num) return equal
    JZ                  ;

    RI.3                ; count++

    RP.1                ; compare *p1 and *p2
    LD
    RI.1
    RP.2
    LD
    RI.2
    SU
    PI  :memcmp_loop    ; continue if equal
    JZ                  ;

    RP.3                ; return count
    RT

:memcmp_eq
    PI  0
    RT
;==========================================
:memset
    PR.0                ; num
    PR.1                ; val
    PR.2                ; p

    RP.0                ; count = num
    PR.3

:memset_loop
    RP.3                ; return if count == 0
    PI  :memset_done    ;
    JZ                  ;

    RD.3                ; count++

    RP.2                ; *p = val
    RP.1                ;
    ST                  ;
    RI.2                ; p++
    PI  :memset_loop    ; loop
    JP                  ;
:memset_done
    RT
;==========================================

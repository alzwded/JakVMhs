.data
:hellow 7   'hello world!', 0
:fortytwo   1   42

.code
    PI  :fortytwo       ; @fortytwo
    LD                  ; deref
    PI  0               ; save_word(@0, @fortytwo)
    PI  11
    IN

    PI  :hellow         ; @hellow
    PI  7               ; length 7
    PI  1               ; @1
    PI  13              ; put_save_data(@1, 7, @hellow)
    IN

.data               ; .data and .code can be intertwined
:blanks 10  -       ; 10x 0
:toten  10  1, 2, 3, 4, 5, 6, 7, 8, 9, 10

.code
    PI  :toten          ; put_save_data(@8, 10, @toten)
    PI  10
    PI  8
    PI  13
    IN

    PI  0               ; get_word(@0)
    PI  10
    IN

    PI  3               ; logword
    IN

    PI  :blanks         ; get_save_data(@8, 10, into :blanks)
    PI  10
    PI  8
    PI  12
    IN

    PI  0               ; i = 0
    PR.0
:loop
    RP.0
    PI  10
    SU
    PI :done
    JZ                  ; while(i != 10) {

    PI  :blanks         ;   x = blanks + i
    RP.0
    AD
    LD                  ;   deref x
    PI  3               ;   logword(x)
    IN
    RI.0                ;   ++i
    PI  :loop
    JP                  ; }
:done
    HL                  ; exit

; ending code

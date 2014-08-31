.data
:str    6   'testutils', 0
:msg    4   '2^3 = ', 0

.code
; call printnum from testutils
    PI  42
    PI  0
    PI  :str
    PI  20
    IN

; call testpow from testutils
    PI  2
    PI  3
    PI  1
    PI  :str
    PI  20
    IN

; display result
    PI  :msg
    PI  5
    IN
    PI  3
    IN

; exit
    HL

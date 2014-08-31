.data
:buf    1   -
:str    6   'testutils', 0

.code
; assign short name
    PI  :str
    PI  1
    IN
    PI  :buf
    ST

; call printnum from testutils
    PI  42
    PI  0
    PI  :buf
    LD
    PI  20
    IN

; call testpow from testutils
    PI  2
    PI  3
    PI  1
    PI  :buf
    LD
    PI  20
    IN

; display result
    PI  3
    IN

; exit
    HL

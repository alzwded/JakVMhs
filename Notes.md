Notes to be taken into account for the documentation
====================================================

calling procedures
------------------

```
PI :param1          ; push parameters
PI :param2
NO                  ; etc
PI :proc_label      ; address of the proc
SV                  ; save return address
JP                  ; the jump needs to immediately follow the save
```

```
:proc_label
AD                  ; do something with the parameters
RT                  ; return
```

registers
---------

R.31 is reserved as the stack pointer, though it _can_ be manipulated

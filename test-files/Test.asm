        ORG  0x8000

start:  ADD  A, 0x42       ; A = A + 0x42
        LD   B, 50        ; loop counter = 50

loop:   INC  A            ; A = A + 1
        DJNZ loop         ; B--; if B != 0, jump to loop

        HALT              ; stop (or RET, NOP loop, etc.)   

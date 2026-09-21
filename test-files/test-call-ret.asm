
        ORG  0x0000

start:  ADD  A, 0x42       ; A = A + 0x42
        LD   B, 50        ; loop counter = 50
	call LOOP
	halt

loop:   INC  A            ; A = A + 1
        DJNZ loop         ; B--; if B != 0, jump to loop
	nop
	ret
       HALT              ; stop (or RET, NOP loop, etc.)   

addi %r6, %r0, $0 ; x = 0
addi %r10, %r0, $2000 ; const int 2000
addi %r11, %r0, $84  ; const int 83
addi %r7, %r0, $0 ; i = 0
addi %r8, %r0, $0 ; j = 0
addi %r9, %r0, $0 ; k = 0
addi %r6, %r6, $1 ; x++
addi %r9, %r9, $1 ; k++
blt %r9, %r10, $-8 ; if (k < 2000) loop
addi %r8, %r8, $1 ; j++
blt %r8, %r10, $-20 ; if (j < 2000) loop
addi %r7, %r7 $1 ; i++
blt %r7, %r11, $-32 ; if (i < 83) loop
addi %r0, %r0, $0   ; stop at here

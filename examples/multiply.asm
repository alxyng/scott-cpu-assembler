pad 50          ; add 50 0 bytes to beginning of binary output file

data r3, 1      ; put 1 into R3
xor r2, r3      ; put 0 into R2
clf             ; clear flags
shr r0, r0      ; set carry bit if bit 0 or r0 is 1
jc 59           ; do the add
jmp 61          ; skip the add
clf             ; clear flags
add r1, r2      ; add this line
clf             ; clear flags
shl r1, r1      ; multiply top number by 2
shl r3, r3      ; shift counter
jc 68           ; out if done
jmp 53          ; do next step

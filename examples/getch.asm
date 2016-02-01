data r2, 00001111b  ; put address of keyboard in R2
outa r2             ; select keyboard
ind r3              ; get ascii of key pressed
xor r2, r2          ; clear address in R2
outa r2             ; un-select keyboard

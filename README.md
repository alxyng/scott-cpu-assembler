# Scott CPU assembler

A machine code assembler for the Scott CPU designed in the book [But How Do It Know?](http://www.buthowdoitknow.com/)

## Build

The assembler consists of a single `.c` file that can be built using:
```
make
```
from the projects root directory.

The assembler was built and has been tested using clang on Mac OS X but should compile using gcc on Linux just fine.

## Run

To invoke the assembler, use:
```
./sca <file>
```
where `<file>` is the name of the assembly file you want to compile. For example to compile the `multiply.asm` example, issue:
```
./sca examples/multiply.asm
```
This will output a file with `.bin` replacing `.asm` extension in the same directory i.e. `examples/multiply.bin`. This is the binary output file. **Note**: this file extension replace will replace the extension after the first `.` found in the file or append `.bin` if no file extension is found e.g:

```
my_assembly_file.txt -> my_assembly_file.bin
```
```
my_assembly_file2 -> my_assembly_file2.bin
```

## Instruction Set

### Arithmetic and Logic Instructions
Opcode (binary)|Opcode (hex)|Mnemonic|Operands|Description|Operation|Flags|#Clocks
-|-|-|-|-|-|-
1000&#95;&#95;&#95;&#95;|8&#95;|ADD|RA, RB|Add the contents of RA to RB and store result in RB|RB ← RA + RB|C, A, E, Z|3
1001&#95;&#95;&#95;&#95;|9&#95;|SHR|RA, RB|Right shift the contents of RA and store result in RB|RB ← RA >> 1|C, A, E, Z|3
1010&#95;&#95;&#95;&#95;|A&#95;|SHL|RA, RB|Left shift the contents of RA and store result in RB|RB ← RA << 1|C, A, E, Z|3
1011&#95;&#95;&#95;&#95;|B&#95;|NOT|RA, RB|NOT RA and store the result in RB|RB ← ~RA|A, E, Z|3
1100&#95;&#95;&#95;&#95;|C&#95;|AND|RA, RB|AND RA and RB and store the result in RB|RB ← RA & RB|A, E, Z|3
1101&#95;&#95;&#95;&#95;|D&#95;|OR|RA, RB|OR RA and RB and store the result in RB|RB ← RA &#124; RB|A, E, Z|3
1110&#95;&#95;&#95;&#95;|E&#95;|XOR|RA, RB|XOR RA and RB and store the result in RB|RB ← RA ^ RB|A, E, Z|3
1111&#95;&#95;&#95;&#95;|F_|CMP|RA, RB|Compare RA and RB|RA - RB|A, E, Z|3

### Load and Store Instructions
Opcode (binary)|Opcode (hex)|Mnemonic|Operands|Description|Operation|Flags|#Clocks
-|-|-|-|-|-|-
0000&#95;&#95;&#95;&#95;|0&#95;|LD|RA, RB|Load RB from RAM at RAM address in RA|RB ← [RA]|-|(2) 3
0001&#95;&#95;&#95;&#95;|1&#95;|ST|RA, RB|Store RB into RAM at RAM address in RA|[RA] ← RB|-|(2) 3

### Data Instruction
Opcode (binary)|Opcode (hex)|Mnemonic|Operands|Description|Operation|Flags|#Clocks
-|-|-|-|-|-|-
001000&#95;&#95;|2&#95;|DATA|RB, K|Load byte constant, K into RB|RB ← K|-|3

### Branch Instructions
Opcode (binary)|Opcode (hex)|Mnemonic|Operands|Description|Operation|Flags|#Clocks
-|-|-|-|-|-|-
001100&#95;&#95;|3&#95;|JMPR|RB|Jump to the RAM address stored in RB|IAR ← RB|-|(1) 3
01000000|40|JMP|K|Jump to the RAM address K|IAR ← K|-|(2) 3
01011000|58|JC|K|Jump to RAM address K if the 'carry' flag is set|If (C == 1) then IAR ← K|-|3
01010100|54|JA|K|Jump to RAM address K if the 'a is larger' flag is set|If (A == 1) then IAR ← K|-|3
01010010|52|JE|K|Jump to RAM address K if the 'equal' flag is set|If (E == 1) then IAR ← K|-|3
01010001|51|JZ|K|Jump to RAM address K if the 'zero' flag is set|If (Z == 1) then IAR ← K|-|3
01011100|5C|JCA|K|Jump to RAM address K if C or A are set|If (C == 1 or A ==1) then IAR ← K|-|3
01011010|5A|JCE|K|Jump to RAM address K if C or E are set|If (C == 1 or E ==1) then IAR ← K|-|3
01011001|59|JCZ|K|Jump to RAM address K if C or Z are set|If (C == 1 or Z ==1) then IAR ← K|-|3
01010110|56|JAE|K|Jump to RAM address K if A or E are set|If (A == 1 or E ==1) then IAR ← K|-|3
01010101|55|JAZ|K|Jump to RAM address K if A or Z are set|If (A == 1 or Z ==1) then IAR ← K|-|3
01010011|53|JEZ|K|Jump to RAM address K if E or Z are set|If (E == 1 or Z ==1) then IAR ← K|-|3
01011110|5E|JCAE|K|Jump to RAM address K if C or A or E are set|If (C == 1 or A ==1 or E == 1) then IAR ← K|-|3
01011101|5D|JCAZ|K|Jump to RAM address K if C or A or Z are set|If (C == 1 or A ==1 or Z == 1) then IAR ← K|-|3
01011011|5B|JCEZ|K|Jump to RAM address K if C or E or Z are set|If (C == 1 or Z ==1 or Z == 1) then IAR ← K|-|3
01010111|57|JAEZ|K|Jump to RAM address K if A or E or Z are set|If (A == 1 or E ==1 or Z == 1) then IAR ← K|-|3
01011111|5F|JCAEZ|K|Jump to RAM address K if C or A or E or Z are set|If (C == 1 or A ==1 or E == 1 or Z == 1) then IAR ← K|-|3

### Clear Flags Instruction
Opcode (binary)|Opcode (hex)|Mnemonic|Operands|Description|Operation|Flags|#Clocks
-|-|-|-|-|-|-
01100000|60|CLF|-|Clear all flags|C = A = E = Z = 0|C, A, E, Z|(1) 3

### IO Instructions
Opcode (binary)|Opcode (hex)|Mnemonic|Operands|Description|Operation|Flags|#Clocks
-|-|-|-|-|-|-
011100&#95;&#95;|7&#95;|IND|RB|Input IO data to RB|IO (data) ← RB|-|(1) 3
011101&#95;&#95;|7&#95;|INA|RB|Input IO address to RB|IO (address) ← RB|-|(1) 3
011110&#95;&#95;|7&#95;|OUTD|RB|Output RB to IO as data|RB ← IO (data)|-|(1) 3
011111&#95;&#95;|7&#95;|OUTA|RB|Output RB to IO as address|RB ← IO (address)|-|(1) 3


## TODO

- `-o` output file cmd line option
- label implementation for branching rather than specifying absolute addresses
- org directive to add constants to label addresses

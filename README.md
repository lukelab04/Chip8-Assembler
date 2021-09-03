# Chip8-Assembler
Assembler for a modified version of the Chip-8 instruction set
# How to use
To compile, run ```gcc -o asm.exe asm.c```. 

Then, once you have written your assembly program, run ```./asm.exe [programname.asm]```. The assembled file will be saved as "program.hex". 

# Instruction Set

The base instruction set can be found [here](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#2.5). However, there are a few additions to this program. 

The instruction ```ld I, vx``` was added, which sets the value of the I register equal to the value in vx. 

Comments are defined with ';', as follows:
```
cls       ;Clear Screen
ret       ;Return from subroutine
```

Anchor points are predefined places where a program can jump to. They can be created as follows:
```
.myAnchorPoint
;Implement routine here
```

Later, Anchor points can be called like this:
```
jp myAnchorPoint
;PC is set to the place where myAnchorPoint was defined
```

It is important to remember that all words are converted to lowercase during the assembly. Therefore, 'myanchorpoint' and 'MYANCHORPOINT' are equivalent.

The binary values of a few instructions were also altered to make room for more instructions in the future.

# Running your program

Unfortunatley, since this is a modified set, preexisting emulators will not work out of the box. You may be able to modify one or write your own to run your program. I am also working on an emulator, but I do not feel that it is complete enough to publish. However, it is in a functional state, so I was able to test the assembler to make sure it was working. I even got Pong running, as shown below. ![chip8](https://user-images.githubusercontent.com/79181426/132065255-83d435af-702e-4214-a7c4-41c29b55b7f3.png)


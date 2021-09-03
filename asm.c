#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

//Keep track of program line count
int linecount = 1;
int PC = 0;
char* programName;

//Variable sized array to keep track of anchorpoints in the assembly
typedef struct{
    char name[32];
    int place;
} anchorPoint;

typedef struct{
    size_t size;
    anchorPoint values[];
} anchorPointList;


//Helper function to add an item to the back of the anchorpoint list
int push_back(anchorPointList **arr, anchorPoint val){
    size_t x = *arr ? arr[0]->size : 0, y = x + 1;

    if((x & y) == 0){
        void *temp = realloc(*arr, sizeof **arr + (x + y) * sizeof(arr[0] -> values[0]));

        if(!temp) return 1;
        *arr = temp;
    }

    arr[0]->values[x] = val;
    arr[0]->size = y;
    return 0;
}

//Helper function to get the numerical value of an anchorpoint
int getPCFromAnchorpoint(char ac[32], anchorPointList *a){
    for(int i = 0; i < a->size; i++){
        if(strcmp(ac, a->values[i].name) == 0){
            return a->values[i].place;
        }
    }

    return -1;
}


//Helper function to get a word from the line buffer
void getword(unsigned char word[32], unsigned char string[32], int wordcount){
    int wc = 0;
    int i = 0;
    int wspot = 0;

    //Remove spaces
    while(i < 32 && string[i] == ' '){i++;}
    int reset = 0;
    //Reset every character in the current word array
    while(reset < 32){
        word[reset] = '\0';
        reset++;
    }

    //Loop though buffer, separating words at spaces or commas
    while(i < 32){
        if(string[i] == '\0') return;
        else if(string[i] == '\n'){i++; continue;}
        else if(string[i] == ' ' || string[i] == ','){
            if(wordcount == wc){return;}
            while(i < 32 && (string[i] == ' ' || string[i] == ',')){i++;}
            wc++;
            continue;
        }
        else if(wordcount == wc){
            word[wspot] = tolower(string[i]);
            wspot++;
        }

        i++;
    }
    return;
}

//Helper function to see if a word is a number
bool isnumber(unsigned char word[32]){
    for(int i = 0; i < strlen(word); i++){
        if(word[i] == '\n') break;
        if(!isdigit(word[i])) return false;
    }
    return true;
}


//Helper function to throw an error with a message and line count
void error(char message[256]){
    printf(message, programName, linecount);
    exit(1);
}


//Helper function to convert a string to an integer
int stringToNum(unsigned char c[32], anchorPointList *a){
    if(isnumber(c)){
        return atoi(c);
    }
    else{
        int num = getPCFromAnchorpoint(c, a);
        if(num == -1) error("Expected address number in %s at line %i\n");
        return num;
    }
}


//Helper function to get the register number from a word in the format 'vx'
unsigned char getRegisterNumber(char str[16]){
    if(str[0] != 'v'){error("Expected address (vx) in %s at line %i\n");}
    
    //If the number is only 1 digit long, str[2] will just be \0
    char digits[3] = {str[1], str[2], '\0'};

    if(!isnumber(digits)){error("Expected address number in %s at line %i\n");}

    unsigned char addr = atoi(digits);

    if(addr < 0 || addr > 15){error("Address number in %s at line %i must be between 0 and 15.\n");}

    return addr;
}

int main(int argc, char *argv[]){
    if(argc < 2){printf("Error: Too few arguments. \n"); return 1;}
    programName = argv[1];

    //Open the file
    FILE *infile = fopen(programName, "r");
    //Output file is program.hex
    FILE *outfile = fopen("program.hex", "wb");
    if(infile == NULL){printf("Error: Could not find input file. \n"); return 1;}

    anchorPointList *AnchorList = NULL;

    unsigned char buffer[128];

    //Get a line from the input file
    while(fgets(buffer, 128, infile)){
        unsigned char word[32];
        //If the line is blank, just continue
        if(strlen(buffer) <= 1){
            linecount++;
            continue;
        }
        //If the line is a comment, continue
        else if(buffer[0] == ';'){
            linecount++;
            continue;
        }
        //If the line starts with ',', the word is defining an anchor point
        else if(buffer[0] == '.'){
            unsigned char* tempbuf = buffer + 1;

            getword(word, tempbuf, 0);

            anchorPoint a;
            strcpy(a.name, word);
            a.place = PC;
            if(push_back(&AnchorList, a) == -1) error("Could not parse Anchor Point in %s at line %i.\n");
            
            linecount++;
            continue;
        }
        
        getword(word, buffer, 0);

        //SYS is in the instruction set, but the does not need an implementation 
        if(strcmp("sys", word) == 0){printf("System call. Skipping.\n");}

        //Clear screen
        else if(strcmp("cls", word) == 0){
            unsigned short opcode = (0 << 12) | 0x00E0;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }
        //Return from subroutine
        else if(strcmp("ret", word) == 0){
            unsigned short opcode = (0 << 12) | 0x00EE;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }
        //Jump to address
        else if(strcmp("jp", word) == 0){
            getword(word, buffer, 1);

            if(word[0] == 'v'){
                getword(word, buffer, 2);
                unsigned short addr = stringToNum(word, AnchorList);
                if(addr < 0 || addr > 4095) error("Address in %s at line %i must be between 0 and 4095\n");

                unsigned short opcode = 0xB << 12;
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }

            else{
                unsigned short opcode = 0x1 << 12;
                unsigned short addr = stringToNum(word, AnchorList);
                if(addr < 0 || addr > 4095) error("Address in %s at line %i must be between 0 and 4095.\n");
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }

        }
        //Call subroutine
        else if(strcmp("call", word) == 0){
            getword(word, buffer, 1);

            unsigned short opcode = 0x2 << 12;
            unsigned short addr = stringToNum(word, AnchorList);
            if(addr < 0 || addr > 4095) error("Address in %s at line %i must be between 0 and 4095");
            opcode = opcode | addr;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Skip instruction if equal
        else if(strcmp("se", word) == 0){
            getword(word, buffer, 1);

            unsigned short addr1 = getRegisterNumber(word);
            unsigned short addr2;
            getword(word, buffer, 2);

            if(word[0] == 'v'){
                addr2 = getRegisterNumber(word);
                unsigned short opcode = 0x5 << 12;
                addr1 = addr1 << 4;
                addr1 = addr1 | addr2;
                addr1 = addr1 << 4;
                opcode = opcode | addr1;
                fwrite(&opcode, sizeof(opcode), 1, outfile);

            }
            else{
                addr2 = stringToNum(word, AnchorList);
                if(addr2 < 0 || addr2 > 255){error("Address in %s at line %i must be between 0 and 255.\n");}
                unsigned short opcode = 0x3 << 12;
                addr1 = addr1 << 8;
                addr1 = addr1 | addr2;
                opcode = addr1 | opcode;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }


        }

        //Skip if not equal
        else if(strcmp("sne", word) == 0){
            getword(word, buffer, 1);

            unsigned short addr1 = getRegisterNumber(word);
            unsigned short addr2;
            getword(word, buffer, 2);

            if(word[0] == 'v'){
                addr2 = getRegisterNumber(word);
                unsigned short opcode = 0x9 << 12;
                addr1 = addr1 << 4;
                addr1 = addr1 | addr2;
                addr1 = addr1 << 4;
                opcode = opcode | addr1;
                fwrite(&opcode, sizeof(opcode), 1, outfile);

            }
            else{
                addr2 = stringToNum(word, AnchorList);
                if(addr2 < 0 || addr2 > 255){error("Address in %s at line %i must be between 0 and 255.\n");}
                unsigned short opcode = 0x4 << 12;
                addr1 = addr1 << 8;
                addr1 = addr1 | addr2;
                opcode = addr1 | opcode;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }
        }

        //Load some data into or out of some registers
        else if(strcmp("ld", word) == 0){
            getword(word, buffer, 1);
            //Load data into a register
            if(word[0] == 'v'){
                unsigned short addr1 = getRegisterNumber(word);
                unsigned short addr2;
                getword(word, buffer, 2);

                //Load from another register
                if(word[0] == 'v'){
                    addr2 = getRegisterNumber(word);
                    unsigned short opcode = 0x8 << 12;
                    addr1 = addr1 << 8;
                    addr1 = addr1 | (addr2 << 4);
                    opcode = opcode | addr1;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }
                //Load from delay timer
                else if(strcmp(word, "dt") == 0){
                    unsigned short opcode = 0xF << 12;
                    addr1 = addr1 << 8;
                    addr1 = addr1 | 0x7;
                    opcode = opcode | addr1;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }
                //Load from keyboard
                else if(strcmp(word, "k") == 0){
                    unsigned short opcode = 0xF << 12;
                    addr1 = addr1 << 8;
                    addr1 = addr1 | 0xA;
                    opcode = opcode | addr1;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }
                //Load from memory
                else if(strcmp(word, "[i]") == 0){
                    unsigned short opcode = 0xF << 12;
                    addr1 = addr1 << 8;
                    addr1 = addr1 | 65;
                    opcode = opcode | addr1;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }
                //Load from literal number
                else{
                    addr2 = stringToNum(word, AnchorList);
                    if(addr2 < 0 || addr2 > 255){error("Address in %s at line %i must be between 0 and 255.\n");}
                    unsigned short opcode = 0x6 << 12;
                    addr1 = addr1 << 8;
                    addr1 = addr1 | addr2;
                    opcode = addr1 | opcode;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }
            }
            //Load into I register
            else if(word[0] == 'i'){
                getword(word, buffer, 2);
                
                //Load from register
                if(word[0] == 'v'){
                    unsigned short addr = getRegisterNumber(word);
                    if(addr < 0 || addr > 255) error("Address in %s at line %i must be between 0 and 255");
                    unsigned short opcode = 0xF << 12;
                    addr = (addr << 8) | 0x30;
                    opcode = opcode | addr;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }

                //Load from literal number
                else{
                    unsigned short addr = stringToNum(word, AnchorList);
                    if(addr < 0 || addr > 4095) error("Address in %s at line %i must be between 0 and 4095");
                    unsigned short opcode = 0xA << 12;
                    opcode = opcode | addr;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }

            }
            //Load into delay timer
            else if(strcmp(word, "dt") == 0){
                getword(word, buffer, 2);
                unsigned short addr = getRegisterNumber(word);
                unsigned short opcode = 0xf << 12;
                addr = addr << 8;
                addr = addr | 15;
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }
            //Load into sound timer
            else if(strcmp(word, "st") == 0){
                getword(word, buffer, 2);
                unsigned short addr = getRegisterNumber(word);
                unsigned short opcode = 0xF << 12;
                addr = addr << 8;
                addr = addr | 18;
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }
            //Load I with the value of vx
            else if(word[0] = 'f'){
                getword(word, buffer, 2);
                unsigned short addr = getRegisterNumber(word);
                unsigned short opcode = 0xF << 12;
                addr = addr << 8;
                addr = addr | 29;
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }
            //Load bcd representation of vx into memory
            else if(word[0] == 'b'){
                getword(word, buffer, 2);
                unsigned short addr = getRegisterNumber(word);
                unsigned short opcode = 0xF << 12;
                addr = addr << 8;
                addr = addr | 33;
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }
            //Load into memory
            else if(strcmp(word, "[i]") == 0){
                getword(word, buffer, 2);
                unsigned short addr = getRegisterNumber(word);
                unsigned short opcode = 0xF << 12;
                addr = addr << 8;
                addr = addr | 55;
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }
        }
        
        //Add number to register
        else if(strcmp("add", word) == 0){
            getword(word, buffer, 1);

            if(word[0] == 'v'){
                unsigned short addr1 = getRegisterNumber(word);
                unsigned short addr2;
                getword(word, buffer, 2);
                if(word[0] == 'v'){
                    addr2 = getRegisterNumber(word);
                    unsigned short opcode = 8 << 12;
                    addr1 = addr1 << 8;
                    addr2 = addr2 << 4;
                    addr1 = addr1 | addr2;
                    addr1 = addr1 | 4;
                    opcode = opcode | addr1;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }
                else{
                    addr2 = stringToNum(word, AnchorList);
                    if(addr2 < 0 || addr2 > 255){error("Byte in %s at line %i must be between 0 and 255.");}
                    unsigned short opcode = 7 << 12;
                    addr1 = addr1 << 8;
                    addr1 = addr1 | addr2;
                    opcode = opcode | addr1;
                    fwrite(&opcode, sizeof(opcode), 1, outfile);
                }
            }
            else if(word[0] == 'i'){
                getword(word, buffer, 2);
                if(!word[0] == 'v'){error("Expected register (vx) in %s at line %i\n");}
                unsigned short addr = getRegisterNumber(word);
                unsigned short opcode = 0xf << 12;
                addr = addr << 8;
                addr = addr | 0x1E;
                opcode = opcode | addr;
                fwrite(&opcode, sizeof(opcode), 1, outfile);
            }
        }
        
        //Perform logical OR on register
        else if(strcmp("or", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            getword(word, buffer, 2);
            unsigned short addr2 = getRegisterNumber(word);

            unsigned short opcode = 8 << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | 1;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);

        }

        //Perform logical AND on register
        else if(strcmp("and", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            getword(word, buffer, 2);
            unsigned short addr2 = getRegisterNumber(word);

            unsigned short opcode = 8 << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | 2;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Perform logical XOR on register
        else if(strcmp("xor", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            getword(word, buffer, 2);
            unsigned short addr2 = getRegisterNumber(word);

            unsigned short opcode = 8 << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | 3;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Subtract from register
        else if(strcmp("sub", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            getword(word, buffer, 2);
            unsigned short addr2 = getRegisterNumber(word);

            unsigned short opcode = 8 << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | 5;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Perform right bit shift on register
        else if(strcmp("shr", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            unsigned short addr2 = 0;

            unsigned short opcode = 8 << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | 6;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Subtract from register and set overflow register
        else if(strcmp("subn", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            getword(word, buffer, 2);
            unsigned short addr2 = getRegisterNumber(word);

            unsigned short opcode = 8 << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | 7;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Perform left bit shift on register
        else if(strcmp("shl", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);

            unsigned short addr2 = 0;

            unsigned short opcode = 8 << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | 0xE;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Generate random 8-bit number and AND it with the specified register
        else if(strcmp("rnd", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            getword(word, buffer, 2);
            unsigned short addr2 = stringToNum(word, AnchorList);
            if(addr2 < 0 || addr2 > 255) error("Value in %s at line %i must be between 0 and 255\n");

            unsigned short opcode = 0xC << 12;
            addr1 = (addr1 << 8) | addr2;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Draw sprite at the memory location in I
        else if(strcmp("drw", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);
            getword(word, buffer, 2);
            unsigned short addr2 = getRegisterNumber(word);
            getword(word, buffer, 3);
            unsigned char n = stringToNum(word, AnchorList);
            if(n < 0 || n > 15) error("Value in %s at line %i must be between 0 and 15");

            unsigned short opcode = 0xD << 12;
            addr1 = (addr1 << 8) | (addr2 << 4) | n;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Skip if a specified key is pressed
        else if(strcmp("skp", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);

            unsigned short opcode = 0xE << 12;
            addr1 = (addr1 << 8) | 0x9E;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }

        //Skip if a specified key is not pressed
        else if(strcmp("sknp", word) == 0){
            getword(word, buffer, 1);
            unsigned short addr1 = getRegisterNumber(word);

            unsigned short opcode = 0xE << 12;
            addr1 = (addr1 << 8) | 0xA1;
            opcode = opcode | addr1;
            fwrite(&opcode, sizeof(opcode), 1, outfile);
        }
        
        //Handle unknown instructions
        else{
            if(!isspace(word[0])){
                printf("%s\n", word);
                error("Unexpected token in %s at line %i\n");
            }
            else{
                linecount++;
                continue;
            }
        }

        
        linecount++;
        PC++;
    }

    //End byte
    unsigned short opcode = 0xFFFF;
    fwrite(&opcode, sizeof(opcode), 1, outfile);
    fwrite(&opcode, sizeof(opcode), 1, outfile);
    fclose(infile);
    fclose(outfile);

    //Free memory used by anchor list
    free(AnchorList);

    return 0;
}
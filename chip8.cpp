#include <bits/stdc++.h>
#include <graphics.h>
#include <conio.h>
#include <windows.h>

typedef unsigned char BYTE;
typedef unsigned short int WORD;

BYTE m_GameMemory[0xFFF];
BYTE m_V[16];
WORD m_Address; //16-bit address register
WORD m_ProgramCounter;
WORD m_I, m_Keys[16];
BYTE m_DelayTimer, m_SoundTimer;
std::vector<WORD> m_Stack;

BYTE m_ScreenData[64][32];

unsigned char chip8_fontset[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void CPUReset(char* programLocation) {
    m_Address = 0;
    m_ProgramCounter = 0x200;
    memset(m_V, 0, sizeof(m_V));
    memset(m_GameMemory, 0, sizeof(m_GameMemory));
    m_Stack.clear();
    memset(m_ScreenData, 0, sizeof(m_ScreenData));
    memset(m_Keys, 0, sizeof(m_Keys));

    for (int i=0; i<80; i++)
        m_GameMemory[i] = chip8_fontset[i];
    m_DelayTimer = m_SoundTimer = 0;
    srand(time(NULL));

    FILE *in;
    in = fopen(programLocation, "rb");
    fread( &m_GameMemory[0x200], 0xFFF, 1, in );
    fclose(in);
}

WORD GetNextOpcode() {
    WORD res = 0;
    res = m_GameMemory[m_ProgramCounter];
    res <<= 8;
    res |= m_GameMemory[m_ProgramCounter + 1];
    m_ProgramCounter += 2;
    return res;
}

void Opcode0NNN (WORD opcode) {
    // Calls machine code routine at address NNN.
}

void Opcode00E0(WORD opcode) {
    // Clears the screen.
    memset(m_ScreenData, 0, sizeof(m_ScreenData));
}

void Opcode00EE(WORD opcode) {
    // Retures from a subroutine.
    m_ProgramCounter = m_Stack.back();
    m_Stack.pop_back();
}

void Opcode1NNN(WORD opcode) {
    // Jumps to address NNN.
    m_ProgramCounter = opcode & 0x0FFF;
}

void Opcode2NNN(WORD opcode) {
    // Calls subroutine at NNN.
    m_Stack.push_back(m_ProgramCounter);
    m_ProgramCounter = opcode & 0xFFF;
}

void Opcode3XNN(WORD opcode) {
    // Skips the next instruction if VX equals NN.
    if (m_V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF))
        m_ProgramCounter += 2;
}

void Opcode4XNN(WORD opcode) {
    // Skips the next instruction if VX ineuals NN.
    if (m_V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
        m_ProgramCounter += 2;
}

void Opcode5XY0(WORD opcode) {
    // Skips the next instruction if VX equals to VY.
    if (m_V[(opcode & 0x0F00) >> 8] == m_V[(opcode & 0x00F0) >> 4])
        m_ProgramCounter += 2;
}

void Opcode6XNN(WORD opcode) {
    // Sets VX to NN.
    m_V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
}

void Opcode7XNN(WORD opcode) {
    // Adds NN to VX.
    m_V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
}

void Opcode8XY0(WORD opcode) {
    // Sets VX to the value of VY.
    m_V[(opcode & 0x0F00) >> 8] = m_V[(opcode & 0x00F0) >> 4];
}

void Opcode8XY1(WORD opcode) {
    // Sets VX to VX or VY.
    m_V[(opcode & 0x0F00) >> 8] |= m_V[(opcode & 0x00F0) >> 4];
}

void Opcode8XY2(WORD opcode) {
    // Sets VX to VX and VY.
    m_V[(opcode & 0x0F00) >> 8] &= m_V[(opcode & 0x00F0) >> 4];
}

void Opcode8XY3(WORD opcode) {
    // Sets VX to VX xor VY.
    m_V[(opcode & 0x0F00) >> 8] ^= m_V[(opcode & 0x00F0) >> 4];
}

void Opcode8XY4(WORD opcode) {
    // Sets VX to VX add VY.
    m_V[0xF] = (m_V[(opcode & 0x0F00) >> 8] > 0xFF - m_V[(opcode & 0x00F0) >> 4]);
    m_V[(opcode & 0x0F00) >> 8] += m_V[(opcode & 0x00F0) >> 4];
}

void Opcode8XY5(WORD opcode) {
    // VY is subtracted from VX.
    m_V[0xF] = (m_V[(opcode & 0x0F00) >> 8] > m_V[(opcode & 0x00F0) >> 4]) ? 1 : 0;
    m_V[(opcode & 0x0F00) >> 8] -= m_V[(opcode & 0x00F0) >> 4];
}

void Opcode8XY6(WORD opcode) {
    // Shifts VX to the right by 1, then stores the least significant bit of VX prior to the shift into VF.
    m_V[0xF] = m_V[(opcode & 0x0F00) >> 8] & 0x1;
    m_V[(opcode & 0x0F00) >> 8] >>= 1;
}

void Opcode8XY7(WORD opcode) {
    // Sets VX to VY minus VX.
    m_V[0xF] = (m_V[(opcode & 0x00F0) >> 4] > m_V[(opcode & 0x0F00) >> 8]) ? 1 : 0;
    m_V[(opcode & 0x0F00) >> 8] = m_V[(opcode & 0x00F0) >> 4] - m_V[(opcode & 0x0F00) >> 8];
}

void Opcode8XYE(WORD opcode) {
    // Shifts VX to the left by 1, stores the most significant bit of VX prior to the shift into VF.
    m_V[0xF] = m_V[(opcode & 0x0F00) >> 8] >> 7;
    m_V[(opcode & 0x0F00) >> 8] <<= 1;
}

void Opcode9XY0(WORD opcode) {
    // Skips the next instruction if VX inequals VY.
    if (m_V[(opcode & 0x0F00) >> 8] != m_V[(opcode & 0x00F0) >> 4])
        m_ProgramCounter += 2;
}

void OpcodeANNN(WORD opcode) {
    // Sets I to the address NNN.
    m_I = opcode & 0xFFF;
}

void OpcodeBNNN(WORD opcode) {
    // Jumps to the address NNN plus V0.
    m_ProgramCounter = m_V[0x0] + (opcode & 0xFFF);
}

void OpcodeCXNN(WORD opcode) {
    // Sets VX to the result of a bitwise and operation on a random number and NN.
    m_V[(opcode & 0x0F00) >> 8] = (rand() % 0xFF) & (opcode & 0x00FF);
}

void OpcodeDXYN(WORD opcode) {
    // Draw sth.
    int height = opcode & 0x000F;
    int coordx = m_V[(opcode & 0x0F00) >> 8];
    int coordy = m_V[(opcode & 0x00F0) >> 4];
    m_V[0xF] = 0;
    
    for (int yline = 0; yline < height; yline++) {
        BYTE data = m_GameMemory[m_I + yline];
        int xpixelinv = 7;
        int xpixel = 0;
        for (xpixel = 0; xpixel < 8; xpixel++, xpixelinv--) {
            int mask = 1 << xpixelinv;
            if (data & mask) {
                int x = coordx + xpixel;
                int y = coordy + yline;
                if (m_ScreenData[x][y] == 1) 
                    m_V[0xF] = 1;
                m_ScreenData[x][y] ^= 1;
            }
        }
    }
}

void OpcodeEX9E(WORD opcode) {
    // Skips the next instruction if the key stored in VX is pressed.
    if (m_Keys[m_V[(opcode & 0x0F00) >> 8]])
        m_ProgramCounter += 2; 
}

void OpcodeEXA1(WORD opcode) {
    // Skips the next instruction if the key stored in VX is not pressed.
    if (! m_Keys[m_V[(opcode & 0x0F00) >> 8]])
        m_ProgramCounter += 2;
}

void OpcodeFX07(WORD opcode) {
    // Sets VX to the value of the dalay timer.
    m_V[(opcode & 0x0F00) >> 8] = m_DelayTimer;
}

void OpcodeFX0A(WORD opcode) {
    // Stores the key pressed into VX.
    bool keyPress = false;
    for (int i = 0; i < 16; i++) {
        if (m_Keys[i] != 0) {
            m_V[(opcode & 0x0F00) >> 8] = i;
            keyPress = true;
        }
    }

    if (!keyPress)
        return;
}

void OpcodeFX15(WORD opcode) {
    // Sets the delay timer to VX.
    m_DelayTimer = m_V[(opcode & 0x0F00) >> 8];
}

void OpcodeFX18(WORD opcode) {
    // Sets the sound timer to VX.
    m_SoundTimer = m_V[(opcode & 0x0F00) >> 8];
}

void OpcodeFX1E(WORD opcode) {
    // Adds VX to I.
    m_V[0xF] = (m_I + m_V[(opcode & 0x0F00) >> 8]) > 0xFFF;
    m_I += m_V[(opcode & 0x0F00) >> 8];
}

void OpcodeFX29(WORD opcode) {
    // Sets I to the location of the sprite for the character in VX.
    m_I = m_V[(opcode & 0x0F00) >> 8] * 5;
}

void OpcodeFX33(WORD opcode) {
    int value = m_V[(opcode & 0x0F00) >> 8];
    
    int hundreds = value / 100;
    int tens = (value / 10) % 10;
    int units = value % 10;

    m_GameMemory[m_I] = hundreds;
    m_GameMemory[m_I + 1] = tens;
    m_GameMemory[m_I + 2] = units;
}

void OpcodeFX55(WORD opcode) {
    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
        m_GameMemory[m_I + i] = m_V[i];
    m_I += ((opcode & 0x0F00) >> 8) + 1;
}

void OpcodeFX65(WORD opcode) {
    for (int i = 0; i <= ((opcode & 0x0F00) >> 8); i++)
        m_V[i] = m_GameMemory[m_I + i];
    m_I += ((opcode & 0x0F00) >> 8) + 1;
}

void HandleOpcode() {
    WORD opcode = GetNextOpcode();

    std::cout << "Read opcode : " << std::hex << opcode << '\n';

    // Decode the opcode
    switch (opcode & 0xF000) {
        case 0x0000: {  // Needs further extraction
            switch(opcode & 0x000F) {
                case 0x0000: Opcode00E0(opcode); break;
                case 0x000E: Opcode00EE(opcode); break; 
                default: Opcode0NNN(opcode); break;
            }
        }
        break;
        case 0x1000: Opcode1NNN(opcode); break; // Jump Opcode
        case 0x2000: Opcode2NNN(opcode); break;
        case 0x3000: Opcode3XNN(opcode); break;
        case 0x4000: Opcode4XNN(opcode); break;
        case 0x5000: Opcode5XY0(opcode); break;
        case 0x6000: Opcode6XNN(opcode); break;
        case 0x7000: Opcode7XNN(opcode); break;
        case 0x8000: {
            switch (opcode & 0x000F) {
                case 0x0000: Opcode8XY0(opcode); break;
                case 0x0001: Opcode8XY1(opcode); break;
                case 0x0002: Opcode8XY2(opcode); break;
                case 0x0003: Opcode8XY3(opcode); break;
                case 0x0004: Opcode8XY4(opcode); break;
                case 0x0005: Opcode8XY5(opcode); break;
                case 0x0006: Opcode8XY6(opcode); break;
                case 0x0007: Opcode8XY7(opcode); break;
                case 0x000E: Opcode8XYE(opcode); break;
                default: break;
            }
            break;
        }
        case 0x9000: Opcode9XY0(opcode); break;
        case 0xA000: OpcodeANNN(opcode); break;
        case 0xB000: OpcodeBNNN(opcode); break;
        case 0xC000: OpcodeCXNN(opcode); break;
        case 0xD000: OpcodeDXYN(opcode); break;
        case 0xE000: {
            switch (opcode & 0x000F) {
                case 0x000E: OpcodeEX9E(opcode); break;
                case 0x0001: OpcodeEXA1(opcode); break;
                default: break;
            }
        }
        case 0xF000: {
            switch (opcode & 0x000F) {
                case 0x0007: OpcodeFX07(opcode); break;
                case 0x000A: OpcodeFX0A(opcode); break;
                case 0x0008: OpcodeFX18(opcode); break;
                case 0x000E: OpcodeFX1E(opcode); break;
                case 0x0009: OpcodeFX29(opcode); break;
                case 0x0003: OpcodeFX33(opcode); break;
                case 0x0005: {
                    switch (opcode & 0x00F0) {
                        case 0x0010: OpcodeFX15(opcode); break;
                        case 0x0050: OpcodeFX55(opcode); break;
                        case 0x0060: OpcodeFX65(opcode); break;
                        default: break;
                    }
                }
                default: break;
            }
        }
        default: break;
    }
    std::cout << "Opcode Handle Done!\n";
    return;
}

void drawxy(int x, int y) {
    int x1, y1, x2, y2;
    x1 = 20 * x, y1 = 20 * y, x2 = 20 * x + 19, y2 = 20 * y + 19;
    fillrectangle(x1, y1, x2, y2);
}

void drawScreen() {
    for (int drow = 0; drow < 64; drow++) 
        for (int dcol = 0; dcol < 32; dcol++) 
            if (m_ScreenData[drow][dcol]) drawxy(drow, dcol);
}


int keymap(unsigned char key) {
    switch (key) {
        case '1' : return 0x1;
        case '2' : return 0x2;
        case '3' : return 0x3;
        case '4' : return 0xc;
        case 'q' : return 0x4;
        case 'w' : return 0x5;
        case 'e' : return 0x6;
        case 'r' : return 0xd;
        case 'a' : return 0x7;
        case 's' : return 0x8;
        case 'd' : return 0x9;
        case 'f' : return 0xe;
        case 'z' : return 0xa;
        case 'x' : return 0x0;
        case 'c' : return 0xb;
        case 'v' : return 0xf;
        default : return -1;
    }
}

void processKey(char key) {
    int idx = keymap(key);
    if (idx >= 0)
        m_Keys[idx] = 1;
    else
        for (int i=0; i<16; i++) m_Keys[i] = 0;
}

void tick() {
    if (m_DelayTimer > 0) 
        --m_DelayTimer;
    if (m_SoundTimer > 0) {
        --m_SoundTimer;
        if (m_SoundTimer == 0)
            Beep(400, 400);
    }
}


int main() {
    initgraph(1280, 640);
    char* fileDir = (char*)"D:/0_Files/0Documents/Coding/C++/Projects/Chip8 Emulation/3-corax+.ch8";
    CPUReset(fileDir);
    std::cout << "CPU Initialization done!\n";
    while (true) {
        HandleOpcode();
        tick();
        if (_kbhit())
            processKey(_getch());
        drawScreen();
        Sleep(1000 / 60);
    }
    return 0; 
}
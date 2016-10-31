/*
 * This sketch is for dumping Motorola MC6805P2.
 * 
 * We created this project in order to fix an HP powersupply which has a failed MC6805P2. 
 * MC68705P3 is reprogrammable and can be used to emulate MC6805P2.
 * 
 * Thanks to Sean Riddle and his brilliant work on dumping and reprogramming this
 * Microprocessor, we were able to reproduce the dumping and reprogramming procedure
 * with an Arduino and a few extra components. We decided to build a dedicated board 
 * for those who may have similar need.
 * 
 * Sean's blogs:
 * http://www.seanriddle.com/mc6805p2.html
 * http://www.seanriddle.com/mc68705p5.html
 * 
 * The source code at GitHub:
 * https://github.com/SandboxElectronics/MC6805P2_clone
 * 
 * The board (to be updated):
 * http://sandboxelectronics.com
 */

#define RW_CTRL A4
#define R     HIGH
#define W      LOW

#define MCU_CLK  6
#define RD_RST  A5

#define IO0     A0 //MC6805P2 PB0
#define IO1     A1 //MC6805P2 PB1
#define IO2     A2 //MC6805P2 PB2
#define IO3     A3 //MC6805P2 PB3
#define IO4      2 //MC6805P2 PB4
#define IO5      3 //MC6805P2 PB5
#define IO6      4 //MC6805P2 PB6
#define IO7      5 //MC6805P2 PB7

#define IO8     11 //MC6805P2 PC2
#define IO9     12 //MC6805P2 PC0
#define IO10    13 //MC6805P2 PC1

byte ROM_128_to_255[128];
byte ROM_960_to_1923[964];
byte ROM_2040_to_2047[8];

void setup() {
    Serial.begin(115200);
    
    pinMode(RW_CTRL, OUTPUT);
    digitalWrite(RW_CTRL, R);
    
    digitalWrite(IO0, HIGH);
    digitalWrite(IO1, HIGH);
    digitalWrite(IO2, HIGH);
    digitalWrite(IO3, HIGH);
    digitalWrite(IO4, HIGH);
    digitalWrite(IO5, HIGH);
    digitalWrite(IO6, HIGH);
    digitalWrite(IO7, HIGH);
    digitalWrite(IO8, HIGH);
    digitalWrite(IO9, HIGH);
    digitalWrite(IO10, HIGH);
    
    pinMode(IO0, INPUT);
    pinMode(IO1, INPUT);
    pinMode(IO2, INPUT);
    pinMode(IO3, INPUT);
    pinMode(IO4, INPUT);
    pinMode(IO5, INPUT);
    pinMode(IO6, INPUT);
    pinMode(IO7, INPUT);
    pinMode(IO8, INPUT);
    pinMode(IO9, INPUT);
    pinMode(IO10, INPUT);
    
    pinMode(MCU_CLK, OUTPUT);
    pinMode(RD_RST, OUTPUT);
}


void loop() {
    byte     b;
    byte     c;
    uint16_t i;
    byte     slide_counter = 1;
    uint16_t zero_counter;
    uint16_t sync_counter;
    uint16_t addr;
    uint16_t prev_addr;

    Serial.println("------------------------------");
    Serial.println(" Type D or d to start dumping");
    Serial.println("------------------------------");
    
    // Wait for command
    while(1) {
        if (Serial.available()) {
            b = Serial.read();

            if (b == 'D' || b == 'd') {
                Serial.println("Start Dumping...");
                break;
            }
        }
    }

    // Reset MCU
    digitalWrite(RD_RST, LOW);
    clock(20);
    digitalWrite(RD_RST, HIGH);

    // Syncronize address
    while(1) {
        prev_addr    = 0xFFFF;
        sync_counter = 2048;
    
        while(1) {
            b  = PINC & 0x0F;
            b |= PIND << 2 & 0xF0;

            c  = PINB >> 3 & 0x07;

            addr = restore_address(b, c);

            if (prev_addr == 0xFFFF) {
                if (addr == 0x59D && sync_counter-- > 0) {
                    prev_addr = addr;
                } else {
                    clock(2);
                    break;
                }
            } else {
                if (addr == (prev_addr+1)%2048) {
                    prev_addr = addr;
                } else {
                    clock(2);
                    break;
                }

                if (addr == 0) {
                    clock(2);
                    break;
                }
            }

            clock(8);
        }

        if (addr == 0 && prev_addr == 0) {
            break;
        }
    }

    clock(128 * 8);

    Serial.println("Dumping ROM 128-255...");
    for (i=0; i<128; i++) {
        b  = PINC & 0x0F;
        b |= (PIND << 2) & 0xF0;
        
        ROM_128_to_255[i] = b;

        clock(8);
    }

    clock(704 * 8);

    Serial.println("Dumping ROM 960-1923...");
    for (i=0; i<964; i++) {
        b  = PINC & 0x0F;
        b |= (PIND << 2) & 0xF0;
        
        ROM_960_to_1923[i] = b;

        clock(8);
    }

    clock(116 * 8);

    Serial.println("Dumping ROM 2040-2047...");
    for (i=0; i<8; i++) {
        b  = PINC & 0x0F;
        b |= (PIND << 2) & 0xF0;
        
        ROM_2040_to_2047[i] = b;

        clock(8);
    }

    

    Serial.println();
    Serial.print("byte ROM_128_to_255[128] = {");
    printArray(ROM_128_to_255, 128);
    Serial.println();
    Serial.println("};");
    Serial.println();
    
    Serial.print("byte ROM_960_to_1923[964] = {");
    printArray(ROM_960_to_1923, 964);
    Serial.println();
    Serial.println("};");
    Serial.println();
    
    Serial.print("byte ROM_2040_to_2047[8] = {");
    printArray(ROM_2040_to_2047, 8);
    Serial.println();
    Serial.println("};");
    Serial.println();

    Serial.println("Dump succeeded!");
    Serial.println("The arrays above are used in Program.ino:");
    Serial.println();
}


void clock(uint16_t n) {
    while (n--) {
        digitalWrite(MCU_CLK, LOW);
        delayMicroseconds(100);
        digitalWrite(MCU_CLK, HIGH);
        delayMicroseconds(100);
    }
}


void printArray(byte *pdata, uint16_t len) {
    uint16_t i;

    for (i=0; i<len; i++) {
        if (i % 16 == 0) {
            Serial.println();
            Serial.print("    ");
        }
        
        Serial.print("0x");

        if (*pdata < 0x10) {
            Serial.print('0');
        }
        
        Serial.print(*pdata, HEX);
        pdata++;
        
        if (i != len - 1) {
            Serial.print(',');
            Serial.print(' ');
        }
    }
}


// The address bit were scrambled. The mapping are as followed
// MC6805P2 PB0 -> addr 0
// MC6805P2 PB7 -> addr 1
// MC6805P2 PB1 -> addr 2
// MC6805P2 PB6 -> addr 3
// MC6805P2 PB5 -> addr 4
// MC6805P2 PB4 -> addr 5
// MC6805P2 PC0 -> addr 6
// MC6805P2 PC1 -> addr 7
// MC6805P2 PC2 -> addr 8
// MC6805P2 PB2 -> addr 9
// MC6805P2 PB3 -> addr 10

uint16_t restore_address(byte b, byte c) {
    uint16_t addr = 0;

    if (b & _BV(0)) {
        addr |= 1<<0;
    }
    
    if (b & _BV(7)) {
        addr |= 1<<1;
    }

    if (b & _BV(1)) {
        addr |= 1<<2;
    }

    if (b & _BV(6)) {
        addr |= 1<<3;
    }

    if (b & _BV(5)) {
        addr |= 1<<4;
    }

    if (b & _BV(4)) {
        addr |= 1<<5;
    }

    if (c & _BV(1)) {
        addr |= 1<<6;
    }

    if (c & _BV(2)) {
        addr |= 1<<7;
    }

    if (c & _BV(0)) {
        addr |= 1<<8;
    }

    if (b & _BV(2)) {
        addr |= 1<<9;
    }

    if (b & _BV(3)) {
        addr |= 1<<10;
    }

    return addr;
}


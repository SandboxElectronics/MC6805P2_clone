/*
 * This sketch is for programming Motorola MC68705P3.
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

#define OPEN  HIGH
#define CLOSE  LOW

#define MCU_CLK  6 // PD6
#define PRG_EN   7 // PD7 S2
#define CTR_CLK  8 // PB0
#define CTR_RST  9 // PB1
#define WR_RST  10 // PB2 S1

#define IO0     A0
#define IO1     A1
#define IO2     A2
#define IO3     A3
#define IO4      2
#define IO5      3
#define IO6      4
#define IO7      5

#define PROGRAMMED A7
#define VERIFIED   A6

// *************************************************************
// Change option byte below according to the datasheet as needed
// *************************************************************
#define OPTION_BYTE (0x00)

unsigned int Counter = 0;
byte Change;
byte CounterClockState = HIGH;
byte CounterResetState = LOW;
byte Programmed = false;
byte Verified   = false;

// *************************************************************
// Replace the 3 arrays below with the ones from Dump.ino output
// *************************************************************
byte ROM_128_to_255[128];
byte ROM_960_to_1923[964];
byte ROM_2040_to_2047[8];

void setup() {
    int  i;
    byte b;
    
    Serial.begin(115200);
    Serial.println("");
    Serial.println("Waiting for command:");
    Serial.println("S - START");
    Serial.println("A - ABORT");
    
    pinMode(RW_CTRL, OUTPUT);
    pinMode(MCU_CLK, OUTPUT);
    pinMode(PRG_EN,  OUTPUT);
    pinMode(CTR_CLK,  INPUT);
    pinMode(CTR_RST,  INPUT);
    pinMode(WR_RST,  OUTPUT);
    
    pinMode(IO0, OUTPUT);
    pinMode(IO1, OUTPUT);
    pinMode(IO2, OUTPUT);
    pinMode(IO3, OUTPUT);
    pinMode(IO4, OUTPUT);
    pinMode(IO5, OUTPUT);
    pinMode(IO6, OUTPUT);
    pinMode(IO7, OUTPUT);
    
    digitalWrite(RW_CTRL, W);
    digitalWrite(WR_RST, CLOSE);
    digitalWrite(PRG_EN, CLOSE);
    
    while (1) {
        clock();
        
        if (Serial.available()) {
            b = Serial.read();
            
            if (b == 'S' || b == 's') {
                digitalWrite(PRG_EN, OPEN);
                digitalWrite(WR_RST, OPEN);
                delay(1);
                break;
            }
        }
    }
}


void loop() {
    byte     b;
    uint32_t verify_timer;
    uint32_t delay_timer;
    
    if (Serial.available()) {
        b = Serial.read();
        
        if (b == 'A' || b == 'a') {
            digitalWrite(WR_RST, CLOSE);
            digitalWrite(PRG_EN, CLOSE);
            Serial.println();
            Serial.println("Aborted!");
            while(1);
        }
    }
    
    if (PINB & _BV(1)) { // CTR_RST HIGH
        if (CounterResetState == LOW) {
            Serial.println();
            Serial.println("CLEAR");
            Counter = 0;
            Change  = true;
        }
        CounterResetState = HIGH;
    } else {
        CounterResetState = LOW;
    }
    
    if (PINB & _BV(0)) { // CTR
        CounterClockState = HIGH;
    } else {
        if (CounterClockState == HIGH && !(PINB & _BV(1))) {
            Counter++;
            Change = true;
        }
        
        CounterClockState = LOW;
    }
    
    if (Change) {
        if (Counter < 128) {
            b = 0x00;
        } else if (Counter < 256) {
            b = ROM_128_to_255[Counter - 128];
        } else if (Counter < 960) {
            b = 0x00;
        } else if (Counter < 1924) {
            b = ROM_960_to_1923[Counter - 960];
        } else if (Counter == 1924) {
            b = OPTION_BYTE;
        } else if (Counter < 2040) {
            b = 0x00;
        } else if (Counter < 2048) {
            b = ROM_2040_to_2047[Counter - 2040];
        } else {
            b = 0x00;

            if (analogRead(PROGRAMMED) < 300) {
                Serial.println();
                Serial.println("Programmed!");
                
                verify_timer = 500000;
                
                while (verify_timer--) {
                    clock();
                    
                    if (analogRead(VERIFIED) < 300) {
                        Serial.println("Verified!");
                        
                        delay_timer = 5000000;
                        while (delay_timer--) {
                            clock();
                        }
                        
                        digitalWrite(WR_RST, CLOSE);
                        digitalWrite(PRG_EN, CLOSE);
                        Serial.println("Done!");
                        while(1);
                    }
                }

                Serial.println("Verify Failed!");
                digitalWrite(WR_RST, CLOSE);
                digitalWrite(PRG_EN, CLOSE);
                while(1);
            }
        

        }
        
        PORTC &= 0xF0;
        PORTC |= b & 0x0F;
        PORTD &= 0xC3;
        PORTD |= (b>>2) & 0x3C;
        Change = false;

        if (Counter <= 2047) {
            Serial.print(Counter);
            Serial.print("=[");
            
            if (b <= 0x0F) {
                Serial.print('0');  
            }
            
            Serial.print(b, HEX);
            Serial.print("] ");
        }
    }

    clock();
}


void clock(void) {
    PORTD &= ~(1<<6);
    delayMicroseconds(1);
    PORTD |= 1<<6;
    delayMicroseconds(1);
}


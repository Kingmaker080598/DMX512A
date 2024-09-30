// LAB 8
///Venkata Siva Rama Sarma - 1002012041

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "clock.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "wait.h"
#include "eeprom.h"
#include "rgb_led.h"


#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))  //PF1
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))  //PF3
#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4)))  //PF2

#define RED_LED_MASK  2   // PF1
#define BLUE_LED_MASK 4   //pf2
#define GREEN_LED_MASK 8  // pf3

#define B_DE        (*((volatile uint32_t *)(0x42000000 + (0x400063FC-0x40000000)*32 + 6*4)))

#define B_DE_MASK  64   //PC6 -- DE PIN

#define UART4_TX_MASK 32
#define UART4_RX_MASK 16

#define DATA_MAX 512    /// max address

//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
uint16_t rampadd,pulseadd, diff, rc;
uint16_t count;
uint32_t duration;
uint16_t startValue, stopValue, firstValue, lastValue;
uint16_t PC;   /// program counter

int R =0;
int temp = 0;
bool F;
uint8_t dmx[DATA_MAX] = {0, };
char RBuffer[513]= {0, };
uint32_t N;

uint16_t Add = (uint16_t) 0;
uint16_t Address2 = (uint16_t) 1;
uint32_t Val;
uint32_t eepromData;
uint32_t deviceAdd = (uint32_t) 0;

//to indicate the maximum number of characters that can be accepted from the user and the structure for holding UI
//information we add:


#define MAX_CHARS 80
#define MAX_FIELDS 6

typedef struct _USER_DATA
{
    char buffer[MAX_CHARS + 1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;

// Initializing the Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;    /// configure TIMER1 clock
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R2;    /// configured TIMER2 Clock

    SYSCTL_RCGCGPIO_R = SYSCTL_RCGCGPIO_R5 | SYSCTL_RCGCGPIO_R2;        /// configure portf

    _delay_cycles(3);

    GPIO_PORTF_DIR_R |= GREEN_LED_MASK | RED_LED_MASK ;
    GPIO_PORTC_DIR_R |=  B_DE_MASK;
    GPIO_PORTF_DR2R_R |= GREEN_LED_MASK | RED_LED_MASK ;
    GPIO_PORTC_DR2R_R |= B_DE_MASK;
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK | RED_LED_MASK ;
    GPIO_PORTC_DEN_R |= B_DE_MASK;

    ///Initializing the EEPROM
    initEeprom();
}
///////////////////////////////////////////////////////////////TIMER////////////////////////////////////////////////////////////////////
void setbreak(void)
{
    UART4_LCRH_R |= UART_LCRH_BRK;
    TIMER1_CTL_R |= TIMER_CTL_TAEN; //timer ON

}

void enableTimer(void)
{
    // Configure Timer 1 as the time base
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;      // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;    // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R |= TIMER_TAMR_TAMR_1_SHOT;      // one-shot mode configuration
    TIMER1_TAILR_R = 3680;           // set load value to 40e6 * 0.000092 = 3680
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts

    NVIC_EN0_R = 1 << (INT_TIMER1A - 16);      // turn-on interrupt 37 (TIMER1A)
}

void enableTimer2(void)
{
    /// Configure TIMER2 as the time base
    TIMER2_CTL_R &= ~TIMER_CTL_TAEN;              // turn-off timer before reconfiguring
    TIMER2_CFG_R = TIMER_CFG_32_BIT_TIMER;        // configure as 32-bit timer (A+B)
    TIMER2_TAMR_R |= TIMER_TAMR_TAMR_1_SHOT;      // one-shot mode configuration
    TIMER2_TAILR_R = 40000;                       // set load value to 40e6 * 1millisec == 40000
    TIMER2_IMR_R = TIMER_IMR_TATOIM;              // turn-on interrupts

    NVIC_EN0_R = 1 << (INT_TIMER2A - 16);         // turn-on interrupt 39(TIMER2A)
}
/// timer ISR
void t1ISR(void)
{

    UART4_LCRH_R &= ~UART_LCRH_BRK;   ///// clear BREAK for MAB or say STOP_BRK

    waitMicrosecond(12);    // wait for 12us

    UART4_DR_R = 0x00;                ///// Send STARTCODE


    uint16_t dmxIndex = 0;


    for(dmxIndex=0;dmxIndex< N; dmxIndex ++)
    {
        while (UART4_FR_R & UART_FR_BUSY);
        setRgbColor(1023,0,0);
        UART4_DR_R = dmx[dmxIndex];
    }
    setRgbColor(0,0,0);

    TIMER1_ICR_R = TIMER_ICR_TATOCINT; // Clear timer interrupt

    if(dmxIndex >= DATA_MAX)
    {
        setbreak();
    }
}
void setbreak2() 
{
    UART4_LCRH_R |= UART_LCRH_BRK;
    TIMER2_CTL_R |= TIMER_CTL_TAEN; //timer2 ON
}
void rampData();
void pulseData();
////timer2ISR
void t2ISR()
{
    UART4_LCRH_R &= ~UART_LCRH_BRK;   ///// clear BREAK for MAB or say STOP_BRK

    waitMicrosecond(12);    // wait for 12us

    UART4_DR_R = 0x00;                ///// Send STARTCODE

    if(F == 1)
    {
        rampData();
        if(count <= rc)
        {
            setbreak2();
        }
    }

    else
    {
        pulseData();
        if(count < PC)
        {
            setbreak2();
        }


    }
    TIMER2_ICR_R = TIMER_ICR_TATOCINT; // Clear timer interrupt

}
////////////////////////////////////////////////UART4 configuration////////////////////////////////////////////////////////////////////
void initUART4()
{
    // Enable clocks
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R4;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R2;
    _delay_cycles(3);

    // Configure UART0 pins
    GPIO_PORTC_DR2R_R |= UART4_TX_MASK; // set drive strength to 2mA (not needed since default configuration -- for clarity)
    GPIO_PORTC_DEN_R |= UART4_TX_MASK | UART4_RX_MASK; // enable digital on UART0 pins
    GPIO_PORTC_AFSEL_R |= UART4_TX_MASK | UART4_RX_MASK; // use peripheral to drive PC4, PC5
    GPIO_PORTC_PCTL_R &= ~(GPIO_PCTL_PC5_M | GPIO_PCTL_PC4_M); // clear bits 0-7
    GPIO_PORTC_PCTL_R |= GPIO_PCTL_PC5_U4TX | GPIO_PCTL_PC4_U4RX;
    // select UART4 to drive pins PC4 and PC5: default, added for clarity

    // Configure UART4 to 115200 baud, 8N2 format
    UART4_CTL_R = 0;                 // turn-off UART0 to allow safe programming
    UART4_CC_R = UART_CC_CS_SYSCLK;                 // use system clock (40 MHz)
    UART4_IBRD_R = 10; // r = 40 MHz / (Nx115.2kHz), set floor(r)=21, where N=16
    UART4_FBRD_R = 0;                                  // round(fract(r)*64)=45
    UART4_IM_R = UART_IM_RXIM;
    UART4_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_STP2; // configure for 8N2 w/ 16-level FIFO
    UART4_CTL_R =  UART_CTL_UARTEN;
    // enable TX, RX, and module


    NVIC_EN1_R = 1 << (INT_UART4 -16-32);
}

void UART4_ISR(void)
{
    RED_LED =1;
    if (B_DE == 0 && UART_RIS_RXRIS)
    {
        if (UART_DR_BE & UART4_DR_R)
        {
            temp = 1;
            R = 0;
        }
        else{
            if(temp==2)

            {
                RBuffer[R] = UART4_DR_R & 0xFF;
                uint32_t x = readEeprom(Address2);
                setRgbColor(0,0,RBuffer[x]);

                R++;

            }
            else{
                if (temp==1 && UART4_DR_R & 0xFF == 0x00);
                {
                    temp = 2;
                }

            }

        }


    }

    UART4_ICR_R = (UART_ICR_RXIC);
}




void clearDataValues();
void setSingleDataValue(uint16_t add, uint8_t add2);
uint8_t getSingleDataValue(uint32_t add);



//////////// lab 4 code ------>>>>>>>>
void getsUart0(USER_DATA *data)
{
    uint8_t count = 0;

    while (true)
    {

        char inputcharacter = getcUart0();

        if ((inputcharacter == 8 || inputcharacter == 127) && count > 0)
        {
            {

                count--;
            }
            // Turn off activity LED
        }
        else if (inputcharacter == 13)
        {
            data->buffer[count] = '\0';
            // Turn on activity LED to indicate data received
            GREEN_LED = 1;
            return;
        }

        else if (inputcharacter >= 32)

        {
            data->buffer[count] = inputcharacter;
            count++;
            GREEN_LED = 1;
            // Turn on activity LED to indicate data received
        }

        if (count == MAX_CHARS)
        {
            data->buffer[count] = '\0';

            return;
        }

        GREEN_LED = 0;
    }
}

bool kbhitUart4()
{
    return !(UART4_FR_R & UART_FR_RXFE);
}

////6- parseFields()
void parseFields(USER_DATA *struct_data)
{
    uint8_t char_count = 0, fieldcount = 0, field_position_type_count = 0;
    char data1, data2;

    //Run the loop till you hit the NULL character
    while ((struct_data->buffer[char_count]) != '\0')
    {

        data1 = (struct_data->buffer[char_count]);   //Present value

        if (char_count > 0)
            data2 = (struct_data->buffer[char_count - 1]);  // Previous value

        //First Character Transition is assumed from delimiter
        if (char_count == 0)
        {
            //Checking first character is a number or not
            if ((data1 > 47 && data1 < 58))
            {
                struct_data->fieldPosition[field_position_type_count] =
                        char_count;
                struct_data->fieldType[field_position_type_count] = 'n';
                char_count = char_count + 1;
                fieldcount = fieldcount + 1;
                field_position_type_count = field_position_type_count + 1;

            }
            //Checking first character is an alphabet or not
            else if ((data1 > 64 && data1 < 91) || (data1 > 96 && data1 < 123))
            {
                struct_data->fieldPosition[field_position_type_count] =
                        char_count;
                struct_data->fieldType[field_position_type_count] = 'a';
                char_count = char_count + 1;
                fieldcount = fieldcount + 1;
                field_position_type_count = field_position_type_count + 1;
            }
            //If it is not a number or an alphabet then it is a delimiter so just increment the char count we shall replace this with "\0"
            else
                char_count = char_count + 1;

        }
        else
        {
            //Checking whether the old character is a delimiter or not and replacingit with NULL
            if (!((data2 > 47 && data2 < 58) || (data2 > 64 && data2 < 91)
                    || (data2 > 96 && data2 < 123)))
            {
                struct_data->buffer[char_count - 1] = '\0';

                //Recording the Transition type
                if ((data1 > 47 && data1 < 58))
                {
                    struct_data->fieldPosition[field_position_type_count] =
                            char_count;
                    struct_data->fieldType[field_position_type_count] = 'n';
                    char_count = char_count + 1;
                    fieldcount = fieldcount + 1;
                    field_position_type_count = field_position_type_count + 1;

                }
                else if ((data1 > 64 && data1 < 91)
                        || (data1 > 96 && data1 < 123))
                {
                    struct_data->fieldPosition[field_position_type_count] =
                            char_count;
                    struct_data->fieldType[field_position_type_count] = 'a';
                    char_count = char_count + 1;
                    fieldcount = fieldcount + 1;
                    field_position_type_count = field_position_type_count + 1;
                }
                else
                {
                    //Come back here whether making NULL is required or not
                    //struct_data->buffer[char_count] = '\0';
                    char_count = char_count + 1;
                }
            }
            else
                char_count = char_count + 1;

        }

    }

    if (fieldcount >= MAX_FIELDS)
        struct_data->fieldCount = MAX_FIELDS;
    else
        struct_data->fieldCount = fieldcount;

}

///7 - getFieldString()

char* getFieldString(USER_DATA *data, uint8_t fieldNumber)
{
    if (fieldNumber <= data->fieldCount)
        return &(data->buffer[data->fieldPosition[fieldNumber]]);
    else
        return '\0';   // returns NULL
}

/// 8 - int32_t getFieldInteger()

int32_t getFieldInteger(USER_DATA *data, uint8_t fieldNumber)
{

    return atoi(&data->buffer[data->fieldPosition[fieldNumber]]);

}

////9 - bool isCommand()

bool isCommand(USER_DATA *data, const char strCommand[], uint8_t minArguments)
{
    if (data->fieldCount == 0)
    {
        // There are no fields, return false
        return false;
    }

    const char *commandField = getFieldString(data, 0);
    int i = 0;

    while (strCommand[i] != '\0' && commandField[i] != '\0')
    {
        if (strCommand[i] != commandField[i])
        {
            return false; // Characters don't match, return false
        }
        i++;
    }

    // Check if the first field matches the requested command
    if (strCommand[i] == '\0' && commandField[i] == '\0')
    {
        uint8_t numArgs = data->fieldCount - 1;
        if (numArgs >= minArguments)
        {
            // The number of arguments (excluding the command) is greater than or equal to the requested minimum
            return true;
        }
    }

    return false;
}

////EEPROM --------->>>>>>>

void Configcommand(USER_DATA *data)
{

    if (isCommand(data, "device", 1))
    {
        //        mode = DEVICE_MODE;
        // uint32_t dAdd = (uint32_t) 0;

        uint32_t dAdd = getFieldInteger(data, 1);
        if (dAdd > 512 || dAdd < 1) 
        {
            putsUart0("Invalid device address. Using default address (1-512).\n");
            dAdd = 1;
        }
        B_DE = 0;
        UART4_CTL_R &= ~ (UART_CTL_TXE);
        UART4_CTL_R |= UART_CTL_RXE;
        Val=0;
        writeEeprom(0, 0); // Store the device mode in EEPROM
        writeEeprom(Address2, dAdd); // Store the device address in EEPROM
        char response[50];
        snprintf(response, sizeof(response), "Device mode configured with address %d\n", dAdd);
        putsUart0(response);
    } else {
        putsUart0("Invalid command. Available commands: 'controller', 'device [ADD]'\n");
    }
}

void loadfromEeprom(void)
{
    uint32_t eepromData = (uint32_t) readEeprom(Add);

    if (eepromData == 1)
    {
        B_DE = 1; // Controller mode
        UART4_CTL_R &= ~(UART_CTL_RXE);
        UART4_CTL_R |= UART_CTL_TXE;
        Add = 0;
        Val = 1;
        writeEeprom(0, 1);
        putsUart0(" controller mode\n");
    }
    else if (eepromData == 0)
    {
        B_DE = 0; // Receiving by default
        UART4_CTL_R &= ~(UART_CTL_TXE);
        UART4_CTL_R |= UART_CTL_RXE;
        Val = 0;
        Add = 0;
        writeEeprom(0, 1);
        Add++;
        writeEeprom(Address2, deviceAdd);
        putsUart0("Device mode \n");
    }
}


int main(void)

{

    initHw();
    enableTimer();
    enableTimer2();

    initUart0();
    initRgb(); ///for led using pwm

    setUart0BaudRate(115200, 40e6);
    N = DATA_MAX;
    USER_DATA data;


    initUART4();

    loadfromEeprom();


    int controllerMode = 0;

    while (1) {
        if (kbhitUart0() == 1) {
            getsUart0(&data);
            parseFields(&data);

            // Command evaluation
            if (isCommand(&data, "controller", 0) || B_DE == 1)
            {
                B_DE = 1;
                UART4_CTL_R &= ~(UART_CTL_RXE);
                UART4_CTL_R |= UART_CTL_TXE;
                Val = 1;

                writeEeprom(0, 1);
                //                          putsUart0("Configured--> controller mode \n");
                controllerMode = 1;
            }
            if (controllerMode == 1)
            {
                if (isCommand(&data, "clear", 0))
                {
                    clearDataValues();
                    putsUart0("Data is Cleared\n");
                }
                else if (isCommand(&data, "get", 1))
                {   setRgbColor(0,1023,0);
                    uint32_t address = getFieldInteger(&data, 1);
                    getSingleDataValue(address);
                    setRgbColor(0,0,0);
                }
                else if (isCommand(&data, "set", 2))
                {   setRgbColor(0,1023,0);
                    int32_t add = getFieldInteger(&data, 1);
                    int32_t add2 = getFieldInteger(&data, 2);
                    setSingleDataValue(add, add2);
                    putsUart0("Data is set \n");
                    setRgbColor(0,0,0);
                }
                else if (isCommand(&data, "max", 1))
                {
                    uint16_t new = getFieldInteger(&data, 1);
                    if (new > 0) {
                        N = new; // Update the maximum address
                        char response[30];
                        snprintf(response, sizeof(response), "Max address set to: %d\n", N);
                        putsUart0(response);
                    }
                    else {
                        putsUart0("Invalid maximum address\n");
                    }
                }
                else if (isCommand(&data, "on", 0))
                {

                    setbreak();
                    putsUart0("DMX streaming \n");
                }
                else if (isCommand(&data, "off", 0))
                {
                    TIMER1_CTL_R &= ~(TIMER_CTL_TAEN); // turn-off timer before reconfiguring
                    TIMER2_CTL_R &= ~(TIMER_CTL_TAEN); // turnof timer2
                    putsUart0("DMX not streaming \n");
                }
                else if (isCommand(&data, "device", 1))
                {
                    Configcommand(&data);
                }

                else if (isCommand(&data, "ramp", 5))
                {
                    F=1;
                    count = 0;
                    rampadd = getFieldInteger(&data,1);
                    duration = getFieldInteger(&data, 2);
                    startValue = getFieldInteger(&data, 3);
                    stopValue = getFieldInteger(&data, 4);
                    rc = getFieldInteger(&data, 5);
                    int minDiff = 1;  // Adjust this value based on your requirements
                    diff = (stopValue - startValue) / duration;
                    diff = (diff < minDiff) ? minDiff : diff;                    rc = rc - 1;
                    setbreak2();

                    putsUart0("Ramping command RxD \n");
                }

                else if (isCommand(&data,"pulse", 5))
                {
                    F= 0;
                    count = 0;
                    PC = 0;
                    pulseadd =  getFieldInteger(&data, 1);
                    duration =  getFieldInteger(&data, 2);
                    firstValue = getFieldInteger(&data, 3);
                    lastValue =  getFieldInteger(&data, 4);
                    PC = getFieldInteger(&data, 5);
                    duration = duration*1000;    //// *1000 for waitMicrosecond function  conversion of time to micro
                    dmx[pulseadd] = firstValue;
                    setbreak2();
                    putsUart0("Pulse command received \n");

                }
                //   else if (isCommand(&data, "checkEeprom", 0))
                //   {
                //       CheckEEPROMCommand();
                //   }


            }
        }
    }
}

///////>>>>>>>>>COMMANDS .......

void clearDataValues()
{
    int i;
    for (i = 0; i < DATA_MAX; i++)
    {
        dmx[i] = 0;
    }
}

void setSingleDataValue(uint16_t add, uint8_t add2)
{
    if (add <= DATA_MAX)
    {
        dmx[add] = add2;
    }
}

uint8_t getSingleDataValue(uint32_t add)
{
    if ( add <= DATA_MAX)
    {
        char R[20];
        snprintf(R, sizeof(R), "Value: %d \n", dmx[add]);

        putsUart0(R);
        return dmx[add];

    }
    else
    {
        putsUart0("Address not in Range\n");

        return 0;  // Return 0 if the address is out of range
    }
}

void rampData()
{
    uint16_t dmxIndex = 0;

    dmx[rampadd] += diff;

    for(dmxIndex=0;dmxIndex< N; dmxIndex ++)
    {
        while (UART4_FR_R & UART_FR_BUSY);
        setRgbColor(0,dmx[rampadd],0);
        UART4_DR_R = dmx[dmxIndex];
    }
    if(dmx[rampadd] == stopValue)
    {
        dmx[rampadd] = startValue;
        count++;
    }

}
void pulseData()
{
    uint16_t dmxIndex = 0;

    for (dmxIndex = 0; dmxIndex < N; dmxIndex++)
    {
        while (UART4_FR_R & UART_FR_BUSY);
        setRgbColor(dmx[pulseadd], 0, 0);
        UART4_DR_R = dmx[dmxIndex];
    }

    waitMicrosecond(duration);
    if (dmx[pulseadd] == firstValue)
    {
        dmx[pulseadd] = lastValue;  // Use loop as an index to update different elements
    }
    else
    {
        dmx[pulseadd] = firstValue; // Use loop as an index to update different elements
        count++;
    }

}


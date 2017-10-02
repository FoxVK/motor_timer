/**
 *  @target PIC12F1572
 * 
 *  datasheet: http://ww1.microchip.com/downloads/en/DeviceDoc/40001723D.pdf
 */

#include <xc.h>

// CONFIG1
#pragma config FOSC = INTOSC    //  (INTOSC oscillator; I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOREN = OFF    // Low Power Brown-out Reset enable bit (LPBOR is disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

// AD channels are sligthly different - consult it with DS
#define PIN_TIME_INPUT      2u
#define PIN_THROTLE_INPUT   4u
#define PIN_MOTOR_OUTPUT    5u
#define PIN_START_BTN_INPUT 0u

#define THRUST_RAMP_DOWN (0xffffu / 2000) ///< 100%->0% per 2000ms
#define BTN_DELAY        (500)            ///< [ms] time to ingore button after press detection

#define FOSC 8000uL  ///< sys freq [kHz]]


#if   (PIN_MOTOR_OUTPUT == 0)
#define PWM_USED 2u
#elif (PIN_MOTOR_OUTPUT == 1)
#define PWM_USED 1u
#elif (PIN_MOTOR_OUTPUT == 2)
#define PWM_USED 3u
#elif (PIN_MOTOR_OUTPUT == 4)
#define PWM_USED 2u
#elif (PIN_MOTOR_OUTPUT == 5)
#define PWM_USED 1u
#else
#error "PWM can be set only on pin 0,1,4,5"
#endif

typedef enum {true=1, false=0}Bool;

/** System clock intin
 * 
 *  See datasheet chaper 5.5 
 */
void sysclk_init(void)
{
    OSCCONbits.IRCF = 0xEu; //8MHz 0xF=16MHz
    
    while(!OSCSTATbits.HFIOFL)
    {
        //wait until oscilator become stable 
    }
}

void tick_init(void)
{
    /*  INput is sysclk / 4
     *  desired intervfal is 1ms
     *  set prescaler to 16 (0b10) and perido to 125
     * 
     */
    T2CONbits.T2CKPS = 0b10; // prescaler /16
    PR2 = 125;
    PIR1bits.TMR2IF = 0;
    T2CONbits.TMR2ON = 1;
}

void tick_wait(void)
{
    while(!PIR1bits.TMR2IF)
    {
        //wait
    }
    PIR1bits.TMR2IF = 0u;
}

void adc_init(void)
{
    TRISA  |= (1u << (PIN_THROTLE_INPUT)); //input
    ANSELA |= (1u << (PIN_THROTLE_INPUT)); //analog
    
    TRISA  |= (1u << (PIN_TIME_INPUT)); //input
    ANSELA |= (1u << (PIN_TIME_INPUT)); //analog
    
#define TAD (FOSC/32) //4us
    ADCON1bits.ADCS = 0b010u; //fosc /32
    ADCON0bits.ADON = 1u; //ADC module ON
    
}

void pwm_init()
{
    TRISA  &= ~(1u << (PIN_MOTOR_OUTPUT)); //output
    ANSELA &= ~(1u << (PIN_MOTOR_OUTPUT)); //digital
    
#if   (PIN_MOTOR_OUTPUT == 4)
    APFCON0bits.P2SEL = 1;

#elif (PIN_MOTOR_OUTPUT == 5)
    APFCON0bits.P1SEL = 1;
#endif
    
    
#if (PWM_USED == 1)
    PWM1PH = 0u;
    PWM1DC = 0u;
    PWM1PR = 255u;
    PWM1CONbits.EN = 1;
    PWM1CONbits.OE = 1;
#elif (PWM_USED == 2)
    PWM2PH = 0u;
    PWM2DC = 0u;
    PWM2PR = 255u;
    PWM2CONbits.EN = 1;
    PWM2CONbits.OE = 1;
#elif (PWM_USED == 3)
    PWM3PH = 0u;
    PWM3DC = 0u;
    PWM3PR = 255u;
    PWM3CONbits.EN = 1;
    PWM3CONbits.OE = 1;
#endif
    
}

void start_btn_init(void)
{
    TRISA  |=  (1u << (PIN_START_BTN_INPUT)); //input
    ANSELA &= ~(1u << (PIN_START_BTN_INPUT)); //digital
    INLVLA |=  (1u << (PIN_START_BTN_INPUT)); // shmit trigger enabled
    WPUA   |=  (1u << (PIN_START_BTN_INPUT)); // weak pull-up enabled
    
}


void adc_start_conv(char channel)
{
    ADCON0bits.CHS = channel;
    NOP(); NOP();NOP(); NOP();NOP(); NOP();NOP(); NOP();NOP(); NOP(); //wait 10us
    NOP(); NOP();NOP(); NOP();NOP(); NOP();NOP(); NOP();NOP(); NOP(); //wait 10us
    ADCON0bits.GO = 1u;
}

char adc_get_result(void)
{
    while(ADCON0bits.ADGO)
    {
        //wait for end of conversion
    }
    return ADRESH;
}

/** Is btn pressed?
 * 
 */
Bool get_btn(void)
{
    return (LATA & (1 << PIN_START_BTN_INPUT)) != 0u;
}

unsigned big_thrust = 0; //thrust multiplied by 255

void set_thrust(char value)
{
    char pwm = value;
    big_thrust = value;
#if (PWM_USED == 1)
    PWM1DCH = 0u;
    PWM1DCL = pwm;
#elif (PWM_USED == 2)
    PWM2DCH = 0u;
    PWM2DCL = pwm;
#elif (PWM_USED == 3)
    PWM3DCH = 0u;
    PWM3DCL = pwm;
#else
#error "No PWM used specified"
#endif
}

typedef unsigned long Iir_type;
typedef unsigned long Thrust_time;

Thrust_time thrust_timeout = 0;

struct MesurementData{
    Iir_type throtle_sum;
    Iir_type time_sum;
}mes_data;

static const char IIR_SHIFT = 10;

void iir_feed(char new_val, Iir_type * sum)
{
    Iir_type decrement = (*sum >> IIR_SHIFT);
    *sum += (Iir_type) new_val;
    *sum -= decrement;
}
char iir_get(Iir_type * sum)
{
    return (char)(*sum >> IIR_SHIFT);
}

void measure(void)
{
    adc_start_conv(PIN_THROTLE_INPUT);
    char throtle = adc_get_result();
    
    adc_start_conv(PIN_TIME_INPUT);
    
    iir_feed(throtle, &mes_data.throtle_sum);
    
    char time = adc_get_result();
    
    iir_feed(time, &mes_data.time_sum);
}

typedef unsigned long Sys_tick;

Sys_tick sys_time = 0;


/*states*/

typedef enum {
    ST_READY_BTN_WAIT,
    ST_READY,
    ST_THRUST_BTN_WAIT,
    ST_THRUST,
    ST_THRUST_DOWN
}State;

State state = 0;
Bool state_changed;

void state_set(State new_state)
{
    state = new_state;
}

void go2ready()
{
    set_thrust(0u);
    state_set(ST_READY_BTN_WAIT);
}

void st_ready_btn_wait(void)
{
    static unsigned timeout;
    
    if(state_changed)
    {
        timeout = BTN_DELAY;
    }
    
    if(timeout>0)
    {
        timeout--;
        measure();
    }
    else
    {
        state_set(ST_READY);
    }

}

void st_ready(void)
{
    if(get_btn())
    {
        char thrust = iir_get(&mes_data.throtle_sum);
        Thrust_time time = iir_get(&mes_data.time_sum);
        
        thrust_timeout = 15000UL + (((Thrust_time)time)<<9);
        
        set_thrust(thrust);
        state_set(ST_THRUST_BTN_WAIT);
    }
    else
    {
        measure();
    }
}

void st_thrust_btn_wait(void)
{
    static unsigned timeout = 0u;
    if(state_changed)
    {
        timeout = BTN_DELAY;
    }
    
    if(timeout>0)
    {
        timeout--;
        if(thrust_timeout > 0)
            thrust_timeout--;
        else
            state_set(ST_THRUST_DOWN);
    }
    else
        state_set(ST_THRUST);
}

void st_thrust(void)
{
    if(thrust_timeout > 0)
        thrust_timeout--;
    else
        go2ready();
    
    if(get_btn())
        go2ready();
}

void st_thrust_down(void)
{
    if(get_btn())  //emergency stop
        go2ready();
    
    if(big_thrust > THRUST_RAMP_DOWN)
    {
        big_thrust -= THRUST_RAMP_DOWN;
        set_thrust(big_thrust >> 8);
    }
    else
    {
        go2ready();
    }
}

/*/states*/

void main(void)
{
    
    sysclk_init();
    adc_init();
    pwm_init();
    start_btn_init();
    tick_init();
        
    
    while(1)
    { //loop
        State old = state;
        switch(state)
        {
            case ST_READY: 
                st_ready(); 
                break;
            case ST_READY_BTN_WAIT: 
                st_ready_btn_wait(); 
                break;
            case ST_THRUST: 
                st_thrust(); 
                break;
            case ST_THRUST_BTN_WAIT: 
                st_thrust_btn_wait(); 
                break;
            case ST_THRUST_DOWN: 
                st_thrust_down(); 
                break;
        }
        state_changed = (old != state);
        
        tick_wait();
        sys_time++;
    }
}
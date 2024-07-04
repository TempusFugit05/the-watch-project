#ifndef MAX30102_CONFIGS_H
#define MAX30102_CONFIGS_H

#define MAX30102_I2C_ADDR   0x57

/*Interupt bit masks*/
#define POWER_READY_INTR_MASK   (1 << 0) // Power ready after brownout
#define DIE_TEMP_READY_MASK     (1 << 1) // Sensor temperature ready
#define ALC_OVERFLOW_INTR_MASK  (1 << 5) // Ambiant light buffer overflow
#define PPG_READY_INTR_MASK     (1 << 6) // SPO2 ready
#define ALMOST_FULL_INTR_MASK   (1 << 7) // Almost-full interrupt bit

/*Interrupt registers*/
#define INTERRUPT_STATUS_REG_1  0x00 // Holds almost-full, ppg-ready, alc-overflow and power-ready interrupt status
#define INTERRUPT_STATUS_REG_2  0x01 // Holds die-temperature-ready interrupt status

#define INTERRUPT_ENABLE_REG_1  0x02 // Enables almost-full, ppg-ready, alc-overflow and power-ready interrupts
#define INTERRUPT_ENABLE_REG_2  0x03 // Enables die-temperature-ready interrupt
/*--------------------------------------------------------------------------------*/


/*FIFO registers*/
#define WRITE_PTR_REG       0x04 // This register points to the FIFO address currently being written to 
#define FIFO_OVERFLOW_REG   0x05 // Number of samples lost after FIFO buffer overflow
#define READ_PTR_REG        0x06 // FIFO address to read data from (does not auto-incrament)
#define FIFO_DATA_REG       0x07 // Actual sample data is stored here (FIFO buffer register)
/*--------------------------------------------------------------------------------*/

/*Config registers*/
#define FIFO_CFG_REG            0x08 // Configs sample averaging, FIFO rollover mode and number of samples written for almost-full interrupt to trigger

/*Number of empty data slots in FIFO when interrupt is activated*/
#define ALMOST_FULL_CFG_MIN     0
#define ALMOST_FULL_CFG_MAX     15

/*Enable/disable FIFO writing over older data when buffer is full*/
typedef enum
{
    FIFO_ROLLOVER_ENABLE = (1 << 4),
    FIFO_ROLLOVER_DISABLE = (0 << 4)
} fifo_rollover_t;

/*Number of samples to average*/
typedef enum
{
    FIFO_AVG_SAMPLE_1  = (0 << 5),
    FIFO_AVG_SAMPLE_2  = (1 << 5),
    FIFO_AVG_SAMPLE_4  = (2 << 5),
    FIFO_AVG_SAMPLE_8  = (3 << 5),
    FIFO_AVG_SAMPLE_16 = (4 << 5),
    FIFO_AVG_SAMPLE_32 = (5 << 5),
    
} sample_average_t;

/*--------------------------------------------------------------------------------*/

/*Mode config*/
#define MODE_CFG_REG            0x09 // Configs power off mode, power reset and led mode (dual/single led)

#define SHUTDOWN_ENABLE_MASK    (1 << 7)
#define SHUTDOWN_DISABLE_MASK   (0 << 7)

#define RESET_MASK              (1 << 6) // (Auto resets to 0 after reset)

typedef enum
{
    RESET_ENABLE  = (1 << 6),
    RESET_DISABLE = (0 << 6),
} reset_enable_t;

typedef enum
{
    LED_HR_ONLY_MODE   = (0b010 << 2),
    LED_SPO2_ONLY_MODE = (0b011 << 2),
    LED_MULTI_LED_MODE = (0b111 << 2),
} led_mode_t;
/*--------------------------------------------------------------------------------*/

/*LED power mode*/

#define LED_1_AMPLITUDE_CFG_REG 0x0C // Configs led1 power level
#define LED_2_AMPLITUDE_CFG_REG 0x0D // Configs led2 power level

#define AMPLITUDE_MIN           0
#define AMPLITUDE_MAX           0xFF

#define LED_CONTROL_REG_1       0x11 // Enables led slots 1 and 2 
#define LED_CONTROL_REG_2       0x12 // Enables led slots 3 and 4
/*--------------------------------------------------------------------------------*/
#endif // MAX30102_CONFIGS_H
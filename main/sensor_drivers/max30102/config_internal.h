#ifndef CONFIG_INTERNAL_H
#define CONFIG_INTERNAL_H

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


/*FIFO config*/
#define FIFO_AVG_SAMPLE_MASK(sample_size)   (sample_size << 5)

#define FIFO_CFG_REG            0x08 // Configs sample averaging, FIFO rollover mode and number of samples written for almost-full interrupt to trigger

#define FIFO_ROLLOVER_ENABLE_MASK   (1 << 4)
#define FIFO_ROLLOVER_DISABLE_MASK  (0 << 4)

#define ALMOST_FULL_CFG_MIN     0
#define ALMOST_FULL_CFG_MAX     15
/*--------------------------------------------------------------------------------*/


/*Mode config*/
#define MODE_CFG_REG            0x09 // Configs power off mode, power reset and led mode (dual/single led)

#define SHUTDOWN_ENABLE_MASK    (1 << 7)
#define SHUTDOWN_DISABLE_MASK   (0 << 7)

#define RESET_MASK          (1 << 6) // (Auto resets to 0 after reset)

#define RESET_DISABLE_MASK  (0 << 6)
#define RESET_ENABLE_MASK   (1 << 6)

#define LED_HR_ONLY_MODE_MASK   0b010
#define LED_SPO2_ONLY_MODE_MASK 0b011
#define LED_MULTI_LED_MODE_MASK 0b111
/*--------------------------------------------------------------------------------*/


/*SpO2 config*/
#define SPO2_CFG_REG    0x0A

#define LED_ADC_RESOLUTION_MASK(resolution) (resolution)
#define SPO2_SAMPLE_RATE_MASK(frequency)    (frequency << 3)
#define SPO2_RANGE_MASK(range)              (range << 6)
/*--------------------------------------------------------------------------------*/


/*LED config*/
#define LED_1_AMPLITUDE_CFG_REG 0x0C // Configs led1 power level
#define LED_2_AMPLITUDE_CFG_REG 0x0D // Configs led2 power level

#define LED_CONTROL_REG_1       0x11 // Enables led slots 1 and 2 
#define LED_CONTROL_REG_2       0x12 // Enables led slots 3 and 4

#define RED_LED_SLOT_MASK   (0b001)
#define IR_LED_SLOT_MASK    (0b010)

#endif // CONFIG_INTERNAL_H

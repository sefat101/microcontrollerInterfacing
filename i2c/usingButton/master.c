#include "stm32f446xx.h"

#define SLAVE_ADDR_WRITE 0x50 // 0x28 shifted left by 1 for Write mode

// Rough delay function for 16 MHz clock
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < (ms * 1600); i++);
}

void I2C1_Init(void) {
    // 1. Enable Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // Enable GPIOB clock
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;  // Enable I2C1 clock

    // 2. Configure PB8 (SCL) and PB9 (SDA) for Alternate Function
    GPIOB->MODER &= ~((3 << 16) | (3 << 18)); // Clear mode for PB8, PB9
    GPIOB->MODER |= ((2 << 16) | (2 << 18));  // Set to Alternate Function mode (10)
    
    GPIOB->OTYPER |= ((1 << 8) | (1 << 9));   // Set to Open Drain
    GPIOB->OSPEEDR |= ((3 << 16) | (3 << 18));// Very High Speed
    GPIOB->PUPDR &= ~((3 << 16) | (3 << 18)); // No internal pull-up (using external)

    // Set Alternate Function 4 (I2C1) for PB8 and PB9
    GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4)); // Clear AFR bits
    GPIOB->AFR[1] |= ((4 << 0) | (4 << 4));      // Set AF4 (0100)

    // 3. Configure I2C1 Peripheral
    I2C1->CR1 |= I2C_CR1_SWRST;   // Reset I2C
    I2C1->CR1 &= ~I2C_CR1_SWRST;  // Release reset

    // Clock config for 100 kHz Standard Mode based on 16 MHz APB1 clock
    I2C1->CR2 = 16;               // APB1 freq is 16 MHz
    I2C1->CCR = 80;               // Thigh = Tlow = 5000ns. 5000ns / 62.5ns = 80
    I2C1->TRISE = 17;             // Max rise time 1000ns. (1000ns / 62.5ns) + 1 = 17

    I2C1->CR1 |= I2C_CR1_PE;      // Enable I2C1
}

void Button_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // Enable GPIOC clock
    GPIOC->MODER &= ~(3 << 26);          // PC13 as Input (00)
}

void I2C_SendCmd(uint8_t command) {
    // Wait until bus is not busy
    while(I2C1->SR2 & I2C_SR2_BUSY);

    // Generate START condition
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB)); // Wait for Start Bit flag

    // Send Slave Address + Write bit
    I2C1->DR = SLAVE_ADDR_WRITE;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); // Wait for Address matched flag
    (void)I2C1->SR1; (void)I2C1->SR2;   // Clear ADDR flag by reading SR1 then SR2

    // Send Command Data
    I2C1->DR = command;
    while(!(I2C1->SR1 & I2C_SR1_TXE));  // Wait for Tx buffer empty

    // Generate STOP condition
    I2C1->CR1 |= I2C_CR1_STOP;
}

int main(void) {
    I2C1_Init();
    Button_Init();

    uint8_t toggle_state = 0; // 0 = stopped, 1 = blinking

    while (1) {
        // Check if button is pressed (active low)
        if (!(GPIOC->IDR & (1 << 13))) {
            delay_ms(50); // Debounce
            
            if (!(GPIOC->IDR & (1 << 13))) { // Still pressed
                
                toggle_state ^= 1; // Flip between 0 and 1
                
                if (toggle_state) {
                    I2C_SendCmd(0x01); // Send Start command
                } else {
                    I2C_SendCmd(0x02); // Send Stop command
                }

                // Wait for button release
                while (!(GPIOC->IDR & (1 << 13))); 
                delay_ms(50); // Debounce release
            }
        }
    }
}

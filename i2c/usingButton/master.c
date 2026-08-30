#include "stm32f446xx.h"

// 0x28 is the 7-bit address. Shifted left by 1 for I2C Write operation (0x50)
#define SLAVE_ADDR_WRITE 0x50 

// Simple delay function assuming default 16 MHz internal clock
void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < (ms * 1600); i++);
}

void I2C1_Init(void) {
    // 1. Enable clocks for GPIOB and I2C1
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; 
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;  

    // 2. Configure PB8 (SCL) and PB9 (SDA)
    // Clear mode bits, then set to Alternate Function mode (10)
    GPIOB->MODER &= ~((3 << 16) | (3 << 18)); 
    GPIOB->MODER |= ((2 << 16) | (2 << 18));  
    
    // Set Output Type to Open-Drain
    GPIOB->OTYPER |= ((1 << 8) | (1 << 9));   
    
    // Set to Very High Speed
    GPIOB->OSPEEDR |= ((3 << 16) | (3 << 18));
    
    // Disable internal pull-ups (must use external 4.7k resistors)
    GPIOB->PUPDR &= ~((3 << 16) | (3 << 18)); 

    // Set Alternate Function 4 (AF4 is I2C1 for PB8/PB9)
    GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4)); 
    GPIOB->AFR[1] |= ((4 << 0) | (4 << 4));      

    // 3. Configure I2C1 Peripheral
    I2C1->CR1 |= I2C_CR1_SWRST;   // Force software reset
    I2C1->CR1 &= ~I2C_CR1_SWRST;  // Release reset

    // Clock timing for 100 kHz Standard Mode (APB1 is 16 MHz by default)
    I2C1->CR2 = 16;               
    I2C1->CCR = 80;               
    I2C1->TRISE = 17;             

    // Enable I2C1 peripheral
    I2C1->CR1 |= I2C_CR1_PE;      
}

void Button_Init(void) {
    // Enable clock for GPIOC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; 
    // Set PC13 (Blue Button) as Input (00)
    GPIOC->MODER &= ~(3 << 26);          
}

void I2C_SendCmd(uint8_t command) {
    // Wait until the I2C bus is free
    while(I2C1->SR2 & I2C_SR2_BUSY);

    // 1. Generate START condition
    I2C1->CR1 |= I2C_CR1_START;
    while(!(I2C1->SR1 & I2C_SR1_SB)); // Wait for Start Bit flag

    // 2. Send Slave Address + Write bit
    I2C1->DR = SLAVE_ADDR_WRITE;
    while(!(I2C1->SR1 & I2C_SR1_ADDR)); // Wait for Address Matched flag
    
    // Clear ADDR flag by reading SR1 followed by SR2
    (void)I2C1->SR1; 
    (void)I2C1->SR2;   

    // 3. Send Command Data Byte
    I2C1->DR = command;
    while(!(I2C1->SR1 & I2C_SR1_TXE));  // Wait until Transmit Data Register is Empty

    // 4. Generate STOP condition
    I2C1->CR1 |= I2C_CR1_STOP;
}

int main(void) {
    I2C1_Init();
    Button_Init();

    uint8_t toggle_state = 0; // Tracks if we are currently commanding ON or OFF

    while (1) {
        // Check if button is pressed (Nucleo button is active LOW)
        if (!(GPIOC->IDR & (1 << 13))) {
            
            delay_ms(50); // Software debounce
            
            // Confirm it's still pressed
            if (!(GPIOC->IDR & (1 << 13))) { 
                
                toggle_state ^= 1; // Flip state between 1 and 0
                
                if (toggle_state == 1) {
                    I2C_SendCmd(0x01); // Send START command
                } else {
                    I2C_SendCmd(0x02); // Send STOP command
                }

                // Wait in a loop until the physical button is released
                while (!(GPIOC->IDR & (1 << 13))); 
                delay_ms(50); // Debounce the release
            }
        }
    }
}

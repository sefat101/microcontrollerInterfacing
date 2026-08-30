#include "stm32f446xx.h"

#define SLAVE_OWN_ADDR 0x50 // 0x28 shifted left by 1

volatile uint8_t blink_active = 0;

void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < (ms * 1600); i++);
}

void LED_Init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; // Enable GPIOA clock
    GPIOA->MODER &= ~(3 << 10);          // Clear mode for PA5
    GPIOA->MODER |= (1 << 10);           // Set PA5 to General Purpose Output (01)
}

void I2C1_Slave_Init(void) {
    // 1. Enable Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; 
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;  

    // 2. Configure PB8 (SCL) and PB9 (SDA)
    GPIOB->MODER &= ~((3 << 16) | (3 << 18)); 
    GPIOB->MODER |= ((2 << 16) | (2 << 18));  
    GPIOB->OTYPER |= ((1 << 8) | (1 << 9));   
    GPIOB->OSPEEDR |= ((3 << 16) | (3 << 18));
    GPIOB->PUPDR &= ~((3 << 16) | (3 << 18)); 
    GPIOB->AFR[1] &= ~((0xF << 0) | (0xF << 4)); 
    GPIOB->AFR[1] |= ((4 << 0) | (4 << 4));      

    // 3. Configure I2C1 Peripheral for Slave Mode
    I2C1->CR1 |= I2C_CR1_SWRST;   
    I2C1->CR1 &= ~I2C_CR1_SWRST;  

    I2C1->CR2 = 16; 
    I2C1->CCR = 80; 
    I2C1->TRISE = 17; 

    // Set Slave Address. Bit 14 must always be kept at 1 by software in OAR1.
    I2C1->OAR1 = (1 << 14) | SLAVE_OWN_ADDR;

    // Enable I2C Event Interrupts and Buffer Interrupts
    I2C1->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN;
    NVIC_EnableIRQ(I2C1_EV_IRQn); // Enable IRQ 31 in the Cortex-M4 NVIC

    I2C1->CR1 |= I2C_CR1_ACK; // Enable Acknowledgement
    I2C1->CR1 |= I2C_CR1_PE;  // Enable I2C1
}

// Bare Metal Interrupt Handler for I2C1 Events
void I2C1_EV_IRQHandler(void) {
    uint16_t sr1 = I2C1->SR1;

    // 1. Address Matched flag
    if (sr1 & I2C_SR1_ADDR) {
        // Clear ADDR flag by reading SR1 (done above) then SR2
        (void)I2C1->SR2; 
    }
    
    // 2. Data Register Not Empty flag (Data received)
    if (sr1 & I2C_SR1_RXNE) {
        uint8_t received_cmd = I2C1->DR; // Reading DR clears RXNE flag
        
        if (received_cmd == 0x01) {
            blink_active = 1; // Start
        } 
        else if (received_cmd == 0x02) {
            blink_active = 0; // Stop
        }
    }
    
    // 3. Stop condition detected flag
    if (sr1 & I2C_SR1_STOPF) {
        // Clear STOPF by reading SR1 (done above), then writing to CR1
        I2C1->CR1 |= I2C_CR1_PE; 
    }
}

int main(void) {
    LED_Init();
    I2C1_Slave_Init();

    while (1) {
        if (blink_active) {
            GPIOA->ODR ^= (1 << 5); // Toggle PA5 using Output Data Register
            delay_ms(200); 
        } else {
            GPIOA->BSRR = (1 << (5 + 16)); // Use Bit Set/Reset Register to turn off PA5 instantly
        }
    }
}

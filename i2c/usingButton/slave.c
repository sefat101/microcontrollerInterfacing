#include "stm32f446xx.h"

// 0x28 is the 7-bit address. Shifted left by 1.
#define SLAVE_OWN_ADDR 0x50 

volatile uint8_t blink_active = 0;

void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < (ms * 1600); i++);
}

void LED_Init(void) {
    // Enable clock for GPIOA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; 
    
    // Set PA5 (Green LED LD2) to General Purpose Output (01)
    GPIOA->MODER &= ~(3 << 10);          
    GPIOA->MODER |= (1 << 10);           
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

    // 3. Configure I2C1 Peripheral
    I2C1->CR1 |= I2C_CR1_SWRST;   
    I2C1->CR1 &= ~I2C_CR1_SWRST;  

    I2C1->CR2 = 16; 
    I2C1->CCR = 80; 
    I2C1->TRISE = 17; 

    // 4. Set Slave Address
    // Bit 14 MUST always be kept at 1 by software in OAR1 register
    I2C1->OAR1 = (1 << 14) | SLAVE_OWN_ADDR;

    // 5. Enable Interrupts
    // Enable I2C Event Interrupts and Buffer Interrupts
    I2C1->CR2 |= I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN;
    
    // Enable IRQ 31 in the Cortex-M4 NVIC (Nested Vectored Interrupt Controller)
    NVIC_EnableIRQ(I2C1_EV_IRQn); 

    // 6. Enable I2C peripheral and Acknowledgement
    I2C1->CR1 |= I2C_CR1_ACK; 
    I2C1->CR1 |= I2C_CR1_PE;  
}

// -------------------------------------------------------------
// HARDWARE INTERRUPT HANDLER FOR I2C1
// This runs automatically whenever the master sends data
// -------------------------------------------------------------
void I2C1_EV_IRQHandler(void) {
    uint16_t sr1 = I2C1->SR1;

    // A) Check if our Address matched
    if (sr1 & I2C_SR1_ADDR) {
        // Clear ADDR flag by reading SR1 (done above), then SR2
        (void)I2C1->SR2; 
    }
    
    // B) Check if Data Register has received a byte
    if (sr1 & I2C_SR1_RXNE) {
        uint8_t received_cmd = I2C1->DR; // Reading DR clears the RXNE flag
        
        if (received_cmd == 0x01) {
            blink_active = 1; // Master said START
        } 
        else if (received_cmd == 0x02) {
            blink_active = 0; // Master said STOP
        }
    }
    
    // C) Check if Master sent a STOP condition
    if (sr1 & I2C_SR1_STOPF) {
        // Clear STOPF flag by reading SR1 (done above), then writing to CR1
        I2C1->CR1 |= I2C_CR1_PE; 
    }
}

int main(void) {
    LED_Init();
    I2C1_Slave_Init();

    while (1) {
        if (blink_active == 1) {
            // Toggle PA5 using the Output Data Register
            GPIOA->ODR ^= (1 << 5); 
            delay_ms(200); 
        } else {
            // Use Bit Set/Reset Register to instantly turn off PA5
            // Shifting by 21 (5 + 16) targets the reset bit for Pin 5
            GPIOA->BSRR = (1 << 21); 
        }
    }
}

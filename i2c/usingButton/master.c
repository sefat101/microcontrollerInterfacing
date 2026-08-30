#include "stm32f4xx_hal.h"

#define SLAVE_ADDR 0x28 << 1 // 7-bit address shifted left for HAL

I2C_HandleTypeDef hi2c1;

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_I2C1_Init(void);

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();

    uint8_t command = 0x01; // Command to toggle LED

    while (1) {
        // Check if the blue button (PC13) is pressed (active low on Nucleo boards)
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_RESET) {
            
            // Send the command byte to the slave
            HAL_I2C_Master_Transmit(&hi2c1, SLAVE_ADDR, &command, 1, HAL_MAX_DELAY);
            
            // Simple debounce delay
            HAL_Delay(300); 
        }
    }
}

void MX_I2C1_Init(void) {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure PB8 (SCL) and PB9 (SDA) for I2C1
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;      // Open-Drain is required for I2C
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // External pull-ups should be used
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;              // 100 kHz Standard Mode
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    HAL_I2C_Init(&hi2c1);
}

void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Configure PC13 for the Blue User Button
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

// Basic clock initialization (adjust if using a specific external crystal)
void SystemClock_Config(void) {
    // Default clock config provided by HAL for 16MHz internal oscillator
}

#include <stdint.h>
#include "stm32f4xx.h" // Automatically includes stm32f446xx.h based on project settings

// A simple blocking delay function
void delay(volatile uint32_t count) {
    while(count--) {
        __NOP(); // No Operation - prevents the compiler from optimizing the loop away
    }
}

int main(void) {
    // 1. Enable the clock for GPIO Port A
    // The RCC (Reset and Clock Control) AHB1ENR register controls clocks for GPIO ports.
    // Bit 0 corresponds to GPIOA.
    RCC->AHB1ENR |= (1 << 0);

    // 2. Configure PA5 as a General Purpose Output
    // The MODER register controls pin modes (2 bits per pin). PA5 uses bits 10 and 11.
    // First, clear bits 10 and 11 for PA5
    GPIOA->MODER &= ~(3U << (5 * 2)); 
    // Then, set bit 10 to '1' to configure it as an output (01 = Output)
    GPIOA->MODER |= (1U << (5 * 2));

    // Infinite loop
    while (1) {
        // 3. Toggle the LED
        // The ODR (Output Data Register) holds the output states of the pins.
        // XORing bit 5 flips the state of PA5.
        GPIOA->ODR ^= (1 << 5);

        // 4. Wait for a short period
        // Note: At the default 16 MHz HSI clock, 1,000,000 iterations is roughly a few hundred milliseconds.
        delay(1000000);
    }
}

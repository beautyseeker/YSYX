#include "arch/SoC.h"

void gpio_write(GPIO_OFFSET_T reg, uint32_t val) {
    *(volatile uint32_t *)(GPIO_BASE + reg) = val;
}

uint32_t gpio_read(GPIO_OFFSET_T reg) {
    return *(volatile uint32_t *)(GPIO_BASE + reg);
}

void gpio_init(void) {
    gpio_write(GPIO_LED, 0);
    gpio_write(GPIO_SEGS, 0);
    if ((gpio_read(GPIO_LED) == 0) && (gpio_read(GPIO_SEGS) == 0)) {
        // GPIO数码管显示O000字符
        gpio_write(GPIO_SEGS, 0x00);
    } else {
        // GPIO数码管显示ERRO字符
        gpio_write(GPIO_SEGS, 0x79);
    }
}

uint32_t hex2dec_seg_val(uint32_t hex) {
    uint32_t seg_val = 0;
    for (int i = 0; i < 8; i++) {
      seg_val |= (hex % 10) << (4 * i);
      hex /= 10;
    }
    return seg_val;
  }


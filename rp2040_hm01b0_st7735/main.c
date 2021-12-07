#include <stdio.h>
#include "pico/stdlib.h"
#include "arducam/arducam.h"
#include "lib/st7735.h"
#include "lib/fonts.h"
uint8_t image_buf[324*324];
uint8_t displayBuf[80*160*2];
uint8_t header[2] = {0x55,0xAA};

int main() {
	stdio_uart_init();
	//printf("\n\nBooted!\n");
	gpio_init(PIN_LED);
	gpio_set_dir(PIN_LED, GPIO_OUT);

	ST7735_Init();
	ST7735_DrawImage(0, 0, 80, 160, arducam_logo);

	struct arducam_config config;
	config.sccb = i2c0;
	config.sccb_mode = I2C_MODE_16_8;
	config.sensor_address = 0x24;
	config.pin_sioc = PIN_CAM_SIOC;
	config.pin_siod = PIN_CAM_SIOD;
	config.pin_resetb = PIN_CAM_RESETB;
	config.pin_xclk = PIN_CAM_XCLK;
	config.pin_vsync = PIN_CAM_VSYNC;
	config.pin_y2_pio_base = PIN_CAM_Y2_PIO_BASE;

	config.pio = pio0;
	config.pio_sm = 0;

	config.dma_channel = 0;
	config.image_buf = image_buf;
	config.image_buf_size = sizeof(image_buf);

	arducam_init(&config);
	while (true) {
	  gpio_put(PIN_LED, !gpio_get(PIN_LED));
	  arducam_capture_frame(&config);

	  uint16_t index = 0;
	  for (int y = 0; y < 160; y++) {
	    for (int x = 0; x < 80; x++) {
              uint8_t c = image_buf[(2+320-2*y)*324+(2+40+2*x)];
              uint16_t imageRGB   = ST7735_COLOR565(c, c, c);
              displayBuf[index++] = (uint8_t)(imageRGB >> 8) & 0xFF;
              displayBuf[index++] = (uint8_t)(imageRGB)&0xFF;
            }
	  }
	  ST7735_DrawImage(0, 0, 80, 160, displayBuf);
	}

	return 0;
}

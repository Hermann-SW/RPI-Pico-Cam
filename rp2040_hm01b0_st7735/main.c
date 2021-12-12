#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include <tusb.h>
#include "pico/multicore.h"
#include "arducam/arducam.h"
#include "lib/st7735.h"
#include "lib/fonts.h"
uint8_t image_buf[324*324];
uint8_t displayBuf[80*160*2];
uint8_t header[2] = {0x55,0xAA};

int frames=0, freq=250000, t0, t1;

void core1_entry() {
        int frame = 0;

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
          ++frame;
          if (frame==3)  t0=time_us_32();
          else if (frame-frames==3) {
            t1=time_us_32();
            printf("%.2ffps\n", 1000000.0*frames/(t1-t0));
          }

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
}

#include "hardware/vreg.h"

#define L 8
unsigned char str[L+1];

unsigned char *readLine() {
  int u; unsigned char *p;
  for(p=str, u=getchar_timeout_us(100000); u!='\r' && p-str<L; u=getchar_timeout_us(10000))  if (u>=0)  putchar(*p++=u);
  *p = 0;  putchar('\n'); return str;
}

int main() {
  int loops=20;
  stdio_init_all();
  while (!tud_cdc_connected()) { sleep_ms(100); if (--loops==0) break;  }

  printf("tud_cdc_connected(%d)\n", tud_cdc_connected()?1:0);

  if (tud_cdc_connected()) {
    getchar_timeout_us(100000);     // without 1st atoi(readLine()) returns 0
 
    printf("clock speed [KHz]: ");            freq=atoi(readLine());
    printf("fps frames: ");                   frames=atoi(readLine());
  }

  vreg_set_voltage(VREG_VOLTAGE_1_30);
  sleep_ms(1000);
  set_sys_clock_khz(freq, true);

  multicore_launch_core1(core1_entry);

  while (1)
    tight_loop_contents();
}

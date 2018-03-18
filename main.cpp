#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "oled.h"

#define SPI_DC 0 // D/C on OLED; Command - LOW, DATA - HIGH
#define SPI_SS 4 // CS on OLED; Active - LOW
#define SPI_MOSI 5 // DIN on OLED
#define SPI_SCK 7 // CLK on OLED

enum WORDTYPE 
{
	COMMAND,
	DATA	
};

#define ASCII(ch) (ch) - 'A' + 10
#define ASCII_DOT 36
#define ASCII_SEMICOLON 37
#define ASCII_SPACE 38
#define ASCII_PERCENT 39

static uint8_t chars_to_ascii[41][6] =
{
	{0x00,0x7C,0x8A,0x92,0xA2,0x7C},        // 0
	{0x00,0x00,0x42,0xFE,0x02,0x00},        // 1
	{0x00,0x46,0x8A,0x92,0x92,0x62},        // 2
	{0x00,0x44,0x92,0x92,0x92,0x6C},        // 3
	{0x00,0x18,0x28,0x48,0xFE,0x08},        // 4
	{0x00,0xF4,0x92,0x92,0x92,0x8C},        // 5
	{0x00,0x3C,0x52,0x92,0x92,0x0C},        // 6
	{0x00,0x80,0x8E,0x90,0xA0,0xC0},        // 7
	{0x00,0x6C,0x92,0x92,0x92,0x6C},        // 8
	{0x00,0x60,0x92,0x92,0x94,0x78},        // 9
	{0x00,0x7E,0x88,0x88,0x88,0x7E},        // A
	{0x00,0xFE,0x92,0x92,0x92,0x6C},        // B
	{0x00,0x7C,0x82,0x82,0x82,0x44},        // C
	{0x00,0xFE,0x82,0x82,0x82,0x7C},        // D
	{0x00,0xFE,0x92,0x92,0x92,0x82},        // E
	{0x00,0xFE,0x90,0x90,0x90,0x80},        // F
	{0x00,0x7C,0x82,0x92,0x92,0x5E},        // G
	{0x00,0xFE,0x10,0x10,0x10,0xFE},        // H
	{0x00,0x00,0x82,0xFE,0x82,0x00},        // I
	{0x00,0x0C,0x02,0x02,0x02,0xFC},        // J
	{0x00,0xFE,0x10,0x28,0x44,0x82},        // K
	{0x00,0xFE,0x02,0x02,0x02,0x02},        // L
	{0x00,0xFE,0x40,0x20,0x40,0xFE},        // M
	{0x00,0xFE,0x40,0x20,0x10,0xFE},        // N
	{0x00,0x7C,0x82,0x82,0x82,0x7C},        // O
	{0x00,0xFE,0x90,0x90,0x90,0x60},        // P
	{0x00,0x7C,0x82,0x8A,0x84,0x7A},        // Q
	{0x00,0xFE,0x90,0x90,0x98,0x66},        // R
	{0x00,0x64,0x92,0x92,0x92,0x4C},        // S
	{0x00,0x80,0x80,0xFE,0x80,0x80},        // T
	{0x00,0xFC,0x02,0x02,0x02,0xFC},        // U
	{0x00,0xF8,0x04,0x02,0x04,0xF8},        // V
	{0x00,0xFC,0x02,0x3C,0x02,0xFC},        // W
	{0x00,0xC6,0x28,0x10,0x28,0xC6},        // X
	{0x00,0xE0,0x10,0x0E,0x10,0xE0},        // Y
	{0x00,0x8E,0x92,0xA2,0xC2,0x00},        // Z
	{0x00,0x00,0x06,0x06,0x00,0x00},        // . - 36
	{0x00,0x00,0x36,0x36,0x00,0x00},        // : - 37
	{0x00,0x00,0x00,0x00,0x00,0x00},        //   - 38
	{0x00,0xC6,0xC8,0x10,0x26,0xC6}	        // % - 39
};

void spi_init(void)
{
	DDRB = (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS) | (1 << SPI_DC);
	PORTB |= (1 << SPI_MOSI) | (1 << SPI_SCK) | (1 << SPI_SS);

	SPCR = (1 << SPE) // Enable SPI
		| (0 << DORD) // MSB first
		| (1 << MSTR) // Master
		| (0 << CPOL) // SPI mode 0
		| (0 << CPHA) //
		| (0 << SPR1)    //
		| (0 << SPR0);   // CLK = Fosc / 2
	SPSR = (1 << SPI2X); //
}

void oled_write_byte(uint8_t data, WORDTYPE wordtype)
{
	// set type of data
	if (wordtype == COMMAND)
		PORTB &= ~(1 << SPI_DC);
	else
		PORTB |= (1 << SPI_DC);

	// start transfer
	PORTB &= ~(1 << SPI_SS);

	SPDR = data;

	_delay_ms(10);

	// end transfer
	PORTB |= (1 << SPI_SS);
}

void oled_prepare_draw_box(int x1, int x2, int y1, int y2)
{
	oled_write_byte(SSD1306_COLUMNADDR, COMMAND);
	oled_write_byte(x1, COMMAND);
	oled_write_byte(x2, COMMAND);

	oled_write_byte(SSD1306_PAGEADDR, COMMAND);
	oled_write_byte(y1, COMMAND);
	oled_write_byte(y2, COMMAND);
}

void oled_draw_text(uint8_t * chars_indexes, int length, int x, int y)
{
	oled_prepare_draw_box(x, x + length * 6, y, y);
	for (int i = 0; i < length; ++i)
	{
		for (int j = 0; j < 6; ++j)
		oled_write_byte(chars_to_ascii[chars_indexes[i]][j], DATA);
	}
}

void oled_init(void)
{
	oled_write_byte(SSD1306_DISPLAYOFF, COMMAND);                    // 0xAE

	oled_write_byte(SSD1306_SETDISPLAYCLOCKDIV, COMMAND);            // 0xD5
	oled_write_byte(0x80, COMMAND);                                  // the suggested ratio 0x80

	oled_write_byte(SSD1306_CHARGEPUMP, COMMAND);                    // 0x8D
	oled_write_byte(0x14, COMMAND);                                  // Enable charge pump

	oled_write_byte(SSD1306_MEMORYMODE, COMMAND);                    // 0x20
	oled_write_byte(0x00, COMMAND);                                  // horizontal

	oled_write_byte(SSD1306_SEGREMAP | 0x1, COMMAND);                // orientation
	oled_write_byte(SSD1306_COMSCANINC, COMMAND);                    //

	oled_write_byte(SSD1306_SETCOMPINS, COMMAND);                    // 0xDA
	oled_write_byte(0x12, COMMAND);                                  // ?????? depends on OLED device

	oled_write_byte(SSD1306_SETCONTRAST, COMMAND);                   // 0x81
	oled_write_byte(0xCF, COMMAND);

	oled_write_byte(SSD1306_SETPRECHARGE, COMMAND);                  // 0xd9
	oled_write_byte(0xF1, COMMAND);                                  // good i guess

	oled_write_byte(SSD1306_SETVCOMDETECT, COMMAND);                 // 0xDB
	oled_write_byte(0x40, COMMAND);                                  // ????? weird

	oled_write_byte(SSD1306_DISPLAYALLON_RESUME, COMMAND);           // 0xA4
	oled_write_byte(SSD1306_DISPLAYON, COMMAND);                     // turn on OLED panel
	
	oled_prepare_draw_box(0, 127, 0, 7);                             // clear display
	for(int i = 0; i < 8 * 128; i++)
		oled_write_byte(0x0, DATA);
}

void oled_draw_template(void)
{
	uint8_t chars_indexes[] =
	{
		ASCII('I'), ASCII('U'), 4, ASCII_SPACE,
		ASCII('L'), ASCII('I'), ASCII('M'), ASCII('I'), ASCII('T'), ASCII('E'), ASCII('D'), ASCII_SPACE,
		ASCII('E'), ASCII('D'), ASCII('I'), ASCII('T'), ASCII('I'), ASCII('O'), ASCII('N')
	};
	
	oled_draw_text(chars_indexes, sizeof(chars_indexes), 3, 6);
}

void oled_draw_resourses(void)
{
	uint8_t cpu_chars_indexes[] =
	{
		ASCII('C'), ASCII('P'), ASCII('U'), ASCII_SEMICOLON, ASCII_SPACE, ASCII_SPACE, ASCII_SPACE, ASCII_PERCENT
	};
	
	oled_draw_text(cpu_chars_indexes, sizeof(cpu_chars_indexes), 25, 4);

	uint8_t ram_chars_indexes[] =
	{
		ASCII('R'), ASCII('A'), ASCII('M'), ASCII_SEMICOLON, ASCII_SPACE, ASCII_SPACE, ASCII_SPACE, ASCII_PERCENT
	};
	
	oled_draw_text(ram_chars_indexes, sizeof(ram_chars_indexes), 25, 3);
	
	uint8_t network_chars_indexes[] =
	{
		ASCII('N'), ASCII('E'), ASCII('T'), ASCII_SEMICOLON, ASCII_SPACE, ASCII_SPACE, ASCII_DOT, ASCII_SPACE, ASCII_SPACE, ASCII('M'), ASCII('B'), ASCII('I'), ASCII('T')
	};
	oled_draw_text(network_chars_indexes, sizeof(network_chars_indexes), 25, 2);
}

void oled_draw_copyrite(void)
{
	uint8_t chars_indexes[] =
	{
		ASCII('B'), ASCII('Y'), ASCII_SPACE,
		ASCII('P'), ASCII('A'), ASCII('V'), ASCII('E'), ASCII('L'), ASCII_SPACE,
		ASCII('G'), ASCII('O'), ASCII('L'), ASCII('U'), ASCII('B'), ASCII('E'), ASCII('V')
	};
	
	oled_draw_text(chars_indexes, sizeof(chars_indexes), 3, 0);
}

void oled_update_resourses(uint8_t cpu, uint8_t ram, double network)
{
	uint8_t cpu_digits[] = {cpu / 10, cpu % 10};
	oled_draw_text(cpu_digits, sizeof(cpu_digits), 55, 4);
	
	uint8_t ram_digits[] = {ram / 10, ram % 10};
	oled_draw_text(ram_digits, sizeof(ram_digits), 55, 3);
	
	uint8_t network_digits[] = {(int)network % 10, ASCII_DOT, (int)(network * 10) % 10, (int)(network * 100) % 10};
	oled_draw_text(network_digits, sizeof(network_digits), 55, 2);
}

int main(void)
{
	spi_init();
	oled_init();
	oled_draw_template();
	oled_draw_resourses();
	oled_draw_copyrite();
	srand(time(NULL));

    while (1)
    {
		_delay_ms(10000);
		oled_update_resourses(rand() % 100, rand() % 100, (rand() % 1000) / double(100));
    }
}


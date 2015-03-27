
#include <stdint.h>
#include <8052.h>

#define LCD_PORT 	P1
#define LCD_RS		P3_2
#define LCD_EN		P3_3
#define _SWITCH		P3_4

#define  __baudRate_calc_timer_1(__freq,__baud) (256 - ((__freq/384) / __baud))

__code char search_cmd[25]   = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x08,0x04,0x01,0x00,0x00,0x00,0x0A,0x00,0x18};
__code char gen_img[20]   = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x01,0x00,0x05};
__code char img_2_tz[20]   = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x01,0x00,0x08};
__code char img_2_tz_2[20]  = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x04,0x02,0x02,0x00,0x09};
__code char gen_temp[20]  = {0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x03,0x05,0x00,0x09};

volatile char rx_buff[32];
volatile uint8_t index = 0;

__code char *nameDb[] ={	"Person1",
							"Person2",
							"Person3",
							"Person4",
							"Person5",
							"Person6",
							"Person7"
						};

void _delay_ms(unsigned int msec)
{
uint16_t i,j;
for(i=0;i<msec;i++)
{
for(j=0;j<1275;j++);
}
}

void lcd_cmd(uint8_t cmd)
{
	LCD_RS = 0;
	LCD_PORT = cmd;
	LCD_EN = 1;
	_delay_ms(1);
	LCD_EN = 0;
}

void lcd_dat(uint8_t dat)
{
	LCD_RS = 1;
	LCD_PORT = dat;
	LCD_EN = 1;
	_delay_ms(1);
	LCD_EN = 0;
}

void lcd_puts(char * str)
{
	uint8_t i = 0;
	while(str[i] != '\0')
	{
		lcd_dat(str[i]);
		i++;
	}
}

void lcd_init(void)
{
	LCD_PORT = 0x00;
	LCD_RS = 0;
	_delay_ms(10);

	///////////// Reset process from datasheet /////////
  lcd_cmd(0x30);
	_delay_ms(50);
  lcd_cmd(0x30);
	_delay_ms(110);
  lcd_cmd(0x30);

  /////////////////////////////////////////////////////
  lcd_cmd(0x38);    //function set
  lcd_cmd(0x0C);    //display on,cursor off,blink off
  lcd_cmd(0x01);    //clear display
  lcd_cmd(0x06);    //entry mode, set increment
}

void uart_init(uint32_t OscillatorFrequency,uint32_t baudRate)
{
	volatile unsigned int autoReloadvalue;
	autoReloadvalue =  __baudRate_calc_timer_1(OscillatorFrequency,baudRate);
	TMOD  |= 0x20;
	SCON  |= 0x50;
	TL1    = autoReloadvalue >> 8;
	TH1    = autoReloadvalue;
	TR1    = 1;
	ES = 1;
	EA = 1;
}

void uart_putc(char _byte)
{
	EA = 0;
	SBUF = _byte;
	while(!TI);
	TI   = 0;
	EA = 1;
}

void uart_puts(char * str)
{
	uint8_t i = 0;
	while(str[i] != '\0')
	{
		uart_putc(str[i]);
		i++;
	}
}

void uart_irq(void) __interrupt (4)
{
	uint8_t dat;
	EA = 0;
	RI = 0;
	dat = SBUF;
	rx_buff[index++] = dat;
	EA = 1;
}

void tx_packet( char *_data,uint8_t len)
{
	uint8_t k;
	len++;
	for(k=0;k<len;k++)
	{
		uart_putc(_data[k]);
	}
    
}


int main(void)
{
	uint8_t _flag = 0;
	P0 = 0xFF;
	_SWITCH = 1;
	lcd_init();
	uart_init(11059200, 9600);
	lcd_puts("  FPS System  ");
	while(1)
	{
		if(_SWITCH == 0)
		{
			lcd_cmd(0xC0);
			lcd_puts("Button Press");
			index = 0;
			tx_packet(gen_img, 11);
			_delay_ms(100);

			if(index >= 11)
			{
				if(rx_buff[9] == 0x00)
				{
					_flag = 1;
					lcd_cmd(0xC0);
					lcd_puts("Colc Success ");
				}
				else if(rx_buff[9] == 0x01)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Error Recvng ");
				}
				else if(rx_buff[9] == 0x02)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Fing det fail ");
				}
				else if(rx_buff[9] == 0x03)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Collect Fail ");
				}
			}

			if(_flag == 1) 
			{
				index = 0;
				_flag = 0;
				tx_packet(img_2_tz, 12);
				_delay_ms(100);

				if(index >= 11)
				{
				if(rx_buff[9] == 0x00)
				{
					_flag = 1;
					lcd_cmd(0xC0);
					lcd_puts("Gen Success ");
				}
				else if(rx_buff[9] == 0x01)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Error Recvng ");
				}
				else if(rx_buff[9] == 0x06)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Dis image    ");
				}
				else if(rx_buff[9] == 0x07)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("smalness err  ");
				}					
				else if(rx_buff[9] == 0x15)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Valid Fail    ");
				}
				}
			}

			if(_flag == 1) 
			{
				index = 0;
				_flag = 0;

				tx_packet(gen_img, 11);
				_delay_ms(100);

				if(index >= 11)
				{
					if(rx_buff[9] == 0x00)
					{
						_flag = 1;
						lcd_cmd(0xC0);
						lcd_puts("Colc Success ");
					}
					else if(rx_buff[9] == 0x01)
					{
						_flag = 0;
						lcd_cmd(0xC0);
						lcd_puts("Error Recvng ");
					}
					else if(rx_buff[9] == 0x02)
					{
						_flag = 0;
						lcd_cmd(0xC0);
						lcd_puts("Fing det fail ");
					}
					else if(rx_buff[9] == 0x03)
					{
						_flag = 0;
						lcd_cmd(0xC0);
						lcd_puts("Collect Fail ");
					}
				}
			}			

			if(_flag == 1) 
			{
				index = 0;
				_flag = 0;
				tx_packet(img_2_tz_2, 12);
				_delay_ms(100);

				if(index >= 11)
				{
				if(rx_buff[9] == 0x00)
				{
					_flag = 1;
					lcd_cmd(0xC0);
					lcd_puts("Gen Success ");
				}
				else if(rx_buff[9] == 0x01)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Error Recvng ");
				}
				else if(rx_buff[9] == 0x06)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Dis image    ");
				}
				else if(rx_buff[9] == 0x07)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("smalness err  ");
				}					
				else if(rx_buff[9] == 0x15)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Valid Fail    ");
				}
				}
			}

			if(_flag == 1) 
			{
				index = 0;
				_flag = 0;
				tx_packet(gen_temp, 11);
				_delay_ms(100);

				if(index >= 11)
				{
				if(rx_buff[9] == 0x00)
				{
					_flag = 1;
					lcd_cmd(0xC0);
					lcd_puts("Opr Success ");
				}
				else if(rx_buff[9] == 0x01)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Error Recvng ");
				}
				else if(rx_buff[9] == 0x0A)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Fail To Combine ");
				}
				}
			}

			if(_flag == 1) 
			{
				index = 0;
				_flag = 0;
				tx_packet(search_cmd, 16);
				_delay_ms(100);

				if(index >= 15)
				{
				if(rx_buff[9] == 0x00)
				{
					_flag = 1;
					lcd_cmd(0xC0);
					lcd_puts("Match found  ");
				}
				else if(rx_buff[9] == 0x01)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("Error Recvng ");
				}
				else if(rx_buff[9] == 0x09)
				{
					_flag = 0;
					lcd_cmd(0xC0);
					lcd_puts("No matching   ");
				}
				}
			}
			_delay_ms(100);
			if(_flag == 1)
			{
				lcd_cmd(0xC0);
				lcd_puts("                   ");
				lcd_cmd(0xC0);
				lcd_puts(nameDb[rx_buff[11]]);

				// Key Pad part has to add here
				// Active Low
				P0 = 0x00;

			}
		}
	}
	return 0;
}
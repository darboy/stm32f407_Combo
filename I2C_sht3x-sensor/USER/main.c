#include "stm32f4xx.h"
#include "usart.h"
#include "delay.h"
#include <stdint.h>
void LED_D2_D3(uint32_t);

//STM32F4����ģ��-�⺯���汾
//�Ա����̣�http://mcudev.taobao.com
#include "sht.h"
int main(void)
{
	
	u32 t=0;
	uart_init(115200);
	delay_init(84);
	//LED_D2_D3();
	//LED_D2_D3(GPIO_Pin_7);
  while (sht_probe() != STATUS_OK) {
	  LED_D2_D3(GPIO_Pin_6);
        /* printf("SHT sensor probing failed\n"); */
    }
	LED_D2_D3(GPIO_Pin_7);
    /*printf("SHT sensor probing successful\n"); */

    while (1) {
        s32 temperature, humidity;
        /* Measure temperature and relative humidity and store into variables
         * temperature, humidity (each output multiplied by 1000).
         */
        s8 ret = sht_measure_blocking_read(&temperature, &humidity);
        if (ret == STATUS_OK) {
            /* printf("measured temperature: %0.2f degreeCelsius, "
                      "measured humidity: %0.2f percentRH\n",
                      temperature / 1000.0f,
                      humidity / 1000.0f); */
			LED_D2_D3(GPIO_Pin_6);
        } else {
            //printf("error reading measurement\n");
			LED_D2_D3(GPIO_Pin_7);
        }

        delay_ms(1000);
    }
}

void LED_D2_D3(uint32_t GPIO_Pin_x)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	GPIO_InitTypeDef GPIOInit;
	GPIOInit.GPIO_Mode = GPIO_Mode_OUT;
	GPIOInit.GPIO_OType = GPIO_OType_PP;
	GPIOInit.GPIO_Pin = GPIO_Pin_x;
	GPIOInit.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIOInit.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIOInit);
}




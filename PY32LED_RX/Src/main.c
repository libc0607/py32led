/*
	PY32LED Receiver demo
	HW: V1.0, RGB, COMP, 2K(0x800) RAM, 16K-128B(0x3F80) FLASH, 128B EEPROM @0x80003F80
	Github/OSHWHub: @libc0607
	
	This is an early demo to prove the idea of broadcasting message to wireless powered LEDs.
	Codes are copied mainly from Puya SDK (PY32F0xx_Firmware_V1.1.3).
	However, at least it works -- but undoubtly dirty. 
	For more details about this project, please see my Github. 

	To zh-cn users：
	开源不等于白嫖，请遵守开源协议（见工程主页）。
	最终解释权归原作者所有。
	总之，希望您能一起为开源社区做点贡献，谢谢茄子。
*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "py32f003xx_ll_Start_Kit.h"

#pragma pack(1)

/* Private define ------------------------------------------------------------*/
//#define DEV_BOARD 	// PA2 conflict in my dev board
/* Private variables ---------------------------------------------------------*/
__IO uint32_t frame_recv, bit_counter, prev_bit_ts, new_bit_ts;
__IO uint32_t update_flag, comp_last_state; 
__IO uint32_t ts, target_fade_time;
__IO uint64_t start_fade_ts;
__IO uint8_t target_r, target_g, target_b, is_rgb;
__IO uint8_t last_r, last_g, last_b;
__IO uint8_t led_addr;
__IO eeprom_conf_t conf;
/* Private user code ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void APP_SystemClockConfig(void);
static void APP_CompInit(void);
static void APP_ConfigTIM1Count(void);
static void APP_ConfigPWMChannel(void);
static void APP_ConfigTIM3Base(void);
static void APP_SetRGBValue(uint8_t r, uint8_t g, uint8_t b);
static uint64_t microsec();
static uint64_t milisec();
static void process_dataframe(uint32_t dat);
static void APP_ConfigErase(void);
static void APP_ConfigProgram(void);
static void load_default_config_to_ram();
static void write_ram_config_to_eeprom();
static int32_t load_eeprom();

int main(void)
{
	new_bit_ts = 0;
	prev_bit_ts = 0;
	bit_counter = 0;
	frame_recv = 0;
	ts = 0;
	update_flag = 0; 
	led_addr = 0;

	uint32_t sum = 0;
	uint64_t ts_ms_cur;
	uint32_t i;
	
	HAL_Init();
	
	LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM1);
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM3);
  APP_SystemClockConfig();
  BSP_LED_Init(LED_GREEN);
  BSP_USART_Config();	
	load_eeprom();

	printf("=========================\r\n");
	printf("PY32LED Demo V0.1\r\n");
	printf("Github/OSHWHub: @libc0607\r\n");
	printf("LED Address: %d\r\n", conf.led_addr);
	printf("=========================\r\n");

  APP_CompInit();
  APP_ConfigTIM1Count();
	NVIC_SetPriority(ADC_COMP_IRQn, 0);
  NVIC_EnableIRQ(ADC_COMP_IRQn);
	NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0);
  NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);

	LL_COMP_Enable(COMP1);
	__IO uint32_t wait_loop_index = 0;
  wait_loop_index = (LL_COMP_DELAY_VOLTAGE_SCALER_STAB_US * (SystemCoreClock / (1000000 * 2)));
  while(wait_loop_index != 0) {
    wait_loop_index--;
  }

	LL_TIM_EnableIT_UPDATE(TIM1);
  LL_TIM_EnableCounter(TIM1);
	
	APP_ConfigTIM3Base();
	APP_ConfigPWMChannel();
  
  while (1) {   
		if (update_flag == 1) {
			update_flag = 0; 
			//printf("r,%d,%d,%d\r\n",  new_bit_ts, prev_bit_ts, new_bit_ts- prev_bit_ts);
			if ( (new_bit_ts - prev_bit_ts > PULSE_UPPER_US) ||
					 (new_bit_ts - prev_bit_ts < PULSE_LOWER_US) ) {
				bit_counter = 0; 
				frame_recv = 0;							 
			} else if (new_bit_ts - prev_bit_ts > PULSE_THRESH_US) {
				bit_counter++;
				frame_recv |= 0x1<<(FRAME_LEN_BIT-bit_counter);
			} else {
				bit_counter++;
			}
		
			if (bit_counter == FRAME_LEN_BIT) {
				sum = 0;
				bit_counter = 0;
				for (uint32_t i=0; i<FRAME_LEN_BIT; i++) {
					sum += ((frame_recv & (0x1<<i))? 1: 0);
				}
				if ((sum & 0x1) != 0) {
				  printf("recv: %x, invalid parity, ignore\r\n", frame_recv);
				} else { 
					printf("recv: %x, parity good\r\n", frame_recv);
					process_dataframe(frame_recv);
				}
			}
		} 
		
		// 2. TO-DO: led fading 
  }
}

int32_t load_eeprom()
{
	//printf("EEPROM: \r\n");
	for (uint32_t i=0; i<EEPROM_SIZE; i++) {
		((uint8_t*)&conf)[i] = HW8_REG(EEPROM_START_ADDR +(i));
		//printf("%2x ", HW8_REG(EEPROM_START_ADDR +(i)));
		//if ((i+1)%16 == 0) {
		//	printf("\r\n");
		//}
	}
	//printf("EEPROM header: %2x %2x %2x %2x. addr: %d\r\n", conf.header1, conf.header2, conf.header3, conf.header4, conf.led_addr);
	if ( (conf.header1 == CONF_HEADER_1) && (conf.header2 == CONF_HEADER_2) &&
			 (conf.header3 == CONF_HEADER_3) && (conf.header4 == CONF_HEADER_4) &&
			 (conf.ver == CONF_VER) 
	) {
		printf("EEPROM config found. \r\n");
		return 0;
	} else {
		load_default_config_to_ram();
		printf("No config found. \r\n");
		return -1;
	}
	
}

void load_default_config_to_ram()
{
	conf.header1 = CONF_HEADER_1;
	conf.header2 = CONF_HEADER_2;
	conf.header3 = CONF_HEADER_3;
	conf.header4 = CONF_HEADER_4;
	conf.ver = CONF_VER;
	conf.len = CONF_LEN;
	conf.led_addr = CONF_DEF_ADDR;
	conf.led_addr_inv = ~CONF_DEF_ADDR;
	conf.led_color_default = CONF_DEF_COLOR;
	conf.led_color_default_inv = ~CONF_DEF_COLOR;
	conf.led_fade_timer_default = CONF_DEF_FADE_TIME;
	conf.led_fade_timer_default_inv = ~CONF_DEF_FADE_TIME;
}

void write_ram_config_to_eeprom()
{
	//printf("write_ram_config_to_eeprom() \r\n");
	HAL_FLASH_Unlock();
	APP_ConfigErase();
	APP_ConfigProgram();
	HAL_FLASH_Lock();
	
	//printf("EEPROM: \r\n");
	//for (uint32_t i=0; i<EEPROM_SIZE; i++) {
	//	printf("%2x ", HW8_REG(EEPROM_START_ADDR +(i)));
	//	if ((i+1)%16 == 0) {
	//		printf("\r\n");
	//	}
	//}
}

void process_dataframe(uint32_t dat)
{
	uint8_t cmd, addr, addr_new; 
	uint64_t ts_tmp;
	
	cmd = (dat&CMD_MASK)>>CMD_SHIFT;
	addr = (dat&ADDR_MASK)>>ADDR_SHIFT;
	ts_tmp = microsec();
	
	if ((conf.led_addr != addr) && (addr != ADDR_BCAST)) {
		printf("[%llu]the packet is not for us\r\n", ts_tmp);
		return; 
	}
	
	switch (cmd) {
		case CMD_SET_COLOR:
			target_r = (dat&RED_MASK)>>RED_SHIFT;
			target_g = (dat&GREEN_MASK)>>GREEN_SHIFT;
			target_b = (dat&BLUE_MASK)>>BLUE_SHIFT;
			start_fade_ts = ts_tmp;	// TO-DO
			printf("[%llu]cmd set_color, %d,%d,%d\r\n", ts_tmp, target_r, target_g, target_b);
			
		  // will be replaced by some fade logic
			APP_SetRGBValue(target_r, target_g, target_b);

		break;
		/*case CMD_SET_TIME:
			start_fade_ts = ts_tmp; 
			target_fade_time = ((dat&FADE_TIME_MASK)>>FADE_TIME_SHIFT)*1000;
			is_rgb = (dat&RGB_HSV_MASK)>>RGB_HSV_SHIFT;
			printf("[%llu]cmd set_time, %d,%d\r\n", ts_tmp, is_rgb, target_fade_time);
		break;*/
		case CMD_SET_ADDR:
			if ((((dat&NEW_ADDR_1_MASK)>>NEW_ADDR_1_SHIFT) != ((dat&NEW_ADDR_2_MASK)>>NEW_ADDR_2_SHIFT)) ||
					(((dat&NEW_ADDR_1_MASK)>>NEW_ADDR_1_SHIFT) != (((~dat)&NEW_ADDR_COMP_MASK)>>NEW_ADDR_COMP_SHIFT))) {
				printf("[%llu]cmd set_addr, illegal frame %x\r\n", ts_tmp, dat);
				return;
			}
			addr_new = (dat&NEW_ADDR_1_MASK)>>NEW_ADDR_1_SHIFT;
			conf.led_addr = addr_new;
			conf.led_addr_inv = ~addr_new;
			write_ram_config_to_eeprom();
			printf("[%llu]cmd set_addr, %d\r\n", ts_tmp, addr_new);
		break; 
		default: 
			printf("[%llu]cmd unknown: %x\r\n", ts_tmp, dat);
		break;
	}
}

static void APP_ConfigErase(void)
{
  uint32_t PAGEError = 0;
  FLASH_EraseInitTypeDef EraseInitStruct;
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGEERASE;       
  EraseInitStruct.PageAddress = EEPROM_START_ADDR;
  EraseInitStruct.NbPages  = CONFIG_PAGES; 
  if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) {
    APP_ErrorHandler();
  }
}

static void APP_ConfigProgram(void)
{
  uint32_t flash_program_start = EEPROM_START_ADDR ;
  uint32_t flash_program_end = (EEPROM_START_ADDR + EEPROM_SIZE); 
  uint32_t *src = (uint32_t *)&conf;

  while (flash_program_start < flash_program_end) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, flash_program_start, src) == HAL_OK) {
      flash_program_start += FLASH_PAGE_SIZE; 
      src += FLASH_PAGE_SIZE / 4;             
    }
  }
}

uint64_t microsec()
{
	static uint64_t prev;
  uint64_t now;
  now = (ts * 10000) + TIM1->CNT;
  if (now < prev) 
		now += 10000;
  prev = now;
  return now;
}

uint64_t milisec()
{
	static uint64_t prev;
  uint64_t now;
  now = (ts * 10) + ((TIM1->CNT)/1000);
  if (now < prev) 
		now += 10;
  prev = now;
  return now;
}

void APP_CompInit(void)
{
  LL_COMP_InitTypeDef COMP_InitStruct = {0};
  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_InitStruct.Pin = LL_GPIO_PIN_0;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_COMP1);

  COMP_InitStruct.InputPlus = LL_COMP_INPUT_PLUS_IO3;
  COMP_InitStruct.InputMinus = LL_COMP_INPUT_MINUS_1_4VREFINT;	
  COMP_InitStruct.InputHysteresis = LL_COMP_HYSTERESIS_ENABLE;
  COMP_InitStruct.OutputPolarity = LL_COMP_OUTPUTPOL_NONINVERTED;

  LL_COMP_Init(COMP1, &COMP_InitStruct);
  LL_COMP_SetPowerMode(COMP1, LL_COMP_POWERMODE_HIGHSPEED);
  LL_COMP_SetCommonWindowMode(__LL_COMP_COMMON_INSTANCE(COMP1), LL_COMP_WINDOWMODE_DISABLE);

	LL_COMP_SetDigitalFilter(COMP1, FILTER_LONG_CNT); 
	LL_COMP_EnableDigitalFilter(COMP1);
	LL_EXTI_EnableRisingTrig(LL_EXTI_LINE_17); 
	LL_EXTI_EnableFallingTrig(LL_EXTI_LINE_17); 
  LL_EXTI_EnableIT(LL_EXTI_LINE_17);

  __IO uint32_t wait_loop_index = 0;
  wait_loop_index = (LL_COMP_DELAY_VOLTAGE_SCALER_STAB_US * (SystemCoreClock / (1000000 * 2)));
  while(wait_loop_index != 0)
    wait_loop_index--;
}

void APP_ComparatorTriggerCallback()
{
	uint32_t comp_state; 
	
	comp_state = LL_COMP_ReadOutputLevel(COMP1);
  if (comp_state==LL_COMP_OUTPUT_LEVEL_HIGH && comp_last_state==LL_COMP_OUTPUT_LEVEL_LOW) {
		LL_COMP_Disable(COMP1);
		LL_COMP_DisableDigitalFilter(COMP1);
		LL_COMP_SetDigitalFilter(COMP1, FILTER_SHORT_CNT); 
		LL_COMP_EnableDigitalFilter(COMP1);
		LL_COMP_Enable(COMP1);
		prev_bit_ts = new_bit_ts;
		new_bit_ts = microsec(); 
		update_flag = 1;
  } else if (comp_last_state==LL_COMP_OUTPUT_LEVEL_HIGH && comp_state==LL_COMP_OUTPUT_LEVEL_LOW) {
		LL_COMP_Disable(COMP1);
		LL_COMP_DisableDigitalFilter(COMP1);
		LL_COMP_SetDigitalFilter(COMP1, FILTER_LONG_CNT); 
		LL_COMP_EnableDigitalFilter(COMP1);
		LL_COMP_Enable(COMP1);
  }
	comp_last_state = comp_state;
}

static void APP_SystemClockConfig(void)
{
  LL_RCC_HSI_Enable();
  while(LL_RCC_HSI_IsReady() != 1);

  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSISYS);
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSISYS);

  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_Init1msTick(8000000);
  LL_SetSystemCoreClock(8000000);
}

static void APP_ConfigTIM1Count(void)
{
  LL_TIM_InitTypeDef TIM1CountInit = {0};
  
  TIM1CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1;
  TIM1CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;    
  TIM1CountInit.Prescaler           = 8-1;                    		
  TIM1CountInit.Autoreload          = TIMER_UNIT_US-1;           
  TIM1CountInit.RepetitionCounter   = 0;                         

  LL_TIM_Init(TIM1, &TIM1CountInit);
  LL_TIM_ClearFlag_UPDATE(TIM1);
  LL_TIM_EnableARRPreload(TIM1);
}

void APP_ConfigPWMChannel(void)
{
	LL_GPIO_InitTypeDef TIM3CH1MapInit= {0};
	#ifndef DEV_BOARD
	TIM3CH1MapInit.Pin        = LL_GPIO_PIN_2;
  TIM3CH1MapInit.Mode       = LL_GPIO_MODE_ALTERNATE;
  TIM3CH1MapInit.Alternate  = LL_GPIO_AF13_TIM3; 
  LL_GPIO_Init(GPIOA, &TIM3CH1MapInit);
	#endif
	TIM3CH1MapInit.Pin        = LL_GPIO_PIN_0 | LL_GPIO_PIN_5;
  TIM3CH1MapInit.Mode       = LL_GPIO_MODE_ALTERNATE;
  TIM3CH1MapInit.Alternate  = LL_GPIO_AF1_TIM3; 
  LL_GPIO_Init(GPIOB, &TIM3CH1MapInit);
	TIM3CH1MapInit.Pin        = LL_GPIO_PIN_2;
  TIM3CH1MapInit.Mode       = LL_GPIO_MODE_ANALOG;
	TIM3CH1MapInit.Alternate  = LL_GPIO_AF_0; 
  LL_GPIO_Init(GPIOF, &TIM3CH1MapInit);
	TIM3CH1MapInit.Pin        = LL_GPIO_PIN_10;
  TIM3CH1MapInit.Mode       = LL_GPIO_MODE_ANALOG;
	TIM3CH1MapInit.Alternate  = LL_GPIO_AF_0; 
  LL_GPIO_Init(GPIOA, &TIM3CH1MapInit);
	TIM3CH1MapInit.Pin        = LL_GPIO_PIN_6;
  TIM3CH1MapInit.Mode       = LL_GPIO_MODE_ANALOG;
	TIM3CH1MapInit.Alternate  = LL_GPIO_AF_0; 
  LL_GPIO_Init(GPIOB, &TIM3CH1MapInit);
  
  LL_TIM_OC_InitTypeDef TIM_OC_Initstruct ={0};
  TIM_OC_Initstruct.OCMode        = LL_TIM_OCMODE_PWM2;      
  TIM_OC_Initstruct.OCState       = LL_TIM_OCSTATE_ENABLE;  
  TIM_OC_Initstruct.OCPolarity    = LL_TIM_OCPOLARITY_HIGH;  
  TIM_OC_Initstruct.OCIdleState   = LL_TIM_OCIDLESTATE_LOW;  
  TIM_OC_Initstruct.CompareValue  = 0;
  LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH1, &TIM_OC_Initstruct);
  TIM_OC_Initstruct.CompareValue  = 0;
  LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH2, &TIM_OC_Initstruct);
  TIM_OC_Initstruct.CompareValue  = 0;
  LL_TIM_OC_Init(TIM3, LL_TIM_CHANNEL_CH3, &TIM_OC_Initstruct);
}

void APP_SetRGBValue(uint8_t r, uint8_t g, uint8_t b)
{
	LL_TIM_OC_SetCompareCH1(TIM3, r<<3);	// PA2, CH1, Red
	LL_TIM_OC_SetCompareCH2(TIM3, b<<3);	// PB5, CH2, Blue
	LL_TIM_OC_SetCompareCH3(TIM3, g<<3);	// PB0, CH3, Green
}

void APP_ConfigTIM3Base(void)
{
  LL_TIM_InitTypeDef TIM3CountInit = {0};
 
  TIM3CountInit.ClockDivision       = LL_TIM_CLOCKDIVISION_DIV1; 
  TIM3CountInit.CounterMode         = LL_TIM_COUNTERMODE_UP;
  TIM3CountInit.Prescaler           = PWM_PRESCALER-1;
  TIM3CountInit.Autoreload          = PWM_CNT_MAX-1;
  TIM3CountInit.RepetitionCounter   = 0;

  LL_TIM_Init(TIM3,&TIM3CountInit);
  LL_TIM_EnableAllOutputs(TIM3);
  LL_TIM_EnableCounter(TIM3);
}

inline void APP_UpdateCallback(void)
{
	ts++;
}



void APP_ErrorHandler(void)
{
	printf("APP_ErrorHandler()\r\n");
  while (1);
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  输出产生断言错误的源文件名及行号
  * @param  file：源文件名指针
  * @param  line：发生断言错误的行号
  * @retval 无
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* 用户可以根据需要添加自己的打印信息,
     例如: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* 无限循环 */
  while (1)
  {
  }
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/

/**
  ******************************************************************************
  * @file    main.h
  * @author  MCU Application Team
  * @brief   Header for main.c file.
  *          This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_ll_rcc.h"
#include "py32f0xx_ll_bus.h"
#include "py32f0xx_ll_system.h"
#include "py32f0xx_ll_exti.h"
#include "py32f0xx_ll_cortex.h"
#include "py32f0xx_ll_utils.h"
#include "py32f0xx_ll_pwr.h"
#include "py32f0xx_ll_dma.h"
#include "py32f0xx_ll_gpio.h"
#include "py32f0xx_ll_comp.h"
#include "py32f0xx_ll_tim.h"
#include "py32f0xx_hal.h"

#if defined(USE_FULL_ASSERT)
#include "py32_assert.h"
#endif /* USE_FULL_ASSERT */

/* Private includes ----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#define CONF_HEADER_1 0xFF
#define CONF_HEADER_2 0x00
#define CONF_HEADER_3 0x55
#define CONF_HEADER_4 0xAA
#define CONF_VER			0x01
#define CONF_LEN			0x80
#define CONF_DEF_ADDR	0x00
#define CONF_DEF_COLOR	0x00000000
#define CONF_DEF_FADE_TIME	0x00000000

#define PULSE_THRESH_US 		375
#define PULSE_UPPER_US 			650
#define PULSE_LOWER_US 			200
#define TIMER_UNIT_US				10000	
#define FRAME_LEN_BIT 			24
#define FILTER_LONG_CNT			400	
#define FILTER_SHORT_CNT 		5	

#define PWM_PRESCALER 			8 
#define PWM_CNT_MAX					256 
#define LED_UPDATE_PERIOD_MS	15	

#define EEPROM_START_ADDR  	0x08003F80
#define EEPROM_SIZE 				128
#define CONFIG_PAGES				((EEPROM_SIZE)/(FLASH_PAGE_SIZE))


#define CMD_MASK 			0x00E00000
#define CMD_SHIFT			21
#define CMD_SET_COLOR 0x01
#define CMD_SET_TIME  0x02
#define CMD_SET_ADDR	0x03
#define CMD_SET_ADDR	0x03
#define ADDR_MASK			0x001F0000
#define ADDR_SHIFT		16
#define ADDR_BCAST		0x1F //5'b11111

// assign frame_set_color[31:0] = {reserved[7:0], cmd[2:0], addr[4:0], r[4:0], g[4:0], b[4:0], parity[0:0]};
#define RED_MASK			0x0000F800
#define RED_SHIFT			11
#define GREEN_MASK		0x000007C0
#define GREEN_SHIFT		6
#define BLUE_MASK			0x0000003E
#define BLUE_SHIFT		1
// TO-DO: assign frame_set_time = {reserved[7:0], cmd[2:0], addr[4:0], fade_time_ms[13:0], rgb_hsv[0:0], parity[0:0]};
#define FADE_TIME_MASK			0x0000FFFC
#define FADE_TIME_SHIFT			2
#define RGB_HSV_MASK				0x00000002
#define RGB_HSV_SHIFT				1
// assign frame_set_addr = {reserved[7:0], cmd[2:0], addr[4:0], new_addr[4:0], new_addr_comp[4:0], new_addr_repeat[4:0], parity[0:0]};
// note: repeats are just to make sure TX really wants to change the addr
#define NEW_ADDR_1_MASK				0x0000F800
#define NEW_ADDR_1_SHIFT			11
#define NEW_ADDR_COMP_MASK		0x000007C0
#define NEW_ADDR_COMP_SHIFT		6
#define NEW_ADDR_2_MASK				0x0000003E
#define NEW_ADDR_2_SHIFT			1
/* Exported variables prototypes ---------------------------------------------*/
typedef struct {
	uint8_t header1;	// FF
	uint8_t header2;	// 00	
	uint8_t header3;	// 55
	uint8_t header4;	// AA	
	uint8_t ver;			// 01
	uint8_t len;			// 80
	
	uint8_t led_addr;
	uint8_t led_addr_inv;
	
	uint32_t led_color_default;
	uint32_t led_color_default_inv;
	
	uint32_t led_fade_timer_default;
	uint32_t led_fade_timer_default_inv;
	
	uint8_t _reserved[108];
} eeprom_conf_t;
/* Exported functions prototypes ---------------------------------------------*/
void APP_ErrorHandler(void);
extern void APP_ComparatorTriggerCallback(void);
extern void APP_UpdateCallback(void); 

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE******************/

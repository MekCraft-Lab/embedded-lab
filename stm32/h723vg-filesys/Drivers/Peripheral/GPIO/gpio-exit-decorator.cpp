/**
 *******************************************************************************
 * @file    gpio-exit-decorator.cpp
 * @brief   The interrupt callback function decorator of GPIO class
 *******************************************************************************
 * @attention
 *
 * Please configure the EXTI IRQHandler first
 *
 *******************************************************************************
 * @note
 *
 * This file is implement the interrupt callback function decorator of GPIO
 * class. You need to pass a regular GPIO object into the constructor of this
 * decorator. And you can get a GPIO object with callback function.
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/8/8
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ------------------------------------------------------------*/





/* ------- include -----------------------------------------------------------*/

#include "gpio-intf.hpp"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"




/* ------- class prototypes---------------------------------------------------*/





/* ------- macro -------------------------------------------------------------*/





/* ------- variables ---------------------------------------------------------*/

std::function<void()> GpioExtiDecorator::_exti_cb[16] = {};




/* ------- function implement ------------------------------------------------*/

/**
 * @brief configure the callback function of EXTI
 * @param cb callback function
 * @return GPIO error code
 */
GpioErrCode GpioExtiDecorator::register_callback(
    std::function<void(void)> cb) const
{
    const uint8_t index = static_cast<uint8_t>(_gpio->pin_getter()) - 2;
    if (_exti_cb[index] == nullptr)
    {
        _exti_cb[index] = cb;
    }
    else
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    return GpioErrCode::GPIO_SUCCESS;
}


/**
 * @brief Enable the EXTI interrupt of the pin.
 * @return GpioErrCode
 */
GpioErrCode GpioExtiDecorator::enable_interrupt() const
{
    IRQn_Type EXTI_x_IRQn;
    switch (_gpio->pin_getter())
    {
        case GpioPinEnum::GPIO_PIN_0_:
            EXTI_x_IRQn = EXTI0_IRQn;
            break;
        case GpioPinEnum::GPIO_PIN_1_:
            EXTI_x_IRQn = EXTI1_IRQn;
            break;
        case GpioPinEnum::GPIO_PIN_2_:
            EXTI_x_IRQn = EXTI2_IRQn;
            break;
        case GpioPinEnum::GPIO_PIN_3_:
            EXTI_x_IRQn = EXTI3_IRQn;
            break;
        case GpioPinEnum::GPIO_PIN_4_:
            EXTI_x_IRQn = EXTI4_IRQn;
            break;
        case GpioPinEnum::GPIO_PIN_5_:
        case GpioPinEnum::GPIO_PIN_6_:
        case GpioPinEnum::GPIO_PIN_7_:
        case GpioPinEnum::GPIO_PIN_8_:
        case GpioPinEnum::GPIO_PIN_9_:
            EXTI_x_IRQn = EXTI9_5_IRQn;
            break;

        case GpioPinEnum::GPIO_PIN_10_:
        case GpioPinEnum::GPIO_PIN_11_:
        case GpioPinEnum::GPIO_PIN_12_:
        case GpioPinEnum::GPIO_PIN_13_:
        case GpioPinEnum::GPIO_PIN_14_:
        case GpioPinEnum::GPIO_PIN_15_:
            EXTI_x_IRQn = EXTI15_10_IRQn;
            break;
        default:
            return GpioErrCode::GPIO_ERR_NONE;
    }

    if (_gpio->enable_getter())
    {
        HAL_NVIC_SetPriority(EXTI_x_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
    else
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }

    return GpioErrCode::GPIO_SUCCESS;
}

GpioErrCode GpioExtiDecorator::disable_interrupt()
{
    return GpioErrCode::GPIO_SUCCESS;
}

/**
 * @brief Get the callback function by the exti number.
 * @param exti_line
 * @return Callback function or error code.
 */
std::variant<std::function<void()>, GpioErrCode>
GpioExtiDecorator::exti_cb_getter(uint8_t exti_line)
{
    if (_exti_cb[exti_line] == nullptr)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    return _exti_cb[exti_line];
}

/**
 * @brief This is the global callback function for EXTI interrupts.
 * @param GPIO_Pin
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // __builtin_ctz是从值获取最低位的1在第多少位
    uint8_t pos = (GPIO_Pin == 0) ? -1 : __builtin_ctz(GPIO_Pin);
    if (pos < 1)
    {
        return;
    }

    auto cb = GpioExtiDecorator::exti_cb_getter(pos - 1);

    // If the return of exit_cb_getter is none-error then we can use it.
    if (std::holds_alternative<std::function<void(void)>>(cb))
    {
        std::get<std::function<void(void)>>(cb)();
    }
}

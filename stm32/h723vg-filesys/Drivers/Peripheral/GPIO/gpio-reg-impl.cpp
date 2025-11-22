/**
 *******************************************************************************
 * @file    GpioRegImpl.cpp
 * @brief   寄存器实现的GPIO封装
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * 定义了寄存器实现的GPIO封装的实现以及工厂的实现和变量
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/8/7
 * @version 1.0
 *******************************************************************************
 */




/* ------- define ------------------------------------------------------------*/





/* ------- include -----------------------------------------------------------*/

#include "gpio-intf.hpp"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include <variant>




/* ------- class prototypes---------------------------------------------------*/

class pyro_gpio_reg_impl_t : public IGpio
{
  public:
    pyro_gpio_reg_impl_t(GpioPortEnum port, GpioPinEnum pin, GpioModeEnum mode);
    GpioErrCode enable() override;
    GpioErrCode set() override;
    GpioErrCode reset() override;
    GpioErrCode write(GpioStateEnum state) override;
    std::variant<GpioStateEnum, GpioErrCode> read() override;
    GpioErrCode toggle() override;

  private:
    using IGpio::_gpioRegAddr;
    using IGpio::_mode;
    using IGpio::_pin;
    using IGpio::_port;
    uint32_t pinRaw = 0;
};


/**
 * @brief The Factory to produce the GPIO object implement with register
 */
class gpio_reg_fcty_impl_t : public GpioFctyIntf
{
    std::variant<IGpio *, GpioErrCode>
    produce(GpioPortEnum port, GpioPinEnum pin, GpioModeEnum mode) override
    {
        IGpio *g = new pyro_gpio_reg_impl_t(port, pin, mode);
        if (g == nullptr)
        {
            return GpioErrCode::GPIO_MEM_ALLOC_FAILED;
        }
        return g;
    }
};

gpio_reg_fcty_impl_t gpioRegFcty;
GpioFctyIntf *p_gpio_reg_fcty = &gpioRegFcty;



/* ------- macro -------------------------------------------------------------*/





/* ------- variables ---------------------------------------------------------*/





/* ------- function implement ------------------------------------------------*/


/**
 * @brief 库驱动的GPIO封装构造方法
 * @param port 端口
 * @param pin 引脚
 * @param mode 模式
 */
pyro_gpio_reg_impl_t::pyro_gpio_reg_impl_t(GpioPortEnum port, GpioPinEnum pin,
                                           GpioModeEnum mode)
{
    pyro_gpio_reg_impl_t::_port = port;
    pyro_gpio_reg_impl_t::_pin  = pin;
    pyro_gpio_reg_impl_t::_mode = mode;
}

/**
 * @brief GPIO库方式实现使能
 * @return GPIO Error Code
 */
GpioErrCode pyro_gpio_reg_impl_t::enable()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    uint32_t gpioPin                 = 0;
    uint32_t gpioMode                = 0;

    /* param check */
    if (_port > GpioPortEnum::GPIO_PORT_G and
        _port == GpioPortEnum::GPIO_PORT_NONE)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    else if (_pin > GpioPinEnum::GPIO_PIN_15_ and
             _pin == GpioPinEnum::GPIO_PIN_NONE_)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }

    /* 使能RCC */
    uint8_t bitPos = static_cast<uint8_t>(_port) - 1;
    _gpioRegAddr   = reinterpret_cast<void *>(
        (D3_AHB1PERIPH_BASE + (static_cast<uint8_t>(_port) - 1) * 0x0400UL));

    SET_BIT(RCC->AHB4ENR, 1 << bitPos);

    gpioPin               = 1 << (static_cast<uint8_t>(_pin) - 1);

    gpioMode              = static_cast<uint32_t>(_mode);

    pinRaw                = gpioPin;

    GPIO_InitStruct.Pin   = gpioPin;
    GPIO_InitStruct.Mode  = gpioMode;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(static_cast<GPIO_TypeDef *>(_gpioRegAddr), &GPIO_InitStruct);

    _isEnabled = true;

    return GpioErrCode::GPIO_SUCCESS;
}

/**
 * @brief 库驱动的GPIO封装设为高电平
 * @return GPIO Error Code
 */
GpioErrCode pyro_gpio_reg_impl_t::set()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }

    static_cast<GPIO_TypeDef *>(_gpioRegAddr)->BSRR = (pinRaw << 16);

    return GpioErrCode::GPIO_SUCCESS;
}

/**
 * @brief 库驱动的GPIO封装设为低电平
 * @return GPIO Error Code
 */
GpioErrCode pyro_gpio_reg_impl_t::reset()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    static_cast<GPIO_TypeDef *>(_gpioRegAddr)->BSRR = pinRaw;
    return GpioErrCode::GPIO_SUCCESS;
}

/**
 * @brief 库驱动的GPIO封装设置引脚状态
 * @param state 设定的引脚状态
 * @return GPIO Error Code
 */
GpioErrCode pyro_gpio_reg_impl_t::write(GpioStateEnum state)
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    // 使用强制类型转换后被.clang-tidy推荐使用auto关键字
    if (state == GpioStateEnum::GPIO_STATE_RESET)
    {
        static_cast<GPIO_TypeDef *>(_gpioRegAddr)->BSRR = pinRaw;
    }
    else if (state == GpioStateEnum::GPIO_STATE_SET)
    {
        static_cast<GPIO_TypeDef *>(_gpioRegAddr)->BSRR = pinRaw << 16;
    }
    else
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    return GpioErrCode::GPIO_SUCCESS;
}


/**
 * @brief 基于库函数的GPIO读取封装实现
 * @return  Error Code和读取到引脚结果
 */
std::variant<GpioStateEnum, GpioErrCode> pyro_gpio_reg_impl_t::read()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    uint8_t res;

    if ((static_cast<GPIO_TypeDef *>(_gpioRegAddr)->IDR & pinRaw) != 0x00U)
    {
        res = GPIO_PIN_SET;
    }
    else
    {
        res = GPIO_PIN_RESET;
    }

    return static_cast<GpioStateEnum>(res + 1);
}

/**
 * @brief 基于库函数的GPIO翻转封装实现
 * @return GPIO Error Code
 */
GpioErrCode pyro_gpio_reg_impl_t::toggle()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }


    const uint32_t odr = (static_cast<GPIO_TypeDef *>(_gpioRegAddr))->ODR;

    /* Set selected pins that were at low level, and reset ones that were high
     */
    (static_cast<GPIO_TypeDef *>(_gpioRegAddr))->BSRR =
        ((odr & pinRaw) << 16) | (~odr & pinRaw);

    return GpioErrCode::GPIO_SUCCESS;
}

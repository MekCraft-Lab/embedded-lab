/**
 *******************************************************************************
 * @file    GpioLibImpl.cpp
 * @brief   库驱动GPIO封装实现
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * 定义了库驱动的GPIO封装的实现以及工厂的实现和变量
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/8/7
 * @version 1.0
 *******************************************************************************
 */



/* ------- define ------------------------------------------------------------*/





/* ------- includes & imports ------------------------------------------------*/

#include "gpio-intf.hpp"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal_gpio.h"
#include <variant>




/* ------- macro -------------------------------------------------------------*/





/* ------- class prototypes --------------------------------------------------*/

/**
 * @brief GPIO类的库实现
 */
class pyro_gpio_lib_impl_t : public IGpio
{
  public:
    pyro_gpio_lib_impl_t(GpioPortEnum port, GpioPinEnum pin, GpioModeEnum mode);
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
 * @brief 生产GPIO库实现的工厂
 */
class gpio_lib_fcty_impl_t : public GpioFctyIntf
{
    std::variant<IGpio *, GpioErrCode>
    produce(GpioPortEnum port, GpioPinEnum pin, GpioModeEnum mode) override
    {
        IGpio *g = new pyro_gpio_lib_impl_t(port, pin, mode);
        if (g == nullptr)
        {
            return GpioErrCode::GPIO_MEM_ALLOC_FAILED;
        }
        return g;
    }
};

gpio_lib_fcty_impl_t gpioLibFcty;
GpioFctyIntf *p_gpio_lib_fcty = &gpioLibFcty;





/* ------- variables ---------------------------------------------------------*/





/* ------- function implement ------------------------------------------------*/



/**
 * @brief 库驱动的GPIO封装构造方法
 * @param port 端口
 * @param pin 引脚
 * @param mode 模式
 */
pyro_gpio_lib_impl_t::pyro_gpio_lib_impl_t(GpioPortEnum port, GpioPinEnum pin,
                                           GpioModeEnum mode)
{
    pyro_gpio_lib_impl_t::_port = port;
    pyro_gpio_lib_impl_t::_pin  = pin;
    pyro_gpio_lib_impl_t::_mode = mode;
}

/**
 * @brief GPIO库方式实现使能
 * @return GPIO错误码
 */
GpioErrCode pyro_gpio_lib_impl_t::enable()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_TypeDef *gpioX              = nullptr;
    uint32_t gpioPin                 = 0;
    uint32_t gpioMode                = 0;

    /* 参数检查 */
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

    switch (IGpio::_port)
    {
        case GpioPortEnum::GPIO_PORT_A:
        {
            __HAL_RCC_GPIOA_CLK_ENABLE();
            gpioX = GPIOA;
        }
        break;

        case GpioPortEnum::GPIO_PORT_B:
        {
            __HAL_RCC_GPIOB_CLK_ENABLE();
            gpioX = GPIOB;
        }
        break;

        case GpioPortEnum::GPIO_PORT_C:
        {
            __HAL_RCC_GPIOC_CLK_ENABLE();
            gpioX = GPIOC;
        }
        break;

        case GpioPortEnum::GPIO_PORT_D:
        {
            __HAL_RCC_GPIOD_CLK_ENABLE();
            gpioX = GPIOD;
        }
        break;

        case GpioPortEnum::GPIO_PORT_E:
        {
            __HAL_RCC_GPIOE_CLK_ENABLE();
            gpioX = GPIOE;
        }
        break;

        case GpioPortEnum::GPIO_PORT_F:
        {
            __HAL_RCC_GPIOF_CLK_ENABLE();
            gpioX = GPIOF;
        }
        break;

        case GpioPortEnum::GPIO_PORT_G:
        {
            __HAL_RCC_GPIOG_CLK_ENABLE();
            gpioX = GPIOG;
        }
        break;

        case GpioPortEnum::GPIO_PORT_H:
        {
            __HAL_RCC_GPIOH_CLK_ENABLE();
            gpioX = GPIOH;
        }
        break;

        default:
            return GpioErrCode::GPIO_ERR_NONE;
            break;
    }

    gpioPin               = 1 << (static_cast<uint8_t>(_pin) - 1);

    gpioMode              = static_cast<uint32_t>(_mode);

    _gpioRegAddr          = static_cast<void *>(gpioX);
    pinRaw                = gpioPin;

    GPIO_InitStruct.Pin   = gpioPin;
    GPIO_InitStruct.Mode  = gpioMode;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(gpioX, &GPIO_InitStruct);

    _isEnabled = true;

    return GpioErrCode::GPIO_SUCCESS;
}

/**
 * @brief 库驱动的GPIO封装设为高电平
 * @return GPIO错误码
 */
GpioErrCode pyro_gpio_lib_impl_t::set()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef *>(_gpioRegAddr), pinRaw,
                      GPIO_PIN_SET);
    return GpioErrCode::GPIO_SUCCESS;
}

/**
 * @brief 库驱动的GPIO封装设为低电平
 * @return GPIO错误码
 */
GpioErrCode pyro_gpio_lib_impl_t::reset()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef *>(_gpioRegAddr), pinRaw,
                      GPIO_PIN_RESET);
    return GpioErrCode::GPIO_SUCCESS;
}

/**
 * @brief 库驱动的GPIO封装设置引脚状态
 * @param state 设定的引脚状态
 * @return GPIO错误码
 */
GpioErrCode pyro_gpio_lib_impl_t::write(GpioStateEnum state)
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    // 使用强制类型转换后被.clang-tidy推荐使用auto关键字
    auto stateTemp =
        static_cast<GPIO_PinState>(static_cast<uint8_t>(state) - 1);
    HAL_GPIO_WritePin(static_cast<GPIO_TypeDef *>(_gpioRegAddr), pinRaw,
                      stateTemp);
    return GpioErrCode::GPIO_SUCCESS;
}


/**
 * @brief 基于库函数的GPIO读取封装实现
 * @return 错误码和读取到引脚结果
 */
std::variant<GpioStateEnum, GpioErrCode> pyro_gpio_lib_impl_t::read()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    const uint8_t res =
        HAL_GPIO_ReadPin(static_cast<GPIO_TypeDef *>(_gpioRegAddr), pinRaw);

    return static_cast<GpioStateEnum>(res + 1);
}

/**
 * @brief 基于库函数的GPIO翻转封装实现
 * @return GPIO错误码
 */
GpioErrCode pyro_gpio_lib_impl_t::toggle()
{
    if (pinRaw > 15)
    {
        return GpioErrCode::GPIO_ERR_NONE;
    }
    HAL_GPIO_TogglePin(static_cast<GPIO_TypeDef *>(_gpioRegAddr), pinRaw);

    return GpioErrCode::GPIO_SUCCESS;
}

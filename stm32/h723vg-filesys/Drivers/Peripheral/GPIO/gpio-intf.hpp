/**
*******************************************************************************
* @file    gpio-intf.hpp
* @brief   the interface of GPIO driver
*******************************************************************************
* @attention
*
* If you want to use the EXTI decorator without CubeMX, please configure the
* EXTI IRQHandler first in stm32h7xx_it.c
*
*******************************************************************************
* @note
*
* The driver is mainly composed of three parts: the interface specification,
* various implementations, and extended functionalities implemented through
* decorators.
*
* +---------------------------------------------------------------------------+
* |                      gpio-intf.hpp - interface                            |
* +---------------------------------------------------------------------------+
*               |               |                             |
*               |               |             |               |
* +--------------------+--------------------+ | +-----------------------------+
* |GpioLibImpl.cpp     |  GpioLibImpl.cpp   | | |      GpioIsrDecorator.cpp   |
* +--------------------+--------------------+ | +-----------------------------+
* |implement - 1       | implement - 2      | | |     decorator - 1           |
* +--------------------+--------------------+ | +-----------------------------+
*                                             |
*
* This is an interface file of GPIO driver. And it can be divided into 5 parts.
* 1. the includes
* 2. the encapsulation of enum
* 3. the interface
* 4. the decorator
* 5. the factories
*
* I. the includes
*   To achieve complete decoupling between the upper and lower layers, the
*   upper layer should not depend on specific chip implementations when handing
*   business logic. As an interface for the upper layer, it should not include
*   header files from the HAL library or any other low-level libraries.
*   ---------------------------------------------------------------------------
* II. the encapsulation of enum
*   Since low-level libraries will not be included, yet the header file still
*   needs to provide information related to the lower-level library, the
*   interface file must encapsulate the relevant details. It should bind the
*   information that needs to be passed from the upper layer to the lower
*   layer.
*   ---------------------------------------------------------------------------
* III. the interface
*   This part is the most important--it provides the driver interface to the
*   upper layer. The upper layer only needs to look for the required methods
*   within these interfaces and understand how to use them in order to
*   implement th corresponding functionality. At the same time, the lower layer
*   must follow this interface to implement the required functions
*   accordingly. This part provides two interface: one for the driver itself,
*   and another for the factory that generates different driver
*   implementations.
*   ---------------------------------------------------------------------------
* IV. the decorator
*   Sometimes, implementations can be multidimensional. For example, a GPIO
*   driver may have different implementation methods, such as using a
*   library-based driver or a register based one, and from a usage perspective,
*   it may support interrupt-enabled or non-interrupt modes. Writing a separate
*   implementation for each combination would lead to redundancy. To improve
*   reusability, the decorator pattern can be used--allowing new features to
*   be added without rewriting existing code. At the same time, to ensure
*   interface consistency, the decorator itself also implements the same
*   interface.
*   ---------------------------------------------------------------------------
* V. the factories
*   To ensure that the upper layer does not need to worry about how the
*   required objects are created, it only needs to focus on obtaining them
*   (handled by the factory) and knowing how to use them (defined by the
*   interface). Therefore, different implementations of an interface are
*   produced by factories and returned to the caller. Each factory is
*   responsible for one product (i.e., one specific implementation), and the
*   factory interface is standardized--every factory provides the same
*   functionality, differing only in the final product they create.
*
*******************************************************************************
* @author  MekLi😋
* @date    2025/8/7
* @version 1.0
*******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/

#pragma once




/*-------- 1. includes & imports ---------------------------------------------*/

#include <cstdint>
#include <functional>
#include <variant>




/*-------- 2. enum -----------------------------------------------------------*/

/**
 * @brief the encapsulation of GPIO port
 */
enum class GpioPortEnum
{
    GPIO_PORT_NONE,
    GPIO_PORT_A,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_F,
    GPIO_PORT_G,
    GPIO_PORT_H,
};

/**
 * @brief the encapsulation of GPIO pin
 */
enum class GpioPinEnum
{
    GPIO_PIN_NONE_,
    GPIO_PIN_0_,
    GPIO_PIN_1_,
    GPIO_PIN_2_,
    GPIO_PIN_3_,
    GPIO_PIN_4_,
    GPIO_PIN_5_,
    GPIO_PIN_6_,
    GPIO_PIN_7_,
    GPIO_PIN_8_,
    GPIO_PIN_9_,
    GPIO_PIN_10_,
    GPIO_PIN_11_,
    GPIO_PIN_12_,
    GPIO_PIN_13_,
    GPIO_PIN_14_,
    GPIO_PIN_15_,
};

/**
 * @brief the encapsulation of GPIO mode
 */
enum class GpioModeEnum
{
    GPIO_MODE_NONE_,                    // null
    GPIO_MODE_INPUT_      = 0x00000000, // input mode
    GPIO_MODE_OUTPUT_PP_  = 0x00000001, // push-pull output
    GPIO_MODE_OUTPUT_OD_  = 0x00000011, // open-drain output
    GPIO_MODE_AF_PP_      = 0x00000002, // alternative push-pull output
    GPIO_MODE_AF_OD_      = 0x00000012, // alternative open-drain output
    GPIO_MODE_ANALOG_     = 0x00000003, // analog mode
    GPIO_MODE_IT_RISING_  = 0x00110000, // interrupt with rising edge
    GPIO_MODE_IT_FALLING_ = 0x00210000, // interrupt with falling edge
    GPIO_MODE_IT_RISING_FALLING_ =
        0x00310000, // interrupt with rising edge and falling edge
};

/**
 * @brief the encapsulation of GPIO state
 */
enum class GpioStateEnum
{
    GPIO_STATE_NONE,
    GPIO_STATE_RESET,
    GPIO_STATE_SET,
};


/**
 * @brief GPIO driver error code
 */

enum class GpioErrCode {
    GPIO_ERR_NONE,
    GPIO_PORT_NOT_EXIST,
    GPIO_PIN_NOT_EXIST,
    GPIO_PIN_STATE_NOT_EXIST,
    GPIO_PIN_MODE_NOT_EXIST,
    GPIO_PIN_EXTI_NOT_EXIST,
    GPIO_PIN_NOT_EN,
    GPIO_MEM_ALLOC_FAILED,
    GPIO_PIN_EXTI_CB_EXIST,
    GPIO_SUCCESS,
};




/*-------- 3. interface ------------------------------------------------------*/

/**
 * @brief 1.GPIO interface
 *
 * @note This is the main part, it defines the GPIO interface that the upper
 * layer can use.
 *
 */
class IGpio
{
  public:
    /**
     * @brief enable the GPIO pin
     */
    [[nodiscard]] virtual GpioErrCode enable()                           = 0;

    /**
     * @brief set the level
     */
    [[nodiscard]] virtual GpioErrCode set()                              = 0;

    /**
     * @brief reset the level
     */
    [[nodiscard]] virtual GpioErrCode reset()                            = 0;

    /**
     * @brief read the state
     * @return return value with error code and state
     */
    // C++17 introduced a type that returns both error codes and results, which
    // seems to be quite interesting
    [[nodiscard]] virtual std::variant<GpioStateEnum, GpioErrCode> read() = 0;

    /**
     * @brief set the level
     * @param state
     */
    [[nodiscard]] virtual GpioErrCode write(GpioStateEnum state)          = 0;

    /**
     * @brief toggle the level
     */
    [[nodiscard]] virtual GpioErrCode toggle()                           = 0;

    virtual ~IGpio() = default;

    /****************** setter & getter *******************/

    void pin_setter(const GpioPinEnum pin)
    {
        this->_pin = pin;
    }
    [[nodiscard]] GpioPinEnum pin_getter() const
    {
        return this->_pin;
    }

    void port_setter(const GpioPortEnum port)
    {
        this->_port = port;
    }
    [[nodiscard]] GpioPortEnum port_getter() const
    {
        return this->_port;
    }

    void mode_setter(const GpioModeEnum mode)
    {
        this->_mode = mode;
    }
    [[nodiscard]] GpioModeEnum mode_getter() const
    {
        return this->_mode;
    }

    [[nodiscard]] bool enable_getter() const
    {
        return this->_isEnabled;
    }

    /****************** setter & getter *******************/


  protected:
    void *_gpioRegAddr = nullptr;
    GpioPortEnum _port  = GpioPortEnum::GPIO_PORT_NONE;
    GpioPinEnum _pin    = GpioPinEnum::GPIO_PIN_NONE_;
    GpioModeEnum _mode  = GpioModeEnum::GPIO_MODE_NONE_;
    bool _isEnabled    = false;
};


/**
 * @brief 2. abstract factory
 *
 * @note This is actually the interface of factories, and so far these factories
 * only has one method: produce.
 */
class GpioFctyIntf
{
  public:
    virtual ~GpioFctyIntf() = default;
    /**
     * @brief The produce method of the factory
     * @param port
     * @param pin
     * @param mode
     * @return
     */
    [[nodiscard]] virtual std::variant<IGpio *, GpioErrCode>
    produce(GpioPortEnum port, GpioPinEnum pin, GpioModeEnum mode) = 0;
};



/*-------- 4. decorator ------------------------------------------------------*/


/**
 * @brief decorator implement additional interrupt method
 *
 * @note  The Decorate Pattern is implemented through combination and
 * inheritance
 *
 */
class GpioExtiDecorator final : public IGpio
{
  public:
    static uint8_t a;
    /***************** base class interface *********************/
    [[nodiscard]] GpioErrCode enable() override
    {
        return this->_gpio->enable();
    }
    [[nodiscard]] GpioErrCode set() override
    {
        return this->_gpio->set();
    }
    [[nodiscard]] GpioErrCode reset() override
    {
        return this->_gpio->reset();
    }
    [[nodiscard]] std::variant<GpioStateEnum, GpioErrCode> read() override
    {
        return this->_gpio->read();
    }
    [[nodiscard]] GpioErrCode write(GpioStateEnum state) override
    {
        return this->_gpio->write(state);
    }
    [[nodiscard]] GpioErrCode toggle() override
    {
        return this->_gpio->toggle();
    }
    /***************** new interface ****************************/



    /**
     * @brief
     * @note explicit keyword prohibits implicit type conversion
     */
    explicit GpioExtiDecorator(IGpio *gpio)
    {
        this->_gpio = gpio;
    }

    /**
     * @brief configure callback function
     * @param cb callback function
     */
    [[nodiscard]] GpioErrCode
    register_callback(std::function<void(void)> cb) const;

    /**
     * @brief enable the interrupt of EXTI
     */
    [[nodiscard]] GpioErrCode enable_interrupt() const;

    /**
     * @brief disable the interrupt of EXTI
     */
    [[nodiscard]] static GpioErrCode disable_interrupt();

    /**
     *
     */
    [[nodiscard]] static std::variant<std::function<void()>, GpioErrCode>
    exti_cb_getter(uint8_t);


  private:
    IGpio *_gpio;             // decorated GPIO object
    std::function<void()> _callback; // callback function
    static std::function<void()> _exti_cb[16];
};



/*-------- 5. factories ------------------------------------------------------*/

extern GpioFctyIntf
    *p_gpio_lib_fcty; // GPIO encapsulation implemented through libraries
extern GpioFctyIntf
    *p_gpio_reg_fcty; // GPIO encapsulation implemented through registers

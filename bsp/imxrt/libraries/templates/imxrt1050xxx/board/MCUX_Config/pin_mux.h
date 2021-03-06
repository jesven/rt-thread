/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#ifndef _PIN_MUX_H_
#define _PIN_MUX_H_

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/

/*! @brief Direction type  */
typedef enum _pin_mux_direction
{
  kPIN_MUX_DirectionInput = 0U,         /* Input direction */
  kPIN_MUX_DirectionOutput = 1U,        /* Output direction */
  kPIN_MUX_DirectionInputOrOutput = 2U  /* Input or output direction */
} pin_mux_direction_t;

/*!
 * @addtogroup pin_mux
 * @{
 */

/***********************************************************************************************************************
 * API
 **********************************************************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Calls initialization functions.
 *
 */
void BOARD_InitBootPins(void);

/* GPIO_AD_B0_12 (coord K14), UART1_TXD */
#define BOARD_INITPINS_UART1_TXD_PERIPHERAL                              LPUART1   /*!< Device name: LPUART1 */
#define BOARD_INITPINS_UART1_TXD_SIGNAL                                       TX   /*!< LPUART1 signal: TX */

/* GPIO_AD_B0_13 (coord L14), UART1_RXD */
#define BOARD_INITPINS_UART1_RXD_PERIPHERAL                              LPUART1   /*!< Device name: LPUART1 */
#define BOARD_INITPINS_UART1_RXD_SIGNAL                                       RX   /*!< LPUART1 signal: RX */

/* GPIO_AD_B1_08 (coord H13), AUD_INT/CSI_D9//J35[13]/J22[4] */
#define BOARD_INITPINS_CSI_D9_PERIPHERAL                                    PWM4   /*!< Device name: PWM4 */
#define BOARD_INITPINS_CSI_D9_SIGNAL                                           A   /*!< PWM4 signal: A */
#define BOARD_INITPINS_CSI_D9_CHANNEL                                         0U   /*!< PWM4 A channel: 0 */

/* GPIO_AD_B1_09 (coord M13), SAI1_MCLK/CSI_D8/J35[11] */
#define BOARD_INITPINS_CSI_D8_PERIPHERAL                                    PWM4   /*!< Device name: PWM4 */
#define BOARD_INITPINS_CSI_D8_SIGNAL                                           A   /*!< PWM4 signal: A */
#define BOARD_INITPINS_CSI_D8_CHANNEL                                         1U   /*!< PWM4 A channel: 1 */

/* GPIO_AD_B0_10 (coord G13), JTAG_TDO/J21[13]/INT1_COMBO/ENET_INT/J22[6]/U32[11] */
#define BOARD_INITPINS_INT1_COMBO_PERIPHERAL                                PWM1   /*!< Device name: PWM1 */
#define BOARD_INITPINS_INT1_COMBO_SIGNAL                                       A   /*!< PWM1 signal: A */
#define BOARD_INITPINS_INT1_COMBO_CHANNEL                                     3U   /*!< PWM1 A channel: 3 */


/*!
 * @brief Configures pin routing and optionally pin electrical features.
 *
 */
void BOARD_InitPins(void);

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif /* _PIN_MUX_H_ */

/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/

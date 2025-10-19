/**
 *
 * Copyright (c) 2024 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "htnb32lxxx_hal_i2c.h"
#include "slpman_qcx212.h"
#include "stdio.h"
#include "Driver_I2C.h"

#define ARM_I2C_DRV_VERSION    ARM_DRIVER_VERSION_MAJOR_MINOR(2, 0) /* driver version */

volatile uint8_t _tx_callback = 0;
volatile uint8_t _rx_callback = 0;

#if ((!RTE_I2C0) && (!RTE_I2C1))
#error "I2C not enabled in RTE_Device.h!"
#endif

#ifdef PM_FEATURE_ENABLE

/** \brief Internal used data structure */
typedef struct _i2c_database
{
    bool                            isInited;            /**< Whether i2c has been initialized */
    struct
    {
        uint32_t MCR;                                    /**< Main Control Register */
        uint32_t SAR;                                    /**< Slave Address Register */
        uint32_t TPR;                                    /**< Timing Parameter Register */
        uint32_t TOR;                                    /**< Timeout Register */
        uint32_t IER;                                    /**< Interrupt Enable Register */
        uint32_t IMR;                                    /**< Interrupt Mask Register */
    } backup_registers;                                  /**< Backup registers for low power restore */
} i2c_database_t;

static i2c_database_t g_i2cDataBase[I2C_INSTANCE_NUM] = {0};

static I2C_TypeDef* const g_i2cBases[I2C_INSTANCE_NUM] = {I2C0, I2C1};
#endif

static const clock_ID_t g_i2cClocks[I2C_INSTANCE_NUM*2] = {GPR_I2C0APBClk, GPR_I2C0FuncClk, GPR_I2C1APBClk, GPR_I2C1FuncClk};
// static const sw_reset_ID_t g_i2cResetClocks[I2C_INSTANCE_NUM*2] = {GPR_ResetI2C0APB, GPR_ResetI2C0Func, GPR_ResetI2C1APB, GPR_ResetI2C1Func};

#ifdef PM_FEATURE_ENABLE
/**
  \brief i2c initialization counter, for lower power callback register/de-register
 */
static uint32_t g_i2cInitCounter = 0;

/**
  \brief Bitmap of I2C working status,
         when all I2C instances are not working, we can vote to enter to low power state.
 */

static uint32_t g_i2cWorkingStatus = 0;

/**
  \fn        static void I2C_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
  \brief     Perform necessary preparations before sleep.
             After recovering from SLPMAN_SLEEP1_STATE, I2C hareware is repowered, we backup
             some registers here first so that we can restore user's configurations after exit.
  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state
 */
static void I2C_EnterLowPowerStatePrepare(void* pdata, slpManLpState state)
{
    uint32_t i;

    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            for(i = 0; i < I2C_INSTANCE_NUM; i++)
            {
                if(g_i2cDataBase[i].isInited == true)
                {
                    g_i2cDataBase[i].backup_registers.MCR = g_i2cBases[i]->MCR;
                    g_i2cDataBase[i].backup_registers.SAR = g_i2cBases[i]->SAR;
                    g_i2cDataBase[i].backup_registers.TPR = g_i2cBases[i]->TPR;
                    g_i2cDataBase[i].backup_registers.TOR = g_i2cBases[i]->TOR;
                    g_i2cDataBase[i].backup_registers.IER = g_i2cBases[i]->IER;
                    g_i2cDataBase[i].backup_registers.IMR = g_i2cBases[i]->IMR;
                }
            }
            break;
        default:
            break;
    }

}

/**
  \fn        static void I2C_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
  \brief     Restore after exit from sleep.
             After recovering from SLPMAN_SLEEP1_STATE, I2C hareware is repowered, we restore user's configurations
             by aidding of the stored registers.

  \param[in] pdata pointer to user data, not used now
  \param[in] state low power state
 */
static void I2C_ExitLowPowerStateRestore(void* pdata, slpManLpState state)
{
    uint32_t i;

    switch (state)
    {
        case SLPMAN_SLEEP1_STATE:

            for(i = 0; i < I2C_INSTANCE_NUM; i++)
            {
                if(g_i2cDataBase[i].isInited == true)
                {
                    GPR_ClockEnable(g_i2cClocks[2*i]);
                    GPR_ClockEnable(g_i2cClocks[2*i+1]);

                    g_i2cBases[i]->SAR = g_i2cDataBase[i].backup_registers.SAR;
                    g_i2cBases[i]->TPR = g_i2cDataBase[i].backup_registers.TPR;
                    g_i2cBases[i]->TOR = g_i2cDataBase[i].backup_registers.TOR;
                    g_i2cBases[i]->IER = g_i2cDataBase[i].backup_registers.IER;
                    g_i2cBases[i]->IMR = g_i2cDataBase[i].backup_registers.IMR;
                    g_i2cBases[i]->MCR = g_i2cDataBase[i].backup_registers.MCR;

                }
            }
            break;

        default:
            break;
    }

}

#define  LOCK_SLEEP(instance)       do                                                                  \
                                    {                                                                   \
                                        g_i2cWorkingStatus |= (1U << instance);                         \
                                        slpManDrvVoteSleep(SLP_VOTE_I2C, SLP_ACTIVE_STATE);       \
                                    }                                                                   \
                                    while(0)

#define  CHECK_TO_UNLOCK_SLEEP(instance)    do                                                                  \
                                            {                                                                   \
                                                g_i2cWorkingStatus &= ~(1U << instance);                        \
                                                if(g_i2cWorkingStatus == 0)                                     \
                                                    slpManDrvVoteSleep(SLP_VOTE_I2C, SLP_SLP1_STATE);      \
                                            }                                                                   \
                                            while(0)
#endif

#define I2C_POLLING_TIMEOUT_CYCLES       1000000

/* Driver Version */
static const ARM_DRIVER_VERSION DriverVersion =
{
    ARM_I2C_API_VERSION,
    ARM_I2C_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_I2C_CAPABILITIES DriverCapabilities =
{
    1 /* supports 10-bit addressing */
};


#if (RTE_I2C0)

static I2C_CTRL I2C0_Ctrl = { 0 };
static PIN I2C0_pin_scl  = {RTE_I2C0_SCL_PAD_ID,   RTE_I2C0_SCL_FUNC};
static PIN I2C0_pin_sda  = {RTE_I2C0_SDA_PAD_ID,   RTE_I2C0_SDA_FUNC};

#if (RTE_I2C0_IO_MODE == DMA_MODE)
void HAL_I2C0_DmaTxEvent(uint32_t event);
void HAL_I2C0_DmaRxEvent(uint32_t event);

static I2C_DMA I2C0_DMA = {
                            RTE_I2C0_DMA_TX_CH,
                            RTE_I2C0_DMA_TX_REQID,
                            HAL_I2C0_DmaTxEvent,
                            RTE_I2C0_DMA_RX_CH,
                            RTE_I2C0_DMA_RX_REQID,
                            HAL_I2C0_DmaRxEvent
                          };
#endif

#if (RTE_I2C0_IO_MODE == IRQ_MODE)
void HAL_I2C0_IRQHandler(void);

static I2C_IRQ I2C0_IRQ = {
                            PXIC_I2c0_IRQn,
                            HAL_I2C0_IRQHandler
                          };

#endif

I2C_HandleTypeDef hi2c0 =
{
    I2C0,
    {
      &I2C0_pin_scl,
      &I2C0_pin_sda,
    },
#if (RTE_I2C0_IO_MODE == DMA_MODE)
    &I2C0_DMA,
#else
    NULL,
#endif

#if (RTE_I2C0_IO_MODE == IRQ_MODE)
    &I2C0_IRQ,
#else
    NULL,
#endif

    &I2C0_Ctrl
};
#endif

#if (RTE_I2C1)

static I2C_CTRL I2C1_Ctrl = { 0 };
static PIN I2C1_pin_scl  = {RTE_I2C1_SCL_PAD_ID,   RTE_I2C1_SCL_FUNC};
static PIN I2C1_pin_sda  = {RTE_I2C1_SDA_PAD_ID,   RTE_I2C1_SDA_FUNC};

#if (RTE_I2C1_IO_MODE == DMA_MODE)
void HAL_I2C1_DmaTxEvent(uint32_t event);
void HAL_I2C1_DmaRxEvent(uint32_t event);

static I2C_DMA I2C1_DMA = {
                            RTE_I2C1_DMA_TX_CH,
                            RTE_I2C1_DMA_TX_REQID,
                            HAL_I2C1_DmaTxEvent,
                            RTE_I2C1_DMA_RX_CH,
                            RTE_I2C1_DMA_RX_REQID,
                            HAL_I2C1_DmaRxEvent
                          };

#endif

#if (RTE_I2C1_IO_MODE == IRQ_MODE)

void HAL_I2C1_IRQHandler(void);

static I2C_IRQ I2C1_IRQ = {
                            PXIC_I2c1_IRQn,
                            HAL_I2C1_IRQHandler
                          };

#endif

I2C_HandleTypeDef hi2c1 =
{
    I2C1,
    {
      &I2C1_pin_scl,
      &I2C1_pin_sda,
    },
#if (RTE_I2C1_IO_MODE == DMA_MODE)
    &I2C1_DMA,
#else
    NULL,
#endif

#if (RTE_I2C1_IO_MODE == IRQ_MODE)
    &I2C1_IRQ,
#else
    NULL,
#endif
    &I2C1_Ctrl
};
#endif

/* Function prototypes  ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn static uint32_t HAL_I2C_GetInstanceNumber(I2C_HandleTypeDef *i2c)
 * \brief Get instance number.
 *
 * \param[in]  I2C_HandleTypeDef *i2c           I2C handle.
 * \param[out] none
 *
 * \retval Instance number.
 *******************************************************************/
static uint32_t HAL_I2C_GetInstanceNumber(I2C_HandleTypeDef *i2c);

 /*!******************************************************************
 * \fn static int32_t HAL_I2C_MasterCheckStatus(I2C_HandleTypeDef *i2c)
 * \brief Check status and clear any error if present
 *
 * \param[in]  I2C_HandleTypeDef *i2c           I2C handle.
 * \param[out] none
 *
 * \retval ARM_DRIVER_ERROR if any exception occurs, otherwise ARM_DRIVER_OK
 *******************************************************************/
static int32_t HAL_I2C_MasterCheckStatus(I2C_HandleTypeDef *i2c);

 /*!******************************************************************
 * \fn static int32_t HAL_I2C_GetClockFreq(I2C_HandleTypeDef *i2c)
 * \brief  Get i2c clock.
 *
 * \param[in]  I2C_HandleTypeDef *i2c           I2C handle.
 * \param[out] none
 *
 * \retval I2C clock value.
 *******************************************************************/
static int32_t HAL_I2C_GetClockFreq(I2C_HandleTypeDef *i2c);

/*!******************************************************************
 * \fn void HAL_I2C_DmaTxEvent(uint32_t event, I2C_HandleTypeDef *i2c)
 * \brief  I2C DMA TX Event handler.
 *
 * \param[in]  event                            DMA TX event.  
 * \param[in]  I2C_HandleTypeDef *i2c           I2C handle.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HAL_I2C_DmaTxEvent(uint32_t event, I2C_HandleTypeDef *i2c);

/*!******************************************************************
 * \fn static void HAL_I2C_DmaRxEvent(uint32_t event, I2C_HandleTypeDef *i2c)
 * \brief  I2C DMA RX Event handler.
 *
 * \param[in]  event                            DMA RX event.  
 * \param[in]  I2C_HandleTypeDef *i2c           I2C handle.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HAL_I2C_DmaRxEvent(uint32_t event, I2C_HandleTypeDef *i2c);

/* ---------------------------------------------------------------------------------------*/


// Local Function
/**
  \fn          ARM_DRIVER_VERSION I2C_GetVersion(void)
  \brief       Get driver version.
  \return      \ref ARM_DRIVER_VERSION
*/
ARM_DRIVER_VERSION ARM_I2C_GetVersion(void)
{
    return DriverVersion;
}


/**
  \fn          ARM_I2C_CAPABILITIES I2C_GetCapabilities(void)
  \brief       Get driver capabilities.
  \return      \ref ARM_I2C_CAPABILITIES
*/
ARM_I2C_CAPABILITIES I2C_GetCapabilities()
{
    return DriverCapabilities;
}

static uint32_t HAL_I2C_GetInstanceNumber(I2C_HandleTypeDef *i2c) {
    return ((uint32_t)i2c->reg - (uint32_t)I2C0) >> 16;
}

int32_t HAL_I2C_Initialize(ARM_I2C_SignalEvent_t cb_event, I2C_HandleTypeDef *i2c) {
    int32_t returnCode;
#ifdef PM_FEATURE_ENABLE
    uint32_t instance;
#endif

    pad_config_t padConfig;

    if(i2c->ctrl->flags & I2C_FLAG_INIT)
    {
        return ARM_DRIVER_OK;
    }

    //I2CDEBUG("I2C_Initialize\r\n");
    // Configure I2C Pins

    PAD_GetDefaultConfig(&padConfig);
    padConfig.mux = i2c->pins.pin_scl->funcNum;
    PAD_SetPinConfig(i2c->pins.pin_scl->pinNum, &padConfig);
    padConfig.mux = i2c->pins.pin_sda->funcNum;
    PAD_SetPinConfig(i2c->pins.pin_sda->pinNum, &padConfig);

    // Reset Run-Time information structure
    memset(i2c->ctrl, 0, sizeof(I2C_CTRL));

    i2c->ctrl->cb_event = cb_event;

    // Configure DMA if necessary
    if(i2c->dma)
    {
        DMA_Init();

        returnCode = DMA_OpenChannel();

        if (returnCode == ARM_DMA_ERROR_CHANNEL_ALLOC)
            return ARM_DRIVER_ERROR;
        else
            i2c->dma->tx_ch = returnCode;

        returnCode = DMA_OpenChannel();

        if (returnCode == ARM_DMA_ERROR_CHANNEL_ALLOC)
            return ARM_DRIVER_ERROR;
        else
            i2c->dma->rx_ch = returnCode;


        DMA_ChannelSetRequestSource(i2c->dma->tx_ch, (dma_request_source_t)i2c->dma->tx_req);
        DMA_ChannelRigisterCallback(i2c->dma->tx_ch, i2c->dma->tx_callback);


        DMA_ChannelSetRequestSource(i2c->dma->rx_ch, (dma_request_source_t)i2c->dma->rx_req);
        DMA_ChannelRigisterCallback(i2c->dma->rx_ch, i2c->dma->rx_callback);
    }

    i2c->ctrl->flags |= I2C_FLAG_INIT;

#ifdef PM_FEATURE_ENABLE

    instance = HAL_I2C_GetInstanceNumber(i2c);

    g_i2cDataBase[instance].isInited = true;

    g_i2cInitCounter++;

    if(g_i2cInitCounter == 1)
    {
        g_i2cWorkingStatus = 0;
        slpManRegisterPredefinedBackupCb(SLP_CALLBACK_I2C_MODULE, I2C_EnterLowPowerStatePrepare, NULL, SLPMAN_SLEEP1_STATE);
        slpManRegisterPredefinedRestoreCb(SLP_CALLBACK_I2C_MODULE, I2C_ExitLowPowerStateRestore, NULL, SLPMAN_SLEEP1_STATE);
    }
#endif

    return ARM_DRIVER_OK;
}

int32_t HAL_I2C_Uninitialize(I2C_HandleTypeDef *i2c) {
#ifdef PM_FEATURE_ENABLE
    uint32_t instance;
#endif

    pad_config_t padConfig;
    i2c->ctrl->flags = 0;
    i2c->ctrl->cb_event = NULL;

    // Unconfigure SCL and SDA pins
    PAD_GetDefaultConfig(&padConfig);
    padConfig.mux = PAD_MuxAlt0;

    PAD_SetPinConfig(i2c->pins.pin_scl->pinNum, &padConfig);
    PAD_SetPinConfig(i2c->pins.pin_sda->pinNum, &padConfig);
    // Input+pullup
    PAD_SetPinPullConfig(i2c->pins.pin_scl->pinNum, PAD_InternalPullUp);
    PAD_SetPinPullConfig(i2c->pins.pin_sda->pinNum, PAD_InternalPullUp);

#ifdef PM_FEATURE_ENABLE
    instance = HAL_I2C_GetInstanceNumber(i2c);
    g_i2cDataBase[instance].isInited = false;

    g_i2cInitCounter--;
    if(g_i2cInitCounter == 0)
    {
        g_i2cWorkingStatus = 0;
        slpManUnregisterPredefinedBackupCb(SLP_CALLBACK_I2C_MODULE);
        slpManUnregisterPredefinedRestoreCb(SLP_CALLBACK_I2C_MODULE);

    }
#endif

    return ARM_DRIVER_OK;
}

int32_t HAL_I2C_PowerControl(ARM_POWER_STATE state, I2C_HandleTypeDef *i2c) {
    uint32_t instance = HAL_I2C_GetInstanceNumber(i2c);

    switch (state)
    {
        case ARM_POWER_OFF:
            // I2C reset controller, including disable all I2C interrupts & clear fifo
            if(i2c->irq)
                XIC_DisableIRQ(i2c->irq->irq_num);

            // DMA disable
            if(i2c->dma)
            {
                DMA_StopChannel(i2c->dma->tx_ch, false);
                DMA_StopChannel(i2c->dma->rx_ch, false);
            }
            // Disable I2C and other control bits
            i2c->reg->MCR = 0;

            // Disable I2C power
            GPR_ClockDisable(g_i2cClocks[instance*2]);
            GPR_ClockDisable(g_i2cClocks[instance*2+1]);

            memset((void*)&i2c->ctrl->status, 0, sizeof(ARM_I2C_STATUS));

            i2c->ctrl->stalled = 0;
            i2c->ctrl->snum    = 0;
            i2c->ctrl->flags  &= ~I2C_FLAG_POWER;
            break;

        case ARM_POWER_LOW:
            return ARM_DRIVER_ERROR_UNSUPPORTED;

        case ARM_POWER_FULL:
            if((i2c->ctrl->flags & I2C_FLAG_INIT) == 0U)
            {
                return ARM_DRIVER_ERROR;
            }
            if(i2c->ctrl->flags & I2C_FLAG_POWER)
            {
                return ARM_DRIVER_OK;
            }

            // Enable power to i2c clock
            GPR_ClockEnable(g_i2cClocks[instance*2]);
            GPR_ClockEnable(g_i2cClocks[instance*2+1]);

            // Setup SDA setup/hold time parameter
            i2c->reg->TPR = ((0x4 << I2C_TPR_SDA_SETUP_TIME_Pos) | (0x4 << I2C_TPR_SDA_HOLD_TIME_Pos)
                                | (0x05 << I2C_TPR_SPIKE_FILTER_CNUM_Pos));

            // Enable I2C irq
            if(i2c->irq)
            {
                XIC_SetVector(i2c->irq->irq_num,i2c->irq->cb_irq);
                XIC_EnableIRQ(i2c->irq->irq_num);
            }

            i2c->ctrl->flags |= I2C_FLAG_POWER;
            break;

        default:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
    }
    return ARM_DRIVER_OK;
}

static int32_t HAL_I2C_MasterCheckStatus(I2C_HandleTypeDef *i2c) {
    int32_t ret = ARM_DRIVER_OK;

    volatile uint32_t isr_reg = i2c->reg->ISR;

    if(isr_reg & (I2C_ISR_ARBITRATATION_LOST_Msk | I2C_ISR_BUS_ERROR_Msk | I2C_ISR_RX_NACK_Msk))
        ret = ARM_DRIVER_ERROR;

    // if any exception occurs, clear it
    if(ret == ARM_DRIVER_ERROR)
    {
        if(isr_reg & I2C_ISR_ARBITRATATION_LOST_Msk)
            i2c->ctrl->status.arbitration_lost = 1;

        if(isr_reg & I2C_ISR_BUS_ERROR_Msk)
            i2c->ctrl->status.bus_error = 1;

        if(isr_reg & I2C_ISR_RX_NACK_Msk)
            i2c->ctrl->status.rx_nack = 1;

        i2c->reg->ISR = isr_reg;
        i2c->reg->SCR |= (I2C_SCR_FLUSH_TX_FIFO_Msk | I2C_SCR_FLUSH_RX_FIFO_Msk);
    }

    return ret;
}

int32_t HAL_I2C_MasterTransmit_IT(I2C_HandleTypeDef *hi2c, uint32_t addr, uint8_t *pTxData, uint32_t size) {
    uint32_t reg_value;
    uint32_t i = 0;
    int32_t ret = 0;

    if(!pTxData || !size || (addr > 0x3ff))
        return ARM_DRIVER_ERROR_PARAMETER;

    if(!(hi2c->ctrl->flags & I2C_FLAG_SETUP))
        return ARM_DRIVER_ERROR;

    // Enable interrupts to reflect specific status
    hi2c->reg->IER = (I2C_IER_TRANSFER_DONE_Msk |
                     I2C_IER_ARBITRATATION_LOST_Msk |
                     I2C_IER_DETECT_STOP_Msk |
                     I2C_IER_BUS_ERROR_Msk |
                     I2C_IER_RX_NACK_Msk);

    // Clear all flags first(W1C)
    hi2c->reg->ISR = hi2c->reg->ISR;
    // Config and issue command
    hi2c->reg->MCR = (I2C_MCR_CONTROL_MODE_Msk | I2C_MCR_I2C_EN_Msk);
    reg_value = (((addr << 1) & (I2C_SCR_TARGET_SLAVE_ADDR_Msk)) | ((size - 1) << I2C_SCR_BYTE_NUM_Pos) | I2C_SCR_START_Msk);
    
    hi2c->reg->SCR = reg_value;

    hi2c->ctrl->flags |= I2C_FLAG_MASTER_TX;

    // Send data
    while(i < size) {
        // Wait for Tx ready
        do {
            ret = HAL_I2C_MasterCheckStatus(hi2c);
        } while(((hi2c->reg->FSR & I2C_FSR_TX_FIFO_FREE_NUM_Msk) == 0) && (ret == ARM_DRIVER_OK));
        
        
        if(ret != ARM_DRIVER_OK) {
            hi2c->ctrl->status.busy = 0;
            return ARM_DRIVER_ERROR;
        }

        hi2c->reg->TDR = pTxData[i++];
    }

    // Wait for ACK/NACK
    do {
        ret = HAL_I2C_MasterCheckStatus(hi2c);
    } while((QCOM_FLD2VAL(I2C_FSR_TX_FIFO_FREE_NUM, hi2c->reg->FSR) != 0x10) && (ret == ARM_DRIVER_OK));
    

    if(ret != ARM_DRIVER_OK) {
        hi2c->ctrl->status.busy = 0;
        return ARM_DRIVER_ERROR;
    }

    while((hi2c->reg->ISR & I2C_ISR_DETECT_STOP_Msk) == 0);
    hi2c->reg->ISR = I2C_ISR_DETECT_STOP_Msk;
    
    hi2c->reg->IER = 0;
    hi2c->ctrl->status.busy = 0;

    return ARM_DRIVER_OK;
}

void HAL_I2C_MasterTransmit_Polling(I2C_HandleTypeDef *hi2c, uint32_t addr, const uint8_t *pTxData, uint32_t size) {
    uint32_t reg_value;
    int32_t ret = 0;
    uint32_t i = 0;

    // Config and issue command
    hi2c->reg->MCR = (I2C_MCR_CONTROL_MODE_Msk | I2C_MCR_I2C_EN_Msk);
    
    reg_value = (((addr << 1) & (I2C_SCR_TARGET_SLAVE_ADDR_Msk)) | ((size - 1) << I2C_SCR_BYTE_NUM_Pos) | I2C_SCR_START_Msk);
    hi2c->reg->SCR = reg_value;
    printf("Config and issue command\n");
    // Send data
    while(i < size) {
        
        // Wait for Tx ready
        do {
            ret = HAL_I2C_MasterCheckStatus(hi2c);
        } while(((hi2c->reg->FSR & I2C_FSR_TX_FIFO_FREE_NUM_Msk) == 0) && (ret == ARM_DRIVER_OK));
        
        if(ret != ARM_DRIVER_OK) {
            return;
        }

        hi2c->reg->TDR = pTxData[i];
        i++;
    }
    printf("Send data\n");
    // Wait for ACK/NACK
    do {
        ret = HAL_I2C_MasterCheckStatus(hi2c);
    } while((QCOM_FLD2VAL(I2C_FSR_TX_FIFO_FREE_NUM, hi2c->reg->FSR) != 0x10) && (ret == ARM_DRIVER_OK));

    if(ret != ARM_DRIVER_OK) {
        printf("Erro em envio de dados. Código: %d\n", ret);
        return;
    }

    while((hi2c->reg->ISR & I2C_ISR_DETECT_STOP_Msk) == 0);
    hi2c->reg->ISR = I2C_ISR_DETECT_STOP_Msk;

    hi2c->reg->IER = 0;
    hi2c->ctrl->status.busy = 0;
    printf("Wait for ACK/NACK\n");
}
#if 0 //not implemented yet
int32_t HAL_I2C_MasterReceive_DMA(I2C_HandleTypeDef *hi2c, uint32_t addr, uint8_t *pRxData, uint32_t size)  {
    dma_transfer_config dmaRxConfig = {(void *)&hi2c->reg->RDR, (void *)pRxData,
                                       DMA_FlowControlSource, DMA_AddressIncrementTarget,
                                       DMA_DataWidthOneByte,
                                       DMA_Burst8Bytes, size};

    if(!pRxData || !size || (addr > 0x3ff)) {
        return ARM_DRIVER_ERROR_PARAMETER;
    }

    if(!(hi2c->ctrl->flags & I2C_FLAG_SETUP)) {
        return ARM_DRIVER_ERROR;
    }

    if(hi2c->ctrl->status.busy) {
        return ARM_DRIVER_ERROR_BUSY;
    }

    // Enable interrupts to reflect specific status
    hi2c->reg->IER = (I2C_IER_TRANSFER_DONE_Msk |
                     I2C_IER_ARBITRATATION_LOST_Msk |
                     I2C_IER_DETECT_STOP_Msk |
                     I2C_IER_BUS_ERROR_Msk |
                     I2C_IER_RX_NACK_Msk);

    // Clear all flags first(W1C)
    hi2c->reg->ISR = hi2c->reg->ISR;

    hi2c->ctrl->flags |= I2C_FLAG_MASTER_RX;

    // Set control variables
    hi2c->ctrl->sla_rw = addr;
    hi2c->ctrl->pending = 1;
    hi2c->ctrl->data = (uint8_t *)pRxData;
    hi2c->ctrl->num  = size;
    hi2c->ctrl->cnt  = 0;

    // Update driver status
    hi2c->ctrl->status.busy = 1;
    // 0 :slave / 1 :master
    hi2c->ctrl->status.mode = 1;
    // 0 :tx  / 1 :rx
    hi2c->ctrl->status.direction = 0;
    hi2c->ctrl->status.arbitration_lost = 0;
    hi2c->ctrl->status.bus_error = 0;
    hi2c->ctrl->status.rx_nack = 0;

    if(hi2c->reg->STR & I2C_STR_BUSY_Msk) {
        return ARM_DRIVER_ERROR_BUSY;
    }

    hi2c->ctrl->flags |= I2C_FLAG_MASTER_RX;

#ifdef PM_FEATURE_ENABLE
    LOCK_SLEEP(instance);
#endif
    // Configure rx DMA and start it
    DMA_TransferSetup(hi2c->dma->rx_ch, &dmaRxConfig);
    DMA_EnableChannelInterrupts(hi2c->dma->rx_ch, DMA_EndInterruptEnable);
    
    DMA_StartChannel(hi2c->dma->rx_ch);

    return ARM_DRIVER_OK;
}

#endif

int32_t HAL_I2C_MasterReceive_IT(I2C_HandleTypeDef *hi2c, uint32_t addr, uint8_t *pRxData, uint32_t size) {
    uint32_t i = 0;
    uint32_t reg_value;

    if(!pRxData || !size || (addr > 0x3ff))
        return ARM_DRIVER_ERROR_PARAMETER;

    if(!(hi2c->ctrl->flags & I2C_FLAG_SETUP))
        return ARM_DRIVER_ERROR;

    // Enable interrupts to reflect specific status
    hi2c->reg->IER = (I2C_IER_TRANSFER_DONE_Msk |
                     I2C_IER_ARBITRATATION_LOST_Msk |
                     I2C_IER_DETECT_STOP_Msk |
                     I2C_IER_BUS_ERROR_Msk |
                     I2C_IER_RX_NACK_Msk);

    // Clear all flags first(W1C)
    hi2c->reg->ISR = hi2c->reg->ISR;

    hi2c->ctrl->flags |= I2C_FLAG_MASTER_RX;

    // Config and issue command
    hi2c->reg->MCR = (I2C_MCR_CONTROL_MODE_Msk | I2C_MCR_I2C_EN_Msk);
    reg_value = (((addr << 1) & (I2C_SCR_TARGET_SLAVE_ADDR_Msk)) | ((size - 1) << I2C_SCR_BYTE_NUM_Pos) | I2C_SCR_TARGET_RWN_Msk | I2C_SCR_START_Msk);

    hi2c->reg->SCR = reg_value;

    while (i < size) {
        while(!(hi2c->reg->FSR & I2C_FSR_RX_FIFO_DATA_NUM_Msk));
        pRxData[i] = hi2c->reg->RDR;
        i++;
    }

    // Wait for IDLE
    while(hi2c->reg->STR & I2C_STR_BUSY_Msk);
    hi2c->reg->IER = 0;
    hi2c->ctrl->status.busy = 0;
    
    return ARM_DRIVER_OK;
}

void HAL_I2C_MasterReceive_Polling(I2C_HandleTypeDef *hi2c, uint32_t addr, uint8_t *pRxData, uint32_t size) {
    uint32_t i = 0;
    uint32_t reg_value;
    int32_t ret;

    // Clear all flags first(W1C)
    hi2c->reg->ISR = hi2c->reg->ISR;
    
    // Config and issue command
    hi2c->reg->MCR = (I2C_MCR_CONTROL_MODE_Msk | I2C_MCR_I2C_EN_Msk);
    reg_value = (((addr << 1) & (I2C_SCR_TARGET_SLAVE_ADDR_Msk)) | ((size - 1) << I2C_SCR_BYTE_NUM_Pos) | I2C_SCR_TARGET_RWN_Msk | I2C_SCR_START_Msk);

    hi2c->reg->SCR = reg_value;

    while (i < size) {

        do {
            ret = HAL_I2C_MasterCheckStatus(hi2c);
        } while(((hi2c->reg->FSR & I2C_FSR_RX_FIFO_DATA_NUM_Msk) == 0) && (ret == ARM_DRIVER_OK));

        pRxData[i] = hi2c->reg->RDR;
        i++;
    }
    
}

static int32_t HAL_I2C_GetClockFreq(I2C_HandleTypeDef *i2c) {
    uint32_t instance = HAL_I2C_GetInstanceNumber(i2c);

    return GPR_GetClockFreq(g_i2cClocks[instance*2 + 1]);
}

void HAL_I2C_InitClock(HAL_I2C_Type i2c_id) {
    switch (i2c_id) {
    case HT_I2C0:
        GPR_ClockDisable(GPR_I2C0FuncClk);
        GPR_SetClockSrc(GPR_I2C0FuncClk, GPR_I2C0ClkSel_26M);    
        GPR_ClockEnable(GPR_I2C0FuncClk);
        GPR_SWReset(GPR_ResetI2C0Func);
        
        break;
    case HT_I2C1:
        GPR_ClockDisable(GPR_I2C1FuncClk);
        GPR_SetClockSrc(GPR_I2C1FuncClk, GPR_I2C1ClkSel_26M);    
        GPR_ClockEnable(GPR_I2C1FuncClk);
        GPR_SWReset(GPR_ResetI2C1Func);

        break;
    }
}

int32_t HAL_I2C_Control(uint32_t control, uint32_t arg, I2C_HandleTypeDef *i2c) {
    uint32_t val, clk;
    if(!(i2c->ctrl->flags & I2C_FLAG_POWER))
    {
        return ARM_DRIVER_ERROR;
    }
    switch(control)
    {
        case ARM_I2C_OWN_ADDRESS:
            // General call enable
            if(arg & ARM_I2C_ADDRESS_GC)
            {
                i2c->reg->SAR |= I2C_SAR_GENERAL_CALL_EN_Msk;
            }
            else
            {
                i2c->reg->SAR &= (~I2C_SAR_GENERAL_CALL_EN_Msk);
            }
            // Slave address mode 0: 7-bit mode 1: 10-bit mode
            if(arg & ARM_I2C_ADDRESS_10BIT)
            {
                i2c->reg->SAR |= I2C_SAR_SLAVE_ADDR_MODE_Msk;
                val = arg & 0x3FF;
            }
            else
            {
                i2c->reg->SAR &= (~I2C_SAR_SLAVE_ADDR_MODE_Msk);
                val = arg & 0x7F;
            }
            // Slave address
            i2c->reg->SAR |= val;
            // Enable slave address
            i2c->reg->SAR |= I2C_SAR_SLAVE_ADDR_EN_Msk;
            break;

        case ARM_I2C_BUS_SPEED:
            clk = HAL_I2C_GetClockFreq(i2c);
            switch(arg)
            {
                case ARM_I2C_BUS_SPEED_STANDARD:
                    // 100kHz
                    clk /= 100000U;
                    break;
                case ARM_I2C_BUS_SPEED_FAST:
                    // 400kHz
                    clk /= 400000U;
                    break;
                case ARM_I2C_BUS_SPEED_FAST_PLUS:
                    // 1MHz
                    clk /= 1000000U;
                    break;
                default:
                    return ARM_DRIVER_ERROR_UNSUPPORTED;
            }
            i2c->reg->TPR |= (((clk / 2) << I2C_TPR_SCLH_Pos) | ((clk / 2 ) << I2C_TPR_SCLL_Pos));
            
            // Speed configured
            i2c->ctrl->flags |= I2C_FLAG_SETUP;
            break;

        case ARM_I2C_BUS_CLEAR:
            break;

        case ARM_I2C_ABORT_TRANSFER:
            break;

        default:
            return ARM_DRIVER_ERROR_UNSUPPORTED;
    }
    return ARM_DRIVER_OK;
}

void I2C_IRQHandler(I2C_HandleTypeDef *i2c) {
    uint32_t tmp_status = 0;

    tmp_status = i2c->reg->ISR;

    //i2c->reg->ISR = tmp_status;

    if(tmp_status & I2C_ISR_TRANSFER_DONE_Msk) {
        // //I2CDEBUG("I2C_IRQHandler transfer done\r\n");

        i2c->ctrl->status.busy = 0;

        if(i2c->ctrl->cb_event) { 
            if(i2c->ctrl->flags & I2C_FLAG_MASTER_TX) {
                _tx_callback = 1;
                i2c->ctrl->flags &= ~(I2C_FLAG_MASTER_TX);
            
            } else if(i2c->ctrl->flags & I2C_FLAG_MASTER_RX) {
                _rx_callback = 1;
                i2c->ctrl->flags &= ~(I2C_FLAG_MASTER_RX);
            
            } else if(i2c->ctrl->flags & I2C_FLAG_SLAVE_TX) {
                _tx_callback = 1;
                i2c->ctrl->flags &= ~(I2C_FLAG_SLAVE_TX);
                
            } else if(i2c->ctrl->flags & I2C_FLAG_SLAVE_RX) {
                _rx_callback = 1;
                i2c->ctrl->flags &= ~(I2C_FLAG_SLAVE_RX);

            } else if(_tx_callback)  {
                _tx_callback = 0;
                i2c->ctrl->cb_event(ARM_I2C_EVENT_TX_DONE);
    
            } else if(_rx_callback) {
                _rx_callback = 0;
                i2c->ctrl->cb_event(ARM_I2C_EVENT_RX_DONE);
            } 
        }
    }
    if(tmp_status & I2C_ISR_TX_FIFO_EMPTY_Msk)
    {
        //I2CDEBUG("I2C_IRQHandler tx fifo empty\r\n");
    }
    if(tmp_status & I2C_ISR_RX_FIFO_FULL_Msk)
    {
        //I2CDEBUG("I2C_IRQHandler rx fifo full\r\n");
    }
    if(tmp_status & I2C_IER_RX_ONE_DATA_Msk)
    {
        //I2CDEBUG("I2C_IRQHandler rx one data\r\n");
        #if 0
        if(i2c->ctrl->num > i2c->ctrl->cnt)
        {
            temp_data = i2c->reg->RDR;
            //I2CDEBUG("recv data=%d\n", temp_data);
            *(i2c->ctrl->data++) = temp_data;
            i2c->ctrl->cnt++;
        }
        #endif
    }
    if(tmp_status & I2C_IER_TX_ONE_DATA_Msk)
    {
        //I2CDEBUG("I2C_IRQHandler tx one data\r\n");
        #if 0
        if(i2c->ctrl->num > i2c->ctrl->cnt)
        {
            if(i2c->ctrl->data)
            {
                // If data available
                temp_data = *(i2c->ctrl->data++);
            }
            //I2CDEBUG("send data=%d\n", temp_data);
            i2c->reg->I2CTDR = temp_data;            // Activate send
            i2c->ctrl->cnt++;
        }
        #endif
    }
}

static void HAL_I2C_DmaTxEvent(uint32_t event, I2C_HandleTypeDef *i2c) {
#ifdef PM_FEATURE_ENABLE
    uint32_t instance = HAL_I2C_GetInstanceNumber(i2c);
#endif

    switch(event)
    {
        case DMA_EVENT_END:

            //I2CDEBUG("HAL_I2C_DmaTxEvent\n");
            i2c->ctrl->cnt= i2c->ctrl->num;
            i2c->ctrl->status.busy = 0U;
            if(i2c->ctrl->cb_event)
            {
                i2c->ctrl->cb_event(ARM_I2C_EVENT_TX_DONE);
            }
#ifdef PM_FEATURE_ENABLE
            CHECK_TO_UNLOCK_SLEEP(instance);
#endif
            break;
        case DMA_EVENT_ERROR:
        default:
            break;
    }
}

static void HAL_I2C_DmaRxEvent(uint32_t event, I2C_HandleTypeDef *i2c) {
#ifdef PM_FEATURE_ENABLE
    uint32_t instance = HAL_I2C_GetInstanceNumber(i2c);
#endif

    switch(event) {
        case DMA_EVENT_END:
            i2c->ctrl->cnt= i2c->ctrl->num;
            i2c->ctrl->status.busy = 0U;
            if(i2c->ctrl->cb_event) {
                i2c->ctrl->cb_event(ARM_I2C_EVENT_RX_DONE);
            }
#ifdef PM_FEATURE_ENABLE
            CHECK_TO_UNLOCK_SLEEP(instance);
#endif
            break;
        case DMA_EVENT_ERROR:
        default:
            break;
    }
}

#if (RTE_I2C0)

void HAL_I2C0_IRQHandler(void) {
    I2C_IRQHandler(&hi2c0);
}

void HAL_I2C0_DmaTxEvent(uint32_t event) {
    HAL_I2C_DmaTxEvent(event, &hi2c0);
}

void HAL_I2C0_DmaRxEvent(uint32_t event) {
    HAL_I2C_DmaRxEvent(event, &hi2c0);
}

#endif

#if (RTE_I2C1)

void HAL_I2C1_IRQHandler(void) {
    I2C_IRQHandler(&hi2c1);
}

void HAL_I2C1_DmaTxEvent(uint32_t event) {
    HAL_I2C_DmaTxEvent(event, &hi2c1);
}

void HAL_I2C1_DmaRxEvent(uint32_t event) {
    HAL_I2C_DmaRxEvent(event, &hi2c1);
}

#endif

/************************ HT Micron Semicondutores S.A *****END OF FILE****/

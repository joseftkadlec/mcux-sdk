/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_lpspi.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_LPSPI_MASTER_BASEADDR         LPSPI0
#define EXAMPLE_LPSPI_MASTER_CLOCK_NAME       kCLOCK_Lpspi0
#define LPSPI_MASTER_CLK_FREQ                 (CLOCK_GetIpFreq(EXAMPLE_LPSPI_MASTER_CLOCK_NAME))
#define EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE     (kCLOCK_IpSrcFircAsync)
#define EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT     kLPSPI_Pcs3
#define EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER kLPSPI_MasterPcs3

#define TRANSFER_SIZE     64U     /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 500000U /*! Transfer baudrate - 500k */

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t masterRxData[TRANSFER_SIZE] = {0U};
uint8_t masterTxData[TRANSFER_SIZE] = {0U};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

int main(void)
{
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    /*Set clock source for LPSPI and get master clock source*/
    CLOCK_SetIpSrc(EXAMPLE_LPSPI_MASTER_CLOCK_NAME, EXAMPLE_LPSPI_MASTER_CLOCK_SOURCE);

    PRINTF("LPSPI 3-Wire board to board polling example.\r\n");
    PRINTF("This example use one board as master and another as slave.\r\n");
    PRINTF("Master uses polling way and slave uses interrupt way. \r\n");
    PRINTF("Please make sure you make the correct line connection. Basically, the connection is: \r\n");
    PRINTF("LPSPI_master --  LPSPI_slave   \r\n");
    PRINTF("   CLK       --    CLK  \r\n");
    PRINTF("   PCS       --    PCS \r\n");
    PRINTF("   SOUT      --    SIN  \r\n");
    PRINTF("   GND       --    GND \r\n");

    uint32_t srcClock_Hz;
    uint32_t errorCount;
    uint32_t loopCount = 1U;
    uint32_t i;
    lpspi_master_config_t masterConfig;
    lpspi_transfer_t masterXfer;

    /*Master config*/
    LPSPI_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate = TRANSFER_BAUDRATE;
    masterConfig.whichPcs = EXAMPLE_LPSPI_MASTER_PCS_FOR_INIT;
    masterConfig.pinCfg   = kLPSPI_SdoInSdoOut;

    srcClock_Hz = LPSPI_MASTER_CLK_FREQ;
    LPSPI_MasterInit(EXAMPLE_LPSPI_MASTER_BASEADDR, &masterConfig, srcClock_Hz);

    while (1)
    {
        /* Set up the transfer data */
        for (i = 0U; i < TRANSFER_SIZE; i++)
        {
            masterTxData[i] = (i + loopCount) % 256U;
            masterRxData[i] = 0U;
        }

        /* Print out transmit buffer */
        PRINTF("\r\n Master transmit:\r\n");
        for (i = 0U; i < TRANSFER_SIZE; i++)
        {
            /* Print 16 numbers in a line */
            if ((i & 0x0FU) == 0U)
            {
                PRINTF("\r\n");
            }
            PRINTF(" 0x%02X", masterTxData[i]);
        }
        PRINTF("\r\n");

        /*Start master transfer, transfer data to slave.*/
        masterXfer.txData      = masterTxData;
        masterXfer.rxData      = NULL;
        masterXfer.dataSize    = TRANSFER_SIZE;
        masterXfer.configFlags = EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterByteSwap;

        LPSPI_MasterTransferBlocking(EXAMPLE_LPSPI_MASTER_BASEADDR, &masterXfer);

        /* Prepare for receiving data from slave board. */
        PRINTF("\r\nMaster transmit data complete! Please input any character to continue!");
        PRINTF("\r\nBefore you input any character, please make sure the slave board is ready to transmit data.\r\n");
        GETCHAR();

        /* Start master transfer, receive data from slave */
        masterXfer.txData      = NULL;
        masterXfer.rxData      = masterRxData;
        masterXfer.dataSize    = TRANSFER_SIZE;
        masterXfer.configFlags = EXAMPLE_LPSPI_MASTER_PCS_FOR_TRANSFER | kLPSPI_MasterByteSwap;
        LPSPI_MasterTransferBlocking(EXAMPLE_LPSPI_MASTER_BASEADDR, &masterXfer);

        errorCount = 0U;
        for (i = 0U; i < TRANSFER_SIZE; i++)
        {
            if (masterTxData[i] != masterRxData[i])
            {
                errorCount++;
            }
        }

        if (errorCount == 0U)
        {
            PRINTF(" \r\nLPSPI transfer all data matched! \r\n");

            /* Print out receive buffer */
            PRINTF("\r\n Master received:\r\n");
            for (i = 0U; i < TRANSFER_SIZE; i++)
            {
                /* Print 16 numbers in a line */
                if ((i & 0x0FU) == 0U)
                {
                    PRINTF("\r\n");
                }
                PRINTF(" 0x%02X", masterRxData[i]);
            }
            PRINTF("\r\n");
        }
        else
        {
            PRINTF("\r\nError occurred in LPSPI transfer ! \r\n");
        }

        /* Wait for press any key */
        PRINTF("\r\nPress any key to run again\r\n");
        GETCHAR();

        /* Increase loop count to change transmit buffer */
        loopCount++;
    }
}
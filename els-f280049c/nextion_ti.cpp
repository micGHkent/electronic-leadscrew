/*
 * nextion_ti.cpp
 *
 *  Created on: Oct 18, 2020
 *      Author: kent
 */

// ===========================================================================
// The support function below are from or based on TI's C2000Ware examples.
// Most are from sci_ex1_echoback.c, and below is the associated TI license.
// ===========================================================================
//
//#############################################################################
// $TI Release: F28002x Support Library v3.02.00.00 $
// $Release Date: Tue May 26 17:23:28 IST 2020 $
// $Copyright:
// Copyright (C) 2020 Texas Instruments Incorporated - http://www.ti.com/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
//
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the
//   distribution.
//
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// $
//#############################################################################

#include "F28x_Project.h"

#include <file.h>
#include "nextion_ti.h"


//#if NEXTION_DEBUG
//
// scia_init - SCIA  8-bit word, baud rate 0x001A, default, 1 STOP bit,
// no parity
//
void scia_init()
{
    //
    // Note: Clocks were turned on to the SCIA peripheral
    // in the InitSysCtrl() function
    //

    //
    // 1 stop bit,  No loopback, No parity,8 char bits, async mode,
    // idle-line protocol
    //
    SciaRegs.SCICCR.all = 0x0007;

    //
    // enable TX, RX, internal SCICLK, Disable RX ERR, SLEEP, TXWAKE
    //
    SciaRegs.SCICTL1.all = 0x0003;

    SciaRegs.SCICTL2.bit.TXINTENA = 1;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1;

    //
    // 57600 baud @LSPCLK = 25MHz (100 MHz SYSCLK)
    //
    SciaRegs.SCIHBAUD.all = 0x00;
    SciaRegs.SCILBAUD.all = 0x1B;

    //
    // Relinquish SCI from Reset
    //
    SciaRegs.SCICTL1.all = 0x0023;

    return;
}
//#endif

//
// scib_init - SCIB  8-bit word, baud rate 0x001A, default, 1 STOP bit,
// no parity
//
void scib_init()
{
    //
    // Note: Clocks were turned on to the SCIB peripheral
    // in the InitSysCtrl() function
    //

    //
    // 1 stop bit,  No loopback, No parity,8 char bits, async mode,
    // idle-line protocol
    //
    ScibRegs.SCICCR.all = 0x0007;

    //
    // enable TX, RX, internal SCICLK, Disable RX ERR, SLEEP, TXWAKE
    //
    ScibRegs.SCICTL1.all = 0x0003;

    ScibRegs.SCICTL2.bit.TXINTENA = 1;
    ScibRegs.SCICTL2.bit.RXBKINTENA = 1;

    // Calculate baud rate bits using BRR = LPSCLK / ((Baud rate + 1) * 8)
    // Round to
#if 0
    //
    // 9600 baud @LSPCLK = 25MHz (100 MHz SYSCLK)
    //
    ScibRegs.SCIHBAUD.all = 0x01;
    ScibRegs.SCILBAUD.all = 0x46;
#else
    //
    // 38400 baud @LSPCLK = 25MHz (100 MHz SYSCLK)
    //
    ScibRegs.SCIHBAUD.all = 0x00;
    ScibRegs.SCILBAUD.all = 0x51;
#endif

    //
    // Relinquish SCI from Reset
    //
    ScibRegs.SCICTL1.all = 0x0023;

    return;
}

//#if NEXTION_DEBUG
#if 0
//
// transmitSCIAChar - Transmit a character from the SCI
//
static void transmitSCIAChar(uint16_t a)
{
    while (SciaRegs.SCIFFTX.bit.TXFFST != 0)
    {

    }
    SciaRegs.SCITXBUF.all = a;
}

//
// transmitSCIAMessage - Transmit message via SCIA
//
static void transmitSCIAMessage(const unsigned char *msg)
{
    int i;
    i = 0;
    while (msg[i] != '\0')
    {
        transmitSCIAChar(msg[i]);
        i++;
    }
}

//
// initSCIAFIFO - Initialize the SCI FIFO
//
static void initSCIAFIFO(void)
{
    SciaRegs.SCIFFTX.all = 0xE040;
    SciaRegs.SCIFFRX.all = 0x2044;
    SciaRegs.SCIFFCT.all = 0x0;
}
#endif
//#endif

//
// transmitSCIAChar - Transmit a character from the SCI
//
void transmitSCIBChar(uint16_t a)
{
    while (ScibRegs.SCIFFTX.bit.TXFFST != 0)
    {

    }
    ScibRegs.SCITXBUF.all = a;
}

//
// transmitSCIAMessage - Transmit message via SCIB
//
void transmitSCIBMessage(const unsigned char *msg, int nn)
{
    if (nn == -1) {
        int i;
        i = 0;
        while (msg[i] != '\0')
        {
            transmitSCIBChar(msg[i]);
            i++;
//            DELAY_US(265);
        }
    } else {
        for(int i=0; i<nn; i++) {
            transmitSCIBChar(msg[i]);
//            DELAY_US(265);
        }
    }
}

//
// initSCIBFIFO - Initialize the SCI FIFO
//
void initSCIBFIFO(void)
{
    ScibRegs.SCIFFTX.all = 0xE040;
    ScibRegs.SCIFFRX.all = 0x2044;
    ScibRegs.SCIFFCT.all = 0x0;
}

// ===========================================================================

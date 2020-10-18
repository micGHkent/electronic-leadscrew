/*
 * nextion.cpp
 *
 *  Created on: Aug 25, 2020
 *      Author: Kent A. Vander Velden (kent.vandervelden@gmail.com)
 *
 *  Summary: The routines here translate between the existing seven
 *  segment display and the Nextion, leaving as much code untouched
 *  as possible. Two UARTS are used, SCIA for virtual COM debugging
 *  and SCIB which connects to the Nextion. An additional GPIO pin
 *  is used as a limit switch input, which may be useful as a hard
 *  limit to profile or thread up to a shoulder.
 *
 *  Other than the TI routines at the end of this file, which have
 *  their own terms, the code is released to the public domain
 *  "as is" and is unsupported.
 *
 */

#include "F28x_Project.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <file.h>
#include "driverlib.h"
#include "device.h"
#include "nextion.h"

typedef unsigned char uchar_t;

// Set to 1 to enable debugging of the Nextion messages over the
// virtual COM port (57600, 8N1)
#define NEXTION_DEBUG 1

#if NEXTION_DEBUG
#include <ctype.h>
#include "launchxl_ex1_sci_io.h"

static void scia_init();
//static void transmitSCIAChar(uint16_t a);
//static void transmitSCIAMessage(const unsigned char *msg);
//static void initSCIAFIFO(void);
#endif

static void scib_init();
static void transmitSCIBChar(uint16_t a);
static void transmitSCIBMessage(const unsigned char *msg);
static void initSCIBFIFO(void);

Nextion::Nextion() :
    rpm_(0),
    enabled_(false),
    alarm_(false),
    at_stop_(false),
    feed_(.005),
    mode_metric_(false),
    mode_feed_(true),
    reverse_(false)
{
    strcpy(feed_str_, ".005");
    strcpy(feed_str_new_, ".005");
    //    init();
}

int Nextion::read(uchar_t buf[], const int nmax)
{
    int n = 0;

    while (n < nmax && ScibRegs.SCIFFRX.bit.RXFFST)
    {
        uint16_t ReceivedChar = ScibRegs.SCIRXBUF.all;
        buf[n++] = ReceivedChar;

        // This delay is done to increase chances that a complete message from
        // the Nextion will be received in one function call. To eliminate the
        // delay, add memory to the Nextion routines, continuing to
        // read and tokenize per call until a valid message is gathered.
        // ~208us to transmit 8-bits at 38.4kBaud
        DELAY_US(500);
    }

#if NEXTION_DEBUG
    if (n > 0)
    {
        printf("%d:", n);
        for (int i = 0; i < n; i++)
        {
            printf(" %02x", buf[i]);
        }
        putchar('\r');
        putchar('\n');
    }
#endif

    return n;
}

void Nextion::send(const uchar_t *msg)
{
    transmitSCIBMessage((const unsigned char*) msg);

#if NEXTION_DEBUG
    {
        const uchar_t *p;
        printf("Send: ");
        p = msg;
        while(*p != '\0') {
            if(isprint(*p)) {
              putchar(*p);
            } else {
              putchar(0xff);
            }
            p++;
        }
        printf(" : ");
        p = msg;
        while(*p != '\0') {
            printf(" %02x", *p);
            p++;
        }
        putchar('\r');
        putchar('\n');
    }
#endif
}

void Nextion::init()
{
    // Configure the GPIO pin for the limit switch input
    // GPIO_setPinConfig(GPIO_25_GPIO25);
    GPIO_SetupPinMux(25, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(25, GPIO_INPUT, GPIO_OPENDRAIN | GPIO_PULLUP);

#if NEXTION_DEBUG
    GPIO_setPinConfig(GPIO_28_SCIRXDA);
    GPIO_setPinConfig(GPIO_29_SCITXDA);

    GPIO_setQualificationMode(28, GPIO_QUAL_ASYNC);

    scia_init();
#endif

    GPIO_setPinConfig(GPIO_13_SCIRXDB);
    GPIO_setPinConfig(GPIO_40_SCITXDB);

    GPIO_setPadConfig(13, GPIO_PIN_TYPE_PULLUP);
    // GPIO_setPadConfig(40, GPIO_PIN_TYPE_PULLUP);

    GPIO_setQualificationMode(13, GPIO_QUAL_ASYNC);

    scib_init();

    initSCIBFIFO();

#if NEXTION_DEBUG
    // To help with debugging, configure the UART that is connected to
    // USB port, the virtual terminal, to be stdout.
    volatile int status = 0;
    status = add_device("scia", _SSA, SCI_open, SCI_close, SCI_read, SCI_write,
                        SCI_lseek, SCI_unlink, SCI_rename);
    volatile FILE *fid = fopen("scia", "w");
    freopen("scia:", "w", stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
#endif
}

// Wait for the Nextion to become ready.
void Nextion::wait()
{
    // The easiest way to while for the Nextion to be ready with a fixed delay
    // to have a fixed delay.
    //   DELAY_US(250000);
    // But the required time, while short, is unspecified. Instead, wait for
    // the Nextion to send a ready message, eventually timing out if necessary.
    //
    // Nextion will send
    //   0x00 0x00 0x00 0xff 0xff 0xff
    // on start up, and
    //   0x88 0xff 0xff 0xff
    // when ready. Often both messages will be read in a single
    // read(...) call
    //
    // The Nextion is ready fast and I rarely saw the ready message unless
    // power cycling only the Nextion. It may be better to test if the Nextion
    // is ready by checking its response to a query like which page is current.

    // Timeout after 40 * 25us = 1s
    for (int i = 0; i < 40; i++)
    {
        const int nmax = 6;
        uchar_t msg[nmax];
        int n = read(msg, nmax);
        if (n > 3)
        {
            bool has_end = msg[n - 3] == 0xff && msg[n - 2] == 0xff
                    && msg[n - 1] == 0xff;
            if (n >= 4 && msg[n - 4] == 0x88 && has_end)
            {
                break;
            }
        }
        DELAY_US(25000);
    }

    set_feed(".005");
    set_rpm(rpm_);

    set_diagram();
    set_units();
    set_sign();
}

void Nextion::set_feed(const char *f)
{
    float f2 = strtof(f, NULL);

    {
        uchar_t msg2[8 + 4 + 5];
        uchar_t *p = msg2 + sprintf((char*)msg2, "t1.txt=\"%s\"", f);
        *p++ = '\xff';
        *p++ = '\xff';
        *p++ = '\xff';
        *p++ = '\0';
        send(msg2);
    }

    feed_ = f2;
#if NEXTION_DEBUG
    printf("aa %d\r\n", int(feed_*10000));
#endif
}


void Nextion::set_rpm(Uint16 rpm)
{
    static bool do_once = true;

    if (rpm_ != rpm || do_once)
    {
        uchar_t msg2[32];
        sprintf((char*) msg2, "t0.txt=\"%u\"\xff\xff\xff", rpm);
        send(msg2);

        rpm_ = rpm;
        do_once = false;
    }

    if (false) {
        int m = 0;
        int M = 3000;
        int h = 160;
//        rpm=3000;
        uchar_t nex_buffer[32];
        int v = (int)(float(rpm - m) / float(M - m) * float(h));
        if (v < 0) v = 0;
        if (v > h) v = h;
        sprintf((char*)nex_buffer, "add 28,0,%d\xff\xff\xff", v);
        send(nex_buffer);
    }
}

bool Nextion::update(Uint16 rpm, bool alarm, bool enabled)
{
    set_rpm(rpm);

    bool updated = false;

    static bool do_once = true;
    updated = updated || do_once;

    bool p_enabled = enabled_;

    updated = updated || alarm_ != alarm;
    alarm_ = alarm;

    // Check the limit switch and update the enabled state
    at_stop_ = GPIO_ReadPin(25);
    enabled_ = enabled_ && enabled && !at_stop_;

    // Receive message from Nextion display
    const int nmax = 12;
    uchar_t msg[nmax];
    int n = read(msg, nmax);

    if (n > 3)
    {
        bool has_end = msg[n - 3] == 0xff && msg[n - 2] == 0xff
                && msg[n - 1] == 0xff;

        if (has_end && (n == 10 || n == 4) && msg[n - 4] == 0x88)
        {
            // Reinitialize the screen if the Nextion resets, through a flag
            // passed back to UserInterface the feed information is reinitialized.
            do_once = true;
#if NEXTION_DEBUG
            printf("Reinitialize\r\n");
#endif
        }
        else if (has_end && n == 5 && msg[0] == 0x25)
        {
            // Touch Event
            uchar_t k = msg[1];

            const char max_n = 6;
            static char f[max_n+1] = {'\0'};
            int n = strlen(f);
            bool feed_updated = false;
            static bool in_edit = false;

            if (0x30 <= k && k <= 0x39) // number, 0-9
            {
                if (n < max_n) {
                    f[n++] = k;
                    f[n] = '\0';
                    feed_updated = true;
                }
                in_edit = true;
            }
            else if (k == 0x2e) // .
            {
                if (n < max_n) {
                    bool has_dot = false;
                    for(int i=0; i<n; i++) {
                        if (f[i] == '.') {
                            has_dot = true;
                            break;
                        }
                    }
                    if (!has_dot) {
                        f[n++] = k;
                        f[n] = '\0';
                        feed_updated = true;
                    }
                }
                in_edit = true;
            }
            else if (k == 0x08) // backspace
            {
                if (n > 0) {
                    f[n-1] = '\0';
                    feed_updated = true;
                }
                in_edit = true;
            }
            else if (k == 0x0d) // enter
            {
                f[0] = '\0';
                feed_updated = false;
                in_edit = false;
            }
            else if (k == 0x1a) // mode/dir change
            {
                updated = true;

                if (!mode_feed_ && !reverse_) {
                    mode_feed_ = true;
                } else if (mode_feed_ && !reverse_) {
                    mode_feed_ = false;
                    reverse_ = true;
                } else if (!mode_feed_ && reverse_) {
                    mode_feed_ = true;
                } else if (mode_feed_ && reverse_) {
                    mode_feed_ = false;
                    reverse_ = false;
                }
                set_diagram();
                set_units();
            }
            else if (k == 0x1b) // units change
            {
                mode_metric_ = !mode_metric_;
                updated = true;

                set_units();
            }
            else if (k == 0x1c) // start/stop
            {
                enabled_ = !enabled_;
            }
            else if (k == 0x1d) // nothing
            {
                in_edit = false;
            }
            else if (k == 0x1e) // cancel
            {
                in_edit = false;
            }

            if (feed_updated) {
                updated = true;
                set_feed(f);
            }

            if (in_edit) {
                send("t1.pco=13812\xff\xff\xff");
            } else {
                send("t1.pco=65535\xff\xff\xff");
            }
        }
    }

    // Remain disabled if limit switch is tripped, else toggle enable.
    //                    enabled_ = !at_stop_ && !enabled_;

    // Set credits message once
    if (do_once)
    {
        const uchar_t *msg = { "t2.txt=\"ELS 1.3.01\r\n"
                               "James Clough (Clough42)\r\n"
                               "\r\n"
                               "Touchscreen interface\r\n"
                               "Kent A. Vander Velden"
                               "\"\xff\xff\xff" };
        send(msg);
    }

    // Update alarm indicator
    {
        static bool p_alarm = true;
        const uchar_t *msgs[2] = { "r1.val=0\xff\xff\xff",
                                   "r1.val=1\xff\xff\xff" };

        if (p_alarm != alarm || do_once)
        {
            // In this fast loop, update the display only if needed to avoid flicker.
            if (alarm) {
                send("vis 4,0\xff\xff\xff");
                send("vis 25,1\xff\xff\xff");
            } else {
                send("vis 25,0\xff\xff\xff");
            }
            send(msgs[alarm ? 1 : 0]);
            p_alarm = alarm;
        }
    }

    // Update the enable/disable button
    if (p_enabled != enabled_ || do_once)
    {
        set_sign();
    }

//    bool init = do_once;
    do_once = false;

    return updated;
}

void Nextion::getFeed(float &v, bool &metric, bool &feed) const {
    v = feed_;
    metric = mode_metric_;
    feed = mode_feed_;
#if NEXTION_DEBUG
    printf("bb %d\r\n", int(feed_*10000));
#endif
}

bool Nextion::isAtStop() const {
    return at_stop_;
}

bool Nextion::isEnabled() const {
    return enabled_;
}

bool Nextion::isReverse() const {
    return reverse_;
}

void Nextion::set_diagram() {
    if (!mode_feed_ && !reverse_) {
        send("p0.pic=5\xff\xff\xff");
    } else if (mode_feed_ && !reverse_) {
        send("p0.pic=4\xff\xff\xff");
    } else if (!mode_feed_ && reverse_) {
        send("p0.pic=3\xff\xff\xff");
    } else if (mode_feed_ && reverse_) {
        send("p0.pic=2\xff\xff\xff");
    }
}

void Nextion::set_units() {
    if (mode_metric_ && mode_feed_) {
        send("p1.pic=8\xff\xff\xff"); // mm/rev
    } else if (mode_metric_ && !mode_feed_) {
        send("p1.pic=7\xff\xff\xff"); // mm
    } else if (!mode_metric_ && mode_feed_) {
        send("p1.pic=6\xff\xff\xff"); // in/rev
    } else if (!mode_metric_ && !mode_feed_) {
        send("p1.pic=9\xff\xff\xff"); // TPI
    }
}

void Nextion::set_sign() {
    if (enabled_) {
        send("p2.pic=11\xff\xff\xff");
    } else {
        send("p2.pic=10\xff\xff\xff");
    }
}


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

#if NEXTION_DEBUG
//
// scia_init - SCIA  8-bit word, baud rate 0x001A, default, 1 STOP bit,
// no parity
//
static void scia_init()
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
#endif

//
// scib_init - SCIB  8-bit word, baud rate 0x001A, default, 1 STOP bit,
// no parity
//
static void scib_init()
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

#if NEXTION_DEBUG
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
#endif

//
// transmitSCIAChar - Transmit a character from the SCI
//
static void transmitSCIBChar(uint16_t a)
{
    while (ScibRegs.SCIFFTX.bit.TXFFST != 0)
    {

    }
    ScibRegs.SCITXBUF.all = a;
}

//
// transmitSCIAMessage - Transmit message via SCIB
//
static void transmitSCIBMessage(const unsigned char *msg)
{
    int i;
    i = 0;
    while (msg[i] != '\0')
    {
        transmitSCIBChar(msg[i]);
        i++;
    }
}

//
// initSCIBFIFO - Initialize the SCI FIFO
//
static void initSCIBFIFO(void)
{
    ScibRegs.SCIFFTX.all = 0xE040;
    ScibRegs.SCIFFRX.all = 0x2044;
    ScibRegs.SCIFFCT.all = 0x0;
}

// ===========================================================================

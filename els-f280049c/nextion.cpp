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
 *  Other than the TI routines in other files, which have
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
#include "nextion_ti.h"

typedef unsigned char uchar_t;

// Set to 1 to enable debugging of the Nextion messages over the
// virtual COM port (57600, 8N1)
#define NEXTION_DEBUG 1

#if NEXTION_DEBUG
#include <ctype.h>
#include "launchxl_ex1_sci_io.h"
#endif

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

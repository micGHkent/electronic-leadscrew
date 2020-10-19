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
    mode_metric_(false),
    mode_feed_(true),
    reverse_(false),
    in_edit_(false)
{
    feed_[0] = .005; // in/rev
    feed_[1] = 8; // TPI
    feed_[2] = .128; // mm/rev
    feed_[3] = 1; // mm pitch
    strcpy(feed_str_[0], ".005");
    strcpy(feed_str_[1], "8");
    strcpy(feed_str_[2], ".128");
    strcpy(feed_str_[3], "1");
    feed_str_new_[0][0] = '\0';
    feed_str_new_[1][0] = '\0';
    feed_str_new_[2][0] = '\0';
    feed_str_new_[3][0] = '\0';
    update_ind();
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

    // Timeout after 40 * 25ms = 1s
    for (int i = 0; i < 40; i++)
    {
        const int nmax = 6;
        uchar_t msg[nmax];
        int n = read(msg, nmax);
        if (n > 3)
        {
            bool has_end = memcmp(msg + (n - 3), "\xff\xff\xff", 3) == 0;
            if (n >= 4 && msg[n - 4] == 0x88 && has_end)
            {
                break;
            }
        }
        DELAY_US(25000);
    }

    set_all();
}

bool Nextion::update(Uint16 rpm, bool alarm, bool enabled)
{
    set_rpm(rpm);

    bool updated = false;
    bool p_enabled = enabled_;

    // Update alarm indicator
    if (alarm_ != alarm) {
        alarm_ = alarm;
        enabled_ = false;
        set_alarm();
        updated = true;
    }

    // Check the limit switch and update the enabled state
    at_stop_ = GPIO_ReadPin(25);
    enabled_ = enabled_ && enabled && !at_stop_;

    // Receive message from Nextion display
    const int nmax = 12;
    uchar_t msg[nmax];
    int n = read(msg, nmax);

    if (n > 3)
    {
        bool has_end = memcmp(msg + (n - 3), "\xff\xff\xff", 3) == 0;

        if (has_end && (n == 10 || n == 4) && msg[n - 4] == 0x88)
        {
            // Reinitialize the screen if the Nextion resets, through a flag
            // passed back to UserInterface the feed information is reinitialized.
            set_all();
        }
        else if (has_end && n == 5 && msg[0] == 0x25 && !alarm_)
        {
            // Touch Event
            uchar_t k = msg[1];

            const char max_n = 6;
            char *f = feed_str_new_[ind_];
            int n = strlen(f);

            if (0x30 <= k && k <= 0x39) // number, 0-9
            {
                if (n < max_n) {
                    f[n++] = k;
                    f[n] = '\0';
                    set_feed_new();
                }
                in_edit_ = true;
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
                        set_feed_new();
                    }
                }
                in_edit_ = true;
            }
            else if (k == 0x08) // backspace
            {
                if (n > 0) {
                    f[n-1] = '\0';
                    set_feed_new();
                }
                in_edit_ = true;
            }
            else if (k == 0x0d) // enter
            {
                strcpy(feed_str_[ind_], feed_str_new_[ind_]);
                f[0] = '\0';
                in_edit_ = false;
                updated = true;
                set_feed();
            }
            else if (k == 0x1a) // mode/dir change
            {
                if(! in_edit_)
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
            }
            else if (k == 0x1b) // units change
            {
                if (! in_edit_)
                {
                    mode_metric_ = !mode_metric_;
                    updated = true;

                    set_units();
                }
            }
            else if (k == 0x1c) // start/stop
            {
                if (p_enabled == enabled_) {
                    // Remain disabled if limit switch is tripped, else toggle enable.
                    enabled_ = !enabled_ && !at_stop_;
                }
            }
            else if (k == 0x1d) // nothing
            {
            }
            else if (k == 0x1e) // cancel
            {
                f[0] = '\0';
                set_feed();
                in_edit_ = false;
            }
        }
    }

    // Update the enable/disable button
    if (p_enabled != enabled_)
    {
        set_sign();
        updated = true;
    }

    if (updated) {
        update_ind();
        set_feed();
    }

    return updated;
}

void Nextion::getFeed(float &v, bool &metric, bool &feed) const {
    v = feed_[ind_];
    metric = mode_metric_;
    feed = mode_feed_;
#if NEXTION_DEBUG
    printf("bb %d\r\n", int(v*10000));
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

void Nextion::set_feed()
{
    const char *f = feed_str_[ind_];
    float f2 = strtof(f, NULL);
    feed_[ind_] = f2;

    {
        uchar_t msg2[8 + 4 + 5];
        uchar_t *p = msg2 + sprintf((char*)msg2, "t1.txt=\"%s\"", f);
        *p++ = '\xff';
        *p++ = '\xff';
        *p++ = '\xff';
        *p++ = '\0';
        send(msg2);
    }

    send("t1.pco=65535\xff\xff\xff");

#if NEXTION_DEBUG
    printf("aa %d\r\n", int(f2 * 10000));
#endif
}

void Nextion::set_feed_new()
{
    const char *f = feed_str_new_[ind_];
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

    send("t1.pco=13812\xff\xff\xff");

#if NEXTION_DEBUG
    printf("aa %d\r\n", int(f2 * 10000));
#endif
}

void Nextion::set_rpm(Uint16 rpm)
{
    if (rpm_ != rpm)
    {
        rpm_ = rpm;

        uchar_t msg2[32];
        sprintf((char*) msg2, "t0.txt=\"%u\"\xff\xff\xff", rpm_);
        send(msg2);
    }

    set_graph();
}

void Nextion::set_graph() {
    const int graph_id = 22;
    const int rpm_m_ = 0;
    const int rpm_M_ = 3000;
    const int graph_h = 160;

    int v = (int)(float(rpm_ - rpm_m_) / float(rpm_M_ - rpm_m_) * float(graph_h));
    if (v < 0) v = 0;
    if (v > graph_h) v = graph_h;

    uchar_t nex_buffer[32];
    sprintf((char*)nex_buffer, "add %d,0,%d\xff\xff\xff", graph_id, v);
    send(nex_buffer);
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

void Nextion::set_alarm() {
    if (alarm_) {
        send("vis 4,0\xff\xff\xff");
        send("vis 25,1\xff\xff\xff");
    } else {
        send("vis 25,0\xff\xff\xff");
    }
}

void Nextion::set_all() {
    update_ind();

    set_feed();
    set_rpm(rpm_);

    set_diagram();
    set_units();
    set_sign();
    set_alarm();

    // Set credits message
    {
        const uchar_t *msg = { "t2.txt=\"ELS 1.3.01\r\n"
                               "James Clough (Clough42)\r\n"
                               "\r\n"
                               "Touchscreen interface\r\n"
                               "Kent A. Vander Velden"
                               "\"\xff\xff\xff" };
        send(msg);
    }

#if NEXTION_DEBUG
    printf("Initialized\r\n");
#endif
}

void Nextion::update_ind() {
    if (mode_metric_ && mode_feed_) {
        ind_ = 2; // mm/rev
    } else if (mode_metric_ && !mode_feed_) {
        ind_ = 3; // mm
    } else if (!mode_metric_ && mode_feed_) {
        ind_ = 0; // in/rev
    } else if (!mode_metric_ && !mode_feed_) {
        ind_ = 1; // TPI
    }
}

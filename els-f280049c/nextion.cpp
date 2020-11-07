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
#include "Configuration.h" // Only for ENCODER_RESOLUTION

typedef unsigned char uchar_t;

// Set to 1 to enable debugging of the Nextion messages over the
// virtual COM port (115200, 8N1)
#define NEXTION_DEBUG 0

#if NEXTION_DEBUG
#include <ctype.h>
#include "launchxl_ex1_sci_io.h"
#endif

__interrupt void scibTxISR(void);
__interrupt void scibRxISR(void);

//
// sciaTxISR - Disable the TXFF interrupt and print message asking
//             for two characters.
//
__interrupt void
scibTxISR(void)
{
    //
    // Disable the TXRDY interrupt.
    //
//    SCI_disableInterrupt(SCIB_BASE, SCI_INT_TXFF);

//    msg = "\r\nEnter two characters: \0";
//    SCI_writeCharArray(SCIB_BASE, (uint16_t*)msg, 26);

    //
    // Acknowledge the PIE interrupt.
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}

int nnn = 0;
unsigned char buff[256];

//
// sciaRxISR - Read two characters from the RXBUF and echo them back.
//
__interrupt void
scibRxISR(void)
{
    uint16_t c;

    //
    // Enable the TXFF interrupt again.
    //
//    SCI_enableInterrupt(SCIB_BASE, SCI_INT_TXFF);

    //
    // Read two characters from the FIFO.
    //
//    c = SCI_readCharBlockingFIFO(SCIB_BASE);
    while(SCI_isDataAvailableNonFIFO(SCIB_BASE)) {
    c = SCI_readCharBlockingNonFIFO(SCIB_BASE);
    if (nnn < 256) {
        buff[nnn++] = (unsigned char)(c & 0xff);
    }
    }

    //
    // Clear the SCI RXFF interrupt and acknowledge the PIE interrupt.
    //
//    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_RXFF);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

//    counter++;
}

Nextion::Nextion() :
    rpm_(0),
    position_(0),
    position_mode_(0),
    enabled_(false),
    alarm_(false),
    at_stop_(false),
    mode_metric_(false),
    mode_feed_(true),
    reverse_(false),
    in_edit_(false)
{
    update_ind();
}

int Nextion::read(uchar_t buf[], const int nmax)
{
    int n = 0;

//    while (n < nmax && ScibRegs.SCIFFRX.bit.RXFFST)
    while (n < nmax && SCI_isDataAvailableNonFIFO(SCIB_BASE))
    {
        uint16_t         c = SCI_readCharBlockingNonFIFO(SCIB_BASE);

        buf[n++] = c;

        // This delay is done to increase chances that a complete message from
        // the Nextion will be received in one function call. To eliminate the
        // delay, add memory to the Nextion routines, continuing to
        // read and tokenize per call until a valid message is gathered.
        // ~260us to transmit 10-bits at 38.4kBaud
        DELAY_US(265);
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

void Nextion::send(const uchar_t *msg, int nn)
{
    if (nn == -1) {
        int i;
        i = 0;
        while (msg[i] != '\0')
        {
//            SCI_writeCharBlockingFIFO(SCIB_BASE, uint16_t(msg[i]));
            SCI_writeCharBlockingNonFIFO(SCIB_BASE, uint16_t(msg[i]));
            i++;
        }
    } else {
        for(int i=0; i<nn; i++) {
//            SCI_writeCharBlockingFIFO(SCIB_BASE, uint16_t(msg[i]));
            SCI_writeCharBlockingNonFIFO(SCIB_BASE, uint16_t(msg[i]));
        }
    }

#if NEXTION_DEBUG
    {
        const uchar_t *p;
        printf("Send (%d): ", nn);
        p = msg;
        while((nn == -1 && *p != '\0') || (nn >= 0 && (p - msg < nn))) {
            if(isprint(*p)) {
              putchar(*p);
            } else {
              putchar(0xff);
            }
            p++;
        }
#if 0
        printf(" : ");
        p = msg;
        while((nn == -1 && *p != '\0') || (nn >= 0 && (p - msg < nn))) {
            printf(" %02x", *p);
            p++;
        }
#endif
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
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    SCI_performSoftwareReset(SCIA_BASE);

    SCI_setConfig(SCIA_BASE, 25000000, 115200, (SCI_CONFIG_WLEN_8 |
                                                SCI_CONFIG_STOP_ONE |
                                                SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
//    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
//    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);


    // To help with debugging, configure the UART that is connected to
    // USB port, the virtual terminal, to be stdout.
    volatile int status = 0;
    status = add_device("scia", _SSA, SCI_open, SCI_close, SCI_read, SCI_write,
                        SCI_lseek, SCI_unlink, SCI_rename);
    volatile FILE *fid = fopen("scia", "w");
    freopen("scia:", "w", stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
#endif

    // Not already defined in CWare2000 v3.03 device.h
#define DEVICE_GPIO_PIN_SCIRXDB     13U             // GPIO number for SCI RX
#define DEVICE_GPIO_PIN_SCITXDB     40U             // GPIO number for SCI TX
#define DEVICE_GPIO_CFG_SCIRXDB     GPIO_13_SCIRXDB // "pinConfig" for SCI RX
#define DEVICE_GPIO_CFG_SCITXDB     GPIO_40_SCITXDB // "pinConfig" for SCI TX

    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDB, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDB);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDB, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDB, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDB, GPIO_QUAL_ASYNC);

    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDB, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDB);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDB, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDB, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDB, GPIO_QUAL_ASYNC);

//    Interrupt_register(INT_SCIB_TX, scibTxISR);
    Interrupt_register(INT_SCIB_RX, scibRxISR);

    SCI_performSoftwareReset(SCIB_BASE);

    SCI_setConfig(SCIB_BASE, 25000000, 38400, (SCI_CONFIG_WLEN_8 |
                                               SCI_CONFIG_STOP_ONE |
                                               SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIB_BASE);
//    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
//    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT);
    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_RXRDY_BRKDT);
//    SCI_enableFIFO(SCIB_BASE);
    SCI_enableModule(SCIB_BASE);
    SCI_performSoftwareReset(SCIB_BASE);

    //
    // Set the transmit FIFO level to 0 and the receive FIFO level to 2.
    // Enable the TXFF and RXFF interrupts.
    //
//    SCI_setFIFOInterruptLevel(SCIB_BASE, SCI_FIFO_TX1, SCI_FIFO_RX1);
//    SCI_enableInterrupt(SCIB_BASE, SCI_INT_RXFF);
//    SCI_enableInterrupt(SCIB_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
//    SCI_enableInterrupt(SCIB_BASE, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT);
    SCI_enableInterrupt(SCIB_BASE, SCI_INT_RXRDY_BRKDT);

//    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
//    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_TXRDY | SCI_INT_RXRDY_BRKDT);
    SCI_clearInterruptStatus(SCIB_BASE, SCI_INT_RXRDY_BRKDT);

    Interrupt_enable(INT_SCIB_RX);
//    Interrupt_enable(INT_SCIB_TX);
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

//    GPIO_setPinConfig(GPIO_13_SCIRXDB);
//    GPIO_setPinConfig(GPIO_40_SCITXDB);
//
//    GPIO_setPadConfig(13, GPIO_PIN_TYPE_PULLUP);
//    // GPIO_setPadConfig(40, GPIO_PIN_TYPE_PULLUP);
//
//    GPIO_setQualificationMode(13, GPIO_QUAL_ASYNC);

//    scib_init();

//    initSCIBFIFO();

    set_params();
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
#if 0
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
#endif
    DELAY_US(1000000);
    set_all(true);
}

bool Nextion::update(Uint16 rpm, Uint32 position, bool alarm, bool enabled)
{
    set_rpm(rpm);
    set_position(position);

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

    if (nnn > 200) nnn = 0;

    // Receive message from Nextion display
    const int nmax = 12;
    uchar_t msg[256];
//    int n = read(msg, nmax);
    int n = 0;

//    DINT;

//    Interrupt_disable(INT_SCIB_RX);
//    SCI_disableInterrupt(SCIB_BASE, SCI_INT_RXFF);
#if NEXTION_DEBUG
    if (nnn > 0) {
        printf("buff(%d)", nnn);
        for (int ii=0; ii<nnn; ii++) {
            printf(" %02x", buff[ii]);
        }
        printf("\r\n");
    }
#endif
    if (nnn > 3) {
        for (int i=0; i<=nnn-3; i++) {
            bool has_end = memcmp(buff + i, "\xff\xff\xff", 3) == 0;
            if (has_end) {
                n = i+3;
                memmove(msg, buff, n);

                nnn -= n;
                memmove(buff, buff+n, nnn);

#if NEXTION_DEBUG
                printf("msg(%d)", n);
                for (int ii=0; ii<n; ii++) {
                    printf(" %02x", msg[ii]);
                }
                printf("\r\n");
#endif
            }
        }
    }
//    SCI_enableInterrupt(SCIB_BASE, SCI_INT_RXFF);
//    Interrupt_enable(INT_SCIB_RX);
//    EINT;

//    return false;

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

                store_params();
                restore_params();
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
            else if (k == 0x1d) // unnamed button
            {
            }
            else if (k == 0x1e) // cancel
            {
                f[0] = '\0';
                set_feed();
                in_edit_ = false;
            }
            else if (k == 0x1d) // Alarm
            {
            }
            else if (k == 0x1f) // RPM meter
            {
            }
            else if (k == 0x20) // encoder position
            {
                position_mode_ = (position_mode_ + 1) % 6;
                set_position(position_, true);
            }
            else if (k == 0x21) // Credits
            {
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

void Nextion::set_rpm(Uint16 rpm, bool force)
{
    if (rpm_ != rpm || force)
    {
        rpm_ = rpm;

        uchar_t msg2[32];
        sprintf((char*) msg2, "t0.txt=\"%u\"\xff\xff\xff", rpm_);
        send(msg2);
    }

//    set_graph();
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

void Nextion::set_position(Uint32 position, bool force)
{
    if (position_ != position || force)
    {
        position_ = position;

        uchar_t msg2[64] = {"t3.txt=\"off\"\xff\xff\xff"};
        if (position_mode_ == 3) {
            // Show raw encoder count.
            sprintf((char*) msg2, "t3.txt=\"%08lutc\"\xff\xff\xff", position_);
        }
        else if (position_mode_ == 4) {
            // Show encoder count modulo encoder count per revolution.
            Uint32 v = position_ % (ENCODER_RESOLUTION);
            sprintf((char*) msg2, "t3.txt=\"%04luc\"\xff\xff\xff", v);
        }
        else if (position_mode_ == 2) {
            // Show percent of a full rotation.
            float v = float(position_ % (ENCODER_RESOLUTION)) / float(ENCODER_RESOLUTION) * float(100);
//            sprintf((char*) msg2, "t3.txt=\"%.6f\"\xff\xff\xff", v);
            int a = int(v);
            int b = int((v - float(a)) * 100);
            sprintf((char*) msg2, "t3.txt=\"%02d.%02d%%\"\xff\xff\xff", a, b);
        }
        else if (position_mode_ == 1) {
            // Show decimal degrees of a full rotation.
            float v = float(position_ % (ENCODER_RESOLUTION)) / float(ENCODER_RESOLUTION) * float(360);
//            sprintf((char*) msg2, "t3.txt=\"%.6f\"\xff\xff\xff", v);
            int a = int(v);
            int b = int((v - float(a)) * 100);
            sprintf((char*) msg2, "t3.txt=\"%03d.%02dd\"\xff\xff\xff", a, b);
        }
        else if (position_mode_ == 0) {
            // Show degrees and minutes (and maybe seconds) of a full rotation.
            // There are 60 minutes in a degree. 1/60 = 0.0167 deg
            // There are 3600 seconds in a degree. 1/3600 = 0.000278 deg
            // If an encoder has 4096 counts per revolutions, the finest graduation
            // possible is 360/4096 = 0.0879 degree.
            // With a 4096-count encoder, display minutes makes sense, but seconds
            // is beyond the encoder's resolution.
            // To display meaningful seconds, a 524288-count encoder or better
            // would be required.
            float dd = float(position_ % (ENCODER_RESOLUTION)) / float(ENCODER_RESOLUTION) * 360.;
            int d = int(dd);
            int m = int((dd - d) * 60);
//            int s = int((dd - d - float(m) / float(60.)) * 3600);
//            sprintf((char*) msg2, "t3.txt=\"%03dd %02d' %02d''\"\xff\xff\xff", d, m, s);
            sprintf((char*) msg2, "t3.txt=\"%03dd %02d'\"\xff\xff\xff", d, m);
        }
        else if (position_mode_ == 5) {
            // Disable updates of rotary position.
        }
        send(msg2);
    }
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

void Nextion::set_all(bool force) {
    update_ind();

    set_feed();
    set_rpm(rpm_, force);
    set_position(position_, force);

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

void Nextion::store_params() {
#if 0
    SCI_disableInterrupt(SCIB_BASE, SCI_INT_RXRDY_BRKDT);
    unsigned char cmd[16];
    sprintf((char*)cmd, "wept 0,%d\xff\xff\xff", 4*16);
    send(cmd);
    DELAY_US(500000);
    unsigned char buf[4];
    read((unsigned char*)buf, 4); // (ready) 0xFE 0xFF 0xFF 0xFF
    DELAY_US(100000);
    send((unsigned char *)feed_str_, 4*16);
    DELAY_US(100000);
    read((unsigned char*)buf, 4); // (finished) 0xFD 0xFF 0xFF 0xFF
    SCI_enableInterrupt(SCIB_BASE, SCI_INT_RXRDY_BRKDT);
    nnn = 0;
#endif
}

void Nextion::restore_params() {
#if 0
//    SCI_disableInterrupt(SCIB_BASE, SCI_INT_RXRDY_BRKDT);
    unsigned char cmd[4*16+1] = { '\0' };
    sprintf((char*)cmd, "rept 0,%d\xff\xff\xff", 4*16);
    printf("aaaa1 %d\r\n", nnn);
    nnn = 0;
    send(cmd);
    DELAY_US(100000);
    memset(feed_str_, '*', 4*16);
    printf("aaaa2 %d\r\n", nnn);
    if (nnn == 4*16) {
        memmove(feed_str_, buff, 4*16);
    }
    nnn = 0;
//    unsigned char buf[4];
//    read((unsigned char*)buf, 4);
//    int a = read((unsigned char*)feed_str_, 4*16);
//    int b = read((unsigned char*)buf, 4);
//    printf("restore %d %d\r\n", a, b);
//    SCI_enableInterrupt(SCIB_BASE, SCI_INT_RXRDY_BRKDT);
#endif
}

void Nextion::set_params() {
    restore_params();

    bool valid = true;
    for (int i=0; i<4; i++) {
        if(strtof(feed_str_[i], NULL) == 0.) {
            valid = false;
            break;
        }
    }
//    valid = false;

    if (! valid) {
        strcpy(feed_str_[0], ".005"); // in/rev
        strcpy(feed_str_[1], "8");    // TPI
        strcpy(feed_str_[2], ".128"); // mm/rev
        strcpy(feed_str_[3], "1");    // mm pitch
        store_params();
    }

    for (int i=0; i<4; i++) {
        feed_[i] = strtof(feed_str_[i], NULL);
        feed_str_new_[i][0] = '\0';
    }
}

/*
 * nextion.h
 *
 *  Created on: Aug 25, 2020
 *      Author: Kent A. Vander Velden (kent.vandervelden@gmail.com)
 */

#ifndef NEXTION_H_
#define NEXTION_H_

#include "ControlPanel.h"

class Nextion {
public:
    Nextion();

    void init();
    void wait();
    bool update(Uint16 rpm, bool alarm, bool enabled);
    bool isAtStop() const;
    bool isEnabled() const;
    bool isReverse() const;
    void getFeed(float &v, bool &metric, bool &feed) const;

protected:
    Uint16 rpm_;
    bool enabled_;
    bool alarm_;
    bool at_stop_;
    float feed_[4];
    char feed_str_[4][16];
    char feed_str_new_[4][16];
    int ind_;
    bool mode_metric_;
    bool mode_feed_;
    bool reverse_;

    int read(unsigned char buf[], const int nmax);
    void send(const unsigned char *msg);
    void set_rpm(Uint16 rpm);
    void set_feed(const char *f);
    void set_diagram();
    void set_units();
    void set_sign();
    void update_ind();
};

#endif /* NEXTION_H_ */

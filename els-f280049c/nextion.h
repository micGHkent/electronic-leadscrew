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
    bool update(Uint16 rpm, Uint32 position, bool alarm, bool enabled);
    bool isAtStop() const;
    bool isEnabled() const;
    bool isReverse() const;
    void getFeed(float &v, bool &metric, bool &feed) const;

protected:
    Uint16 rpm_;
    Uint32 position_;
    int position_mode_;
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
    bool in_edit_;

    int read(unsigned char buf[], const int nmax);
    void send(const unsigned char *msg, int nn=-1);
    void set_rpm(Uint16 rpm, bool force=false);
    void set_position(Uint32 position, bool force=false);
    void set_feed();
    void set_feed_new();
    void set_graph();
    void set_diagram();
    void set_units();
    void set_sign();
    void set_alarm();
    void set_all(bool force=false);
    void update_ind();
    void store_params();
    void restore_params();
    void set_params();
};

#endif /* NEXTION_H_ */

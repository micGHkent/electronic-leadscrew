// Clough42 Electronic Leadscrew
// https://github.com/clough42/electronic-leadscrew
//
// MIT License
//
// Copyright (c) 2019 James Clough
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


#ifndef __CORE_H
#define __CORE_H

#include "StepperDrive.h"
#include "Encoder.h"
#include "ControlPanel.h"


class Core
{
private:
    Encoder *encoder;
    StepperDrive *stepperDrive;

    float feed;
    float previousFeed;

    int16 feedDirection;
    int16 previousFeedDirection;

    Uint32 previousSpindlePosition;

    int32 feedRatio(Uint32 count);

    // KVV
    bool enabled;
    bool reenabled;

public:
    Core( Encoder *encoder, StepperDrive *stepperDrive );

    void setFeed(float feed, bool metric, bool isfeed);
    void setReverse(bool reverse);
    Uint16 getRPM();
    bool isAlarm();

    void ISR();

    // KVV
    void setEnabled(bool v);
    bool isEnabled() const;
};

// KVV
inline void Core :: setEnabled(bool v)
{
    if(enabled != v) {
        enabled = v;
        reenabled = enabled;
    }
}

inline bool Core :: isEnabled() const
{
    return enabled;
}

inline Uint16 Core :: getRPM()
{
    return encoder->getRPM();
}

inline bool Core :: isAlarm()
{
    return this->stepperDrive->isAlarm();
}

inline int32 Core :: feedRatio(Uint32 count)
{
    return count * feed * feedDirection;
}

inline void Core :: ISR()
{
    if( this->enabled && this->feed != NULL ) {
        // read the encoder
        Uint32 spindlePosition = encoder->getPosition();

        // calculate the desired stepper position
        int32 desiredSteps = feedRatio(spindlePosition);
        stepperDrive->setDesiredPosition(desiredSteps);

        // compensate for encoder overflow/underflow
        if( spindlePosition < previousSpindlePosition && previousSpindlePosition - spindlePosition > encoder->getMaxCount()/2 ) {
            stepperDrive->incrementCurrentPosition(-1 * feedRatio(encoder->getMaxCount()));
        }
        if( spindlePosition > previousSpindlePosition && spindlePosition - previousSpindlePosition > encoder->getMaxCount()/2 ) {
            stepperDrive->incrementCurrentPosition(feedRatio(encoder->getMaxCount()));
        }

        // if the feed or direction changed, reset sync to avoid a big step
        if( feed != previousFeed || feedDirection != previousFeedDirection || reenabled) {
            stepperDrive->setCurrentPosition(desiredSteps);
            // KVV
            reenabled = false;
        }

        // remember values for next time
        previousSpindlePosition = spindlePosition;
        previousFeedDirection = feedDirection;
        previousFeed = feed;

        // service the stepper drive state machine
        stepperDrive->ISR();
    }
}


#endif // __CORE_H

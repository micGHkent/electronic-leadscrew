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


#include "F28x_Project.h"
#include "Core.h"


static const float step_enc_ratio = float(STEPPER_RESOLUTION * STEPPER_MICROSTEPS) / float(ENCODER_RESOLUTION);
static const float step_enc_ratio_feed = float(STEPPER_RESOLUTION_FEED * STEPPER_MICROSTEPS_FEED) / float(ENCODER_RESOLUTION);

#if defined(LEADSCREW_TPI)
#define TPI_FRACTION(tpi) (float(LEADSCREW_TPI) / float(tpi) * step_enc_ratio)
#elif defined(LEADSCREW_MM)
#define TPI_FRACTION(tpi) (25.4 / float(tpi * LEADSCREW_MM) * step_enc_ratio)
#endif

#if defined(LEADSCREW_TPI)
#define THOU_IN_FRACTION(thou) (float(thou * LEADSCREW_TPI) * step_enc_ratio_feed)
#elif defined(LEADSCREW_MM)
#define THOU_IN_FRACTION(thou) (float(thou * 25.4) / float(LEADSCREW_MM) * step_enc_ratio_feed)
#endif

#if defined(LEADSCREW_TPI)
#define MM_FRACTION(mm) (float(mm) / 25.4 * LEADSCREW_TPI * step_enc_ratio)
#elif defined(LEADSCREW_MM)
#define MM_FRACTION(mm) (float(mm) / float(LEADSCREW_MM) * step_enc_ratio)
#endif

#if defined(LEADSCREW_TPI)
#define MM_FRACTION_FEED(mm) (float(mm) / 25.4 * LEADSCREW_TPI * step_enc_ratio_feed)
#elif defined(LEADSCREW_MM)
#define MM_FRACTION_FEED(mm) (float(mm) / float(LEADSCREW_MM) * step_enc_ratio_feed)
#endif

void Core :: setFeed(float v, bool metric, bool feed)
{
    if (!metric && !feed) {
        v = TPI_FRACTION(v);
    } else if (!metric && feed) {
        v = THOU_IN_FRACTION(v);
    } else if (metric && !feed) {
        v = MM_FRACTION(v);
    } else if (metric && feed) {
        v = MM_FRACTION_FEED(v);
    }
    this->feed = v;
}




Core :: Core( Encoder *encoder, StepperDrive *stepperDrive )
{
    this->encoder = encoder;
    this->stepperDrive = stepperDrive;

    feed = NULL;
    feedDirection = 0;

    previousSpindlePosition = 0;
    previousFeedDirection = 0;
    previousFeed = NULL;

    // KVV
    enabled = true;
    reenabled = true;
}

void Core :: setReverse(bool reverse)
{
    feedDirection = reverse ? -1 : 1;
}


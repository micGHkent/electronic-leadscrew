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


#include "Tables.h"


//
// INCH THREAD DEFINITIONS
//
// Each row in the table defines a standard imperial thread, with the display data,
// LED indicator states and gear ratio fraction to use.
//
#if defined(LEADSCREW_TPI)
#define TPI_NUMERATOR(tpi) ((Uint64)LEADSCREW_TPI*STEPPER_RESOLUTION*STEPPER_MICROSTEPS*10)
#define TPI_DENOMINATOR(tpi) ((Uint64)tpi*ENCODER_RESOLUTION)
#endif
#if defined(LEADSCREW_HMM)
#define TPI_NUMERATOR(tpi) ((Uint64)254*100*STEPPER_RESOLUTION*STEPPER_MICROSTEPS)
#define TPI_DENOMINATOR(tpi) ((Uint64)tpi*ENCODER_RESOLUTION*LEADSCREW_HMM)
#endif
#define TPI_FRACTION(tpi) .numerator = TPI_NUMERATOR(tpi), .denominator = TPI_DENOMINATOR(tpi)

const FEED_THREAD inch_thread_table[] =
{
 { .display = "8",  .leds = LED_THREAD | LED_TPI, TPI_FRACTION(80) },
 { .display = "9",  .leds = LED_THREAD | LED_TPI, TPI_FRACTION(90) },
 { .display = "10", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(100) },
 { .display = "11", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(110) },
 { .display = "11.5",.leds = LED_THREAD | LED_TPI, TPI_FRACTION(115) },
 { .display = "12", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(120) },
 { .display = "13", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(130) },
 { .display = "14", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(140) },
 { .display = "16", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(160) },
 { .display = "18", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(180) },
 { .display = "19", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(190) },
 { .display = "20", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(200) },
 { .display = "24", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(240) },
 { .display = "26", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(260) },
 { .display = "27", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(270) },
 { .display = "28", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(280) },
 { .display = "32", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(320) },
 { .display = "36", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(360) },
 { .display = "40", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(400) },
 { .display = "44", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(440) },
 { .display = "48", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(480) },
 { .display = "56", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(560) },
 { .display = "64", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(640) },
 { .display = "72", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(720) },
 { .display = "80", .leds = LED_THREAD | LED_TPI, TPI_FRACTION(800) },
};



//
// INCH FEED DEFINITIONS
//
// Each row in the table defines a standard imperial feed rate, with the display data,
// LED indicator states and gear ratio fraction to use.
//

#if defined(LEADSCREW_TPI)
#define THOU_IN_NUMERATOR(thou) ((Uint64)thou*LEADSCREW_TPI*STEPPER_RESOLUTION_FEED*STEPPER_MICROSTEPS_FEED)
#define THOU_IN_DENOMINATOR(thou) ((Uint64)ENCODER_RESOLUTION*1000)
#endif
#if defined(LEADSCREW_HMM)
#define THOU_IN_NUMERATOR(thou) ((Uint64)thou*254*STEPPER_RESOLUTION_FEED*STEPPER_MICROSTEPS_FEED)
#define THOU_IN_DENOMINATOR(thou) ((Uint64)ENCODER_RESOLUTION*100*LEADSCREW_HMM)
#endif
#define THOU_IN_FRACTION(thou) .numerator = THOU_IN_NUMERATOR(thou), .denominator = THOU_IN_DENOMINATOR(thou)

const FEED_THREAD inch_feed_table[] =
{
 { .display = ".001", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(1) },
 { .display = ".002", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(2) },
 { .display = ".003", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(3) },
 { .display = ".004", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(4) },
 { .display = ".005", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(5) },
 { .display = ".006", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(6) },
 { .display = ".007", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(7) },
 { .display = ".008", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(8) },
 { .display = ".009", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(9) },
 { .display = ".010", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(10) },
 { .display = ".011", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(11) },
 { .display = ".012", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(12) },
 { .display = ".013", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(13) },
 { .display = ".015", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(15) },
 { .display = ".017", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(17) },
 { .display = ".020", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(20) },
 { .display = ".023", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(23) },
 { .display = ".026", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(26) },
 { .display = ".030", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(30) },
 { .display = ".035", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(35) },
 { .display = ".040", .leds = LED_FEED | LED_INCH, THOU_IN_FRACTION(40) },
};




//
// METRIC THREAD DEFINITIONS
//
// Each row in the table defines a standard metric thread, with the display data,
// LED indicator states and gear ratio fraction to use.
//
#if defined(LEADSCREW_TPI)
#define HMM_NUMERATOR(hmm) ((Uint64)hmm*10*LEADSCREW_TPI*STEPPER_RESOLUTION*STEPPER_MICROSTEPS)
#define HMM_DENOMINATOR(hmm) ((Uint64)ENCODER_RESOLUTION*254*100)
#endif
#if defined(LEADSCREW_HMM)
#define HMM_NUMERATOR(hmm) ((Uint64)hmm*STEPPER_RESOLUTION*STEPPER_MICROSTEPS)
#define HMM_DENOMINATOR(hmm) ((Uint64)ENCODER_RESOLUTION*LEADSCREW_HMM)
#endif
#define HMM_FRACTION(hmm) .numerator = HMM_NUMERATOR(hmm), .denominator = HMM_DENOMINATOR(hmm)

const FEED_THREAD metric_thread_table[] =
{
 { .display = ".20",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(20) },
 { .display = ".25",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(25) },
 { .display = ".30",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(30) },
 { .display = ".35",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(35) },
 { .display = ".40",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(40) },
 { .display = ".45",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(45) },
 { .display = ".50",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(50) },
 { .display = ".60",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(60) },
 { .display = ".70",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(70) },
 { .display = ".75",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(75) },
 { .display = ".80",  .leds = LED_THREAD | LED_MM, HMM_FRACTION(80) },
 { .display = "1.00", .leds = LED_THREAD | LED_MM, HMM_FRACTION(100) },
 { .display = "1.25", .leds = LED_THREAD | LED_MM, HMM_FRACTION(125) },
 { .display = "1.50", .leds = LED_THREAD | LED_MM, HMM_FRACTION(150) },
 { .display = "1.75", .leds = LED_THREAD | LED_MM, HMM_FRACTION(175) },
 { .display = "2.00", .leds = LED_THREAD | LED_MM, HMM_FRACTION(200) },
 { .display = "2.50", .leds = LED_THREAD | LED_MM, HMM_FRACTION(250) },
 { .display = "3.00", .leds = LED_THREAD | LED_MM, HMM_FRACTION(300) },
 { .display = "3.50", .leds = LED_THREAD | LED_MM, HMM_FRACTION(350) },
 { .display = "4.00", .leds = LED_THREAD | LED_MM, HMM_FRACTION(400) },
 { .display = "4.50", .leds = LED_THREAD | LED_MM, HMM_FRACTION(450) },
 { .display = "5.00", .leds = LED_THREAD | LED_MM, HMM_FRACTION(500) },
 { .display = "5.50", .leds = LED_THREAD | LED_MM, HMM_FRACTION(550) },
 { .display = "6.00", .leds = LED_THREAD | LED_MM, HMM_FRACTION(600) },
};



//
// METRIC FEED DEFINITIONS
//
// Each row in the table defines a standard metric feed, with the display data,
// LED indicator states and gear ratio fraction to use.
//
#if defined(LEADSCREW_TPI)
#define HMM_NUMERATOR_FEED(hmm) ((Uint64)hmm*10*LEADSCREW_TPI*STEPPER_RESOLUTION_FEED*STEPPER_MICROSTEPS_FEED)
#define HMM_DENOMINATOR_FEED(hmm) ((Uint64)ENCODER_RESOLUTION*254*100)
#endif
#if defined(LEADSCREW_HMM)
#define HMM_NUMERATOR_FEED(hmm) ((Uint64)hmm*STEPPER_RESOLUTION_FEED*STEPPER_MICROSTEPS_FEED)
#define HMM_DENOMINATOR_FEED(hmm) ((Uint64)ENCODER_RESOLUTION*LEADSCREW_HMM)
#endif
#define HMM_FRACTION_FEED(hmm) .numerator = HMM_NUMERATOR_FEED(hmm), .denominator = HMM_DENOMINATOR_FEED(hmm)

const FEED_THREAD metric_feed_table[] =
{
 { .display = ".02",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(2) },
 { .display = ".05",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(5) },
 { .display = ".07",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(7) },
 { .display = ".10",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(10) },
 { .display = ".12",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(12) },
 { .display = ".15",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(15) },
 { .display = ".17",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(17) },
 { .display = ".29",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(20) },
 { .display = ".22",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(22) },
 { .display = ".25",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(25) },
 { .display = ".27",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(27) },
 { .display = ".30",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(30) },
 { .display = ".35",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(35) },
 { .display = ".40",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(40) },
 { .display = ".45",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(45) },
 { .display = ".50",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(50) },
 { .display = ".55",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(55) },
 { .display = ".60",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(60) },
 { .display = ".70",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(70) },
 { .display = ".85",  .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(85) },
 { .display = "1.00", .leds = LED_FEED | LED_MM, HMM_FRACTION_FEED(100) },
};





FeedTable::FeedTable(const FEED_THREAD *table, Uint16 numRows, Uint16 defaultSelection)
{
    this->table = table;
    this->numRows = numRows;
    selectedRow = defaultSelection;
}

const FEED_THREAD *FeedTable :: current()
{
    return &table[selectedRow];
}

const FEED_THREAD *FeedTable :: next()
{
    if( selectedRow < numRows - 1 )
    {
        selectedRow++;
    }
    return current();
}

const FEED_THREAD *FeedTable :: previous()
{
    if( selectedRow > 0 )
    {
        selectedRow--;
    }
    return current();
}

FeedTableFactory::FeedTableFactory():
        inchThreads(inch_thread_table, sizeof(inch_thread_table)/sizeof(inch_thread_table[0]), 12),
        inchFeeds(inch_feed_table, sizeof(inch_feed_table)/sizeof(inch_feed_table[0]), 4),
        metricThreads(metric_thread_table, sizeof(metric_thread_table)/sizeof(metric_thread_table[0]), 6),
        metricFeeds(metric_feed_table, sizeof(metric_feed_table)/sizeof(metric_feed_table[0]), 4)
{
}

FeedTable *FeedTableFactory::getFeedTable(bool metric, bool thread)
{
    if( metric )
    {
        if( thread )
        {
            return &metricThreads;
        }
        else
        {
            return &metricFeeds;
        }
    }
    else
    {
        if( thread )
        {
            return &inchThreads;
        }
        else
        {
            return &inchFeeds;
        }
    }
}

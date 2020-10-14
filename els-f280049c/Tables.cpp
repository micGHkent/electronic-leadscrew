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


static const float step_enc_ratio = float(STEPPER_RESOLUTION * STEPPER_MICROSTEPS) / float(ENCODER_RESOLUTION);
static const float step_enc_ratio_feed = float(STEPPER_RESOLUTION_FEED * STEPPER_MICROSTEPS_FEED) / float(ENCODER_RESOLUTION);

//
// INCH THREAD DEFINITIONS
//
// Each row in the table defines a standard imperial thread, with the display data,
// LED indicator states and gear ratio fraction to use.
//
#if defined(LEADSCREW_TPI)
#define TPI_FRACTION(tpi) (float(LEADSCREW_TPI) / float(tpi) * step_enc_ratio)
#elif defined(LEADSCREW_MM)
#define TPI_FRACTION(tpi) (25.4 / float(tpi * LEADSCREW_MM) * step_enc_ratio)
#endif

const FEED_THREAD inch_thread_table[] =
{
 { "8",   LED_THREAD | LED_TPI, TPI_FRACTION(8) },
 { "9",   LED_THREAD | LED_TPI, TPI_FRACTION(9) },
 { "10",  LED_THREAD | LED_TPI, TPI_FRACTION(10) },
 { "11",  LED_THREAD | LED_TPI, TPI_FRACTION(11) },
 { "11.5",LED_THREAD | LED_TPI, TPI_FRACTION(11.5) },
 { "12",  LED_THREAD | LED_TPI, TPI_FRACTION(12) },
 { "13",  LED_THREAD | LED_TPI, TPI_FRACTION(13) },
 { "14",  LED_THREAD | LED_TPI, TPI_FRACTION(14) },
 { "16",  LED_THREAD | LED_TPI, TPI_FRACTION(16) },
 { "18",  LED_THREAD | LED_TPI, TPI_FRACTION(18) },
 { "19",  LED_THREAD | LED_TPI, TPI_FRACTION(19) },
 { "20",  LED_THREAD | LED_TPI, TPI_FRACTION(20) },
 { "24",  LED_THREAD | LED_TPI, TPI_FRACTION(24) },
 { "26",  LED_THREAD | LED_TPI, TPI_FRACTION(26) },
 { "27",  LED_THREAD | LED_TPI, TPI_FRACTION(27) },
 { "28",  LED_THREAD | LED_TPI, TPI_FRACTION(28) },
 { "32",  LED_THREAD | LED_TPI, TPI_FRACTION(32) },
 { "36",  LED_THREAD | LED_TPI, TPI_FRACTION(36) },
 { "40",  LED_THREAD | LED_TPI, TPI_FRACTION(40) },
 { "44",  LED_THREAD | LED_TPI, TPI_FRACTION(44) },
 { "48",  LED_THREAD | LED_TPI, TPI_FRACTION(48) },
 { "56",  LED_THREAD | LED_TPI, TPI_FRACTION(56) },
 { "64",  LED_THREAD | LED_TPI, TPI_FRACTION(64) },
 { "72",  LED_THREAD | LED_TPI, TPI_FRACTION(72) },
 { "80",  LED_THREAD | LED_TPI, TPI_FRACTION(80) },
};



//
// INCH FEED DEFINITIONS
//
// Each row in the table defines a standard imperial feed rate, with the display data,
// LED indicator states and gear ratio fraction to use.
//

#if defined(LEADSCREW_TPI)
#define THOU_IN_FRACTION(thou) (float(thou * LEADSCREW_TPI) * step_enc_ratio_feed)
#elif defined(LEADSCREW_MM)
#define THOU_IN_FRACTION(thou) (float(thou * 25.4) / float(LEADSCREW_MM) * step_enc_ratio_feed)
#endif

const FEED_THREAD inch_feed_table[] =
{
 { ".001", LED_FEED | LED_INCH, THOU_IN_FRACTION(.001) },
 { ".002", LED_FEED | LED_INCH, THOU_IN_FRACTION(.002) },
 { ".003", LED_FEED | LED_INCH, THOU_IN_FRACTION(.003) },
 { ".004", LED_FEED | LED_INCH, THOU_IN_FRACTION(.004) },
 { ".005", LED_FEED | LED_INCH, THOU_IN_FRACTION(.005) },
 { ".006", LED_FEED | LED_INCH, THOU_IN_FRACTION(.006) },
 { ".007", LED_FEED | LED_INCH, THOU_IN_FRACTION(.007) },
 { ".008", LED_FEED | LED_INCH, THOU_IN_FRACTION(.008) },
 { ".009", LED_FEED | LED_INCH, THOU_IN_FRACTION(.009) },
 { ".010", LED_FEED | LED_INCH, THOU_IN_FRACTION(.010) },
 { ".011", LED_FEED | LED_INCH, THOU_IN_FRACTION(.011) },
 { ".012", LED_FEED | LED_INCH, THOU_IN_FRACTION(.012) },
 { ".013", LED_FEED | LED_INCH, THOU_IN_FRACTION(.013) },
 { ".015", LED_FEED | LED_INCH, THOU_IN_FRACTION(.015) },
 { ".017", LED_FEED | LED_INCH, THOU_IN_FRACTION(.017) },
 { ".020", LED_FEED | LED_INCH, THOU_IN_FRACTION(.020) },
 { ".023", LED_FEED | LED_INCH, THOU_IN_FRACTION(.023) },
 { ".026", LED_FEED | LED_INCH, THOU_IN_FRACTION(.026) },
 { ".030", LED_FEED | LED_INCH, THOU_IN_FRACTION(.030) },
 { ".035", LED_FEED | LED_INCH, THOU_IN_FRACTION(.035) },
 { ".040", LED_FEED | LED_INCH, THOU_IN_FRACTION(.040) },
};




//
// METRIC THREAD DEFINITIONS
//
// Each row in the table defines a standard metric thread, with the display data,
// LED indicator states and gear ratio fraction to use.
//
#if defined(LEADSCREW_TPI)
#define MM_FRACTION(mm) (float(mm) / 25.4 * LEADSCREW_TPI * step_enc_ratio)
#elif defined(LEADSCREW_MM)
#define MM_FRACTION(mm) (float(mm) / float(LEADSCREW_MM) * step_enc_ratio)
#endif

const FEED_THREAD metric_thread_table[] =
{
 { ".20",  LED_THREAD | LED_MM, MM_FRACTION(.20) },
 { ".25",  LED_THREAD | LED_MM, MM_FRACTION(.25) },
 { ".30",  LED_THREAD | LED_MM, MM_FRACTION(.30) },
 { ".35",  LED_THREAD | LED_MM, MM_FRACTION(.35) },
 { ".40",  LED_THREAD | LED_MM, MM_FRACTION(.40) },
 { ".45",  LED_THREAD | LED_MM, MM_FRACTION(.45) },
 { ".50",  LED_THREAD | LED_MM, MM_FRACTION(.50) },
 { ".60",  LED_THREAD | LED_MM, MM_FRACTION(.60) },
 { ".70",  LED_THREAD | LED_MM, MM_FRACTION(.70) },
 { ".75",  LED_THREAD | LED_MM, MM_FRACTION(.75) },
 { ".80",  LED_THREAD | LED_MM, MM_FRACTION(.80) },
 { "1.00", LED_THREAD | LED_MM, MM_FRACTION(1.00) },
 { "1.25", LED_THREAD | LED_MM, MM_FRACTION(1.25) },
 { "1.50", LED_THREAD | LED_MM, MM_FRACTION(1.50) },
 { "1.75", LED_THREAD | LED_MM, MM_FRACTION(1.75) },
 { "2.00", LED_THREAD | LED_MM, MM_FRACTION(2.00) },
 { "2.50", LED_THREAD | LED_MM, MM_FRACTION(2.50) },
 { "3.00", LED_THREAD | LED_MM, MM_FRACTION(3.00) },
 { "3.50", LED_THREAD | LED_MM, MM_FRACTION(3.50) },
 { "4.00", LED_THREAD | LED_MM, MM_FRACTION(4.00) },
 { "4.50", LED_THREAD | LED_MM, MM_FRACTION(4.50) },
 { "5.00", LED_THREAD | LED_MM, MM_FRACTION(5.00) },
 { "5.50", LED_THREAD | LED_MM, MM_FRACTION(5.50) },
 { "6.00", LED_THREAD | LED_MM, MM_FRACTION(6.00) },
};



//
// METRIC FEED DEFINITIONS
//
// Each row in the table defines a standard metric feed, with the display data,
// LED indicator states and gear ratio fraction to use.
//
#if defined(LEADSCREW_TPI)
#define MM_FRACTION_FEED(mm) (float(mm) / 25.4 * LEADSCREW_TPI * step_enc_ratio_feed)
#elif defined(LEADSCREW_MM)
#define MM_FRACTION_FEED(mm) (float(mm) / float(LEADSCREW_MM) * step_enc_ratio_feed)
#endif

const FEED_THREAD metric_feed_table[] =
{
 { ".02",  LED_FEED | LED_MM, MM_FRACTION_FEED(.02) },
 { ".05",  LED_FEED | LED_MM, MM_FRACTION_FEED(.05) },
 { ".07",  LED_FEED | LED_MM, MM_FRACTION_FEED(.07) },
 { ".10",  LED_FEED | LED_MM, MM_FRACTION_FEED(.10) },
 { ".12",  LED_FEED | LED_MM, MM_FRACTION_FEED(.12) },
 { ".15",  LED_FEED | LED_MM, MM_FRACTION_FEED(.15) },
 { ".17",  LED_FEED | LED_MM, MM_FRACTION_FEED(.17) },
 { ".29",  LED_FEED | LED_MM, MM_FRACTION_FEED(.20) },
 { ".22",  LED_FEED | LED_MM, MM_FRACTION_FEED(.22) },
 { ".25",  LED_FEED | LED_MM, MM_FRACTION_FEED(.25) },
 { ".27",  LED_FEED | LED_MM, MM_FRACTION_FEED(.27) },
 { ".30",  LED_FEED | LED_MM, MM_FRACTION_FEED(.30) },
 { ".35",  LED_FEED | LED_MM, MM_FRACTION_FEED(.35) },
 { ".40",  LED_FEED | LED_MM, MM_FRACTION_FEED(.40) },
 { ".45",  LED_FEED | LED_MM, MM_FRACTION_FEED(.45) },
 { ".50",  LED_FEED | LED_MM, MM_FRACTION_FEED(.50) },
 { ".55",  LED_FEED | LED_MM, MM_FRACTION_FEED(.55) },
 { ".60",  LED_FEED | LED_MM, MM_FRACTION_FEED(.60) },
 { ".70",  LED_FEED | LED_MM, MM_FRACTION_FEED(.70) },
 { ".85",  LED_FEED | LED_MM, MM_FRACTION_FEED(.85) },
 { "1.00", LED_FEED | LED_MM, MM_FRACTION_FEED(1.00) },
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
        return thread ? &metricThreads : &metricFeeds;
    }
    else
    {
        return thread ? &inchThreads : &inchFeeds;
    }
}

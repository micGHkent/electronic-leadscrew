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


#include "UserInterface.h"
// KVV
#include "nextion.h"


UserInterface :: UserInterface(void *controlPanel, Core *core, FeedTableFactory *feedTableFactory)
{
//    this->controlPanel = controlPanel;
    this->core = core;
    this->feedTableFactory = feedTableFactory;

    this->metric = false; // start out with imperial
    this->thread = false; // start out with feeds
    this->reverse = false; // start out going forward

    this->feedTable = NULL;

    this->keys.all = 0xff;

    // initialize the core so we start up correctly
    core->setReverse(this->reverse);
    core->setFeed(loadFeedTable());
}

const FEED_THREAD *UserInterface::loadFeedTable()
{
    this->feedTable = this->feedTableFactory->getFeedTable(this->metric, this->thread);
    return this->feedTable->current();
}

LED_REG UserInterface::calculateLEDs()
{
    // get the LEDs for this feed
    LED_REG leds = feedTable->current()->leds;

    if( this->core->isPowerOn() )
    {
        // and add a few of our own
        leds.bit.POWER = 1;
        leds.bit.REVERSE = this->reverse;
        leds.bit.FORWARD = ! this->reverse;
    }
    else
    {
        // power is off
        leds.all = 0;
    }

    return leds;
}

void UserInterface :: setMessage(const MESSAGE *message)
{
    this->message = message;
    this->messageTime = message->displayTime;
}

void UserInterface :: overrideMessage()
{
//    if( this->message != NULL )
//    {
//        if( this->messageTime > 0 ) {
//            this->messageTime--;
//            controlPanel->setMessage(this->message->message);
//        }
//        else {
//            this->message = this->message->next;
//            if( this->message == NULL )
//                controlPanel->setMessage(NULL);
//            else
//                this->messageTime = this->message->displayTime;
//        }
//    }
}

void UserInterface :: loop()
{
    const FEED_THREAD *newFeed = NULL;

    // read the RPM up front so we can use it to make decisions
    Uint16 currentRpm = core->getRPM();

    // display an override message, if there is one
    overrideMessage();

    // just in case, initialize the first time through
    if( feedTable == NULL ) {
        newFeed = loadFeedTable();
    }

    // KVV
    KEY_REG keys;
    keys.all = 0;
    {
        bool at_stop;
        bool enabled = core->isEnabled();
        bool nextion_init = false;
        KEY_REG nkeys = nextion_loop(core->isAlarm(), enabled, at_stop, nextion_init);
        core->setEnabled(enabled);
        if (nkeys.all) {
            keys = nkeys;
        }
        if( nextion_init ) {
            newFeed = loadFeedTable();
        }
    }

    // respond to keypresses
    if( currentRpm == 0 )
    {
        // these keys should only be sensitive when the machine is stopped
        if( keys.bit.POWER ) {
            this->core->setPowerOn(!this->core->isPowerOn());
        }

        // these should only work when the power is on
        if( this->core->isPowerOn() ) {
            if( keys.bit.IN_MM )
            {
                this->metric = ! this->metric;
                newFeed = loadFeedTable();
            }
            if( keys.bit.FEED_THREAD )
            {
                this->thread = ! this->thread;
                newFeed = loadFeedTable();
            }
            if( keys.bit.FWD_REV )
            {
                this->reverse = ! this->reverse;
                core->setReverse(this->reverse);
                // feed table hasn't changed, but we need to trigger an update
                newFeed = loadFeedTable();
            }
            if( keys.bit.SET )
            {
            }
        }
    }

#ifdef IGNORE_ALL_KEYS_WHEN_RUNNING
    if( currentRpm == 0 )
        {
#endif // IGNORE_ALL_KEYS_WHEN_RUNNING

        // these should only work when the power is on
        if( this->core->isPowerOn() ) {
            // these keys can be operated when the machine is running
            if( keys.bit.UP )
            {
                newFeed = feedTable->next();
            }
            if( keys.bit.DOWN )
            {
                newFeed = feedTable->previous();
            }
        }

#ifdef IGNORE_ALL_KEYS_WHEN_RUNNING
    }
#endif // IGNORE_ALL_KEYS_WHEN_RUNNING

    // if we have changed the feed
    if( newFeed != NULL ) {
        // update the control panel
        LED_REG leds = this->calculateLEDs();
	
        // KVV
        // Must pass leds as newFeed->leds is out of date, and may not have foward/reverse set.
        nextion_feed(newFeed, leds);

        // update the core
        core->setFeed(newFeed);
        core->setReverse(this->reverse);
    }

    // KVV
    nextion_rpm(currentRpm);
}

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


UserInterface :: UserInterface(void *controlPanel, Core *core)
{
    this->controlPanel = controlPanel;
    this->core = core;
}

void UserInterface :: loop()
{
    // KVV
    Nextion *nextion = (Nextion*)controlPanel;
    bool updated = nextion->update(core->getRPM(), core->getPosition(), core->isAlarm(), core->isEnabled());
//    core->setEnabled(nextion->isEnabled());
    core->setEnabled(true);
    if (updated) {
        float v;
        bool metric, feed;
        nextion->getFeed(v, metric, feed);
        core->setFeed(v, metric, feed);
        core->setReverse(nextion->isReverse());
    }
}

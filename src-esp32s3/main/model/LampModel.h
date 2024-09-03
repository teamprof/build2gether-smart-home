/* Copyright 2024 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once
#include <stdint.h>
#include <cJSON.h>

class LampModel
{
public:
    static constexpr char NAME[] = "lamp-esp";
    static constexpr char REQ_UPDATE[] = "req-update";
    static constexpr char USER_CLICK[] = "user-click";
    static constexpr char UPDATE[] = "update";

    LampModel();
    ~LampModel();

    void reset(void);

    bool build(const char *device, const char *event, int arg0 = 0, int arg1 = 0);
    bool parse(const char *str);

    const char *stringnify(void);
    void stringDelete(void *str);

    const char *device(void);
    const char *event(void);
    int arg0(void);
    int arg1(void);

private:
    static constexpr char DEVICE[] = "device";
    static constexpr char EVENT[] = "event";
    static constexpr char ARG0[] = "arg0";
    static constexpr char ARG1[] = "arg1";

    cJSON *_root;

    cJSON *_device;
    cJSON *_event;
    int _arg0;
    int _arg1;
};

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
#include <cJSON.h> // ref: https://github.com/DaveGamble/cJSON/blob/master/tests/readme_examples.c
#include "LampModel.h"
#include "ArduProfFreeRTOS.h"

static const char *TAG = "LampModel";

LampModel::LampModel() : _root(NULL)
{
}

LampModel::~LampModel()
{
    if (_root)
    {
        cJSON_Delete(_root);
    }
}

void LampModel::reset(void)
{
    if (_root)
    {
        cJSON_Delete(_root);
        _root = NULL;
    }
}

bool LampModel::build(const char *device, const char *event, int arg0, int arg1)
{
    if (_root)
    {
        cJSON_Delete(_root);
    }

    _device = cJSON_CreateString(device ? device : "");
    _event = cJSON_CreateString(event ? event : "");
    _arg0 = arg0;
    _arg1 = arg1;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, DEVICE, _device);
    cJSON_AddItemToObject(root, EVENT, _event);
    cJSON_AddNumberToObject(root, ARG0, arg0);
    cJSON_AddNumberToObject(root, ARG1, arg1);

    _root = root;
    return root != NULL;
}

bool LampModel::parse(const char *str)
{
    if (!str)
    {
        return false;
    }

    if (_root)
    {
        cJSON_Delete(_root);
    }

    cJSON *root = cJSON_Parse(str);
    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            ESP_LOGI(TAG, "%s: cJSON_Parse() failed: cJSON_GetErrorPtr() returns %s", __func__, error_ptr);
        }
        return false;
    }

    _device = cJSON_GetObjectItem(root, DEVICE);
    _event = cJSON_GetObjectItem(root, EVENT);
    _arg0 = cJSON_GetObjectItem(root, ARG0)->valueint;
    _arg1 = cJSON_GetObjectItem(root, ARG1)->valueint;

    return true;
}

const char *LampModel::stringnify(void)
{
    return (_root ? cJSON_PrintUnformatted(_root) : NULL);
    // return (_root ? cJSON_Print(_root) : NULL);
}

void LampModel::stringDelete(void *str)
{
    if (str)
    {
        cJSON_free(str);
    }
}

const char *LampModel::device(void)
{
    return _device ? _device->valuestring : NULL;
}
const char *LampModel::event(void)
{
    return _event ? _event->valuestring : NULL;
}
int LampModel::arg0(void)
{
    return _arg0;
}
int LampModel::arg1(void)
{
    return _arg1;
}

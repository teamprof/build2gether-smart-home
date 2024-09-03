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
#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include "BleConst.h"
#include "led_service.h"

#define LOG_LEVEL 4
LOG_MODULE_REGISTER(led_service, LOG_LEVEL);

static struct led_service_cb service_cb;
static bool notify_enabled = false;

static bool button_state;

static void onCccChanged(const struct bt_gatt_attr *attr, uint16_t value)
{
    LOG_DBG("attr->handle=%u, value=%u, BT_GATT_CCC_NOTIFY=%u", attr->handle, value, BT_GATT_CCC_NOTIFY);
    notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t onChrcWrite(struct bt_conn *conn,
                           const struct bt_gatt_attr *attr,
                           const void *buf,
                           uint16_t len, uint16_t offset, uint8_t flags)
{
    LOG_DBG("conn=%p, attr->handle=%u, len=%u offset=%u, flags=0x%04x", (void *)conn, attr->handle, len, offset, flags);

    if (len != 1U)
    {
        LOG_DBG("Write led: Incorrect data length");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    if (offset != 0)
    {
        LOG_DBG("Write led: Incorrect data offset");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }

    if (service_cb.write)
    {
        uint8_t val = *((uint8_t *)buf);

        if (val == 0x00 || val == 0x01)
        {
            service_cb.write(val ? true : false);
        }
        else
        {
            LOG_DBG("Write led: Incorrect value");
            return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
        }
    }

    return len;
}

static ssize_t onChrcRead(struct bt_conn *conn,
                          const struct bt_gatt_attr *attr,
                          void *buf,
                          uint16_t len,
                          uint16_t offset)
{
    LOG_DBG("conn=%p, attr->handle=%u, len=%u offset=%u", (void *)conn, attr->handle, len, offset);

    if (service_cb.read)
    {
        bool state = service_cb.read();
        return bt_gatt_attr_read(conn, attr, buf, len, offset, &state, sizeof(state));
    }

    return 0;
}

// LED Service Declaration
BT_GATT_SERVICE_DEFINE(ledService,
                       BT_GATT_PRIMARY_SERVICE(BT_UUID_LED_SERV),
                       BT_GATT_CHARACTERISTIC(BT_UUID_LED_CHRC,
                                              BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
                                              BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                                              onChrcRead, onChrcWrite, &button_state),
                       BT_GATT_CCC(onCccChanged,
                                   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), );

int ble_led_service_init(struct led_service_cb *cb)
{
    if (cb)
    {
        service_cb.write = cb->write;
        service_cb.read = cb->read;
    }
    return 0;
}

int ble_led_notify(bool value)
{
    return notify_enabled ? bt_gatt_notify(NULL, &ledService.attrs[2], &value, sizeof(value)) : -EACCES;
}

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
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <soc.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <dk_buttons_and_leds.h>

#include "./main.h"
#include "./AppEvent.h"
#include "./BleConst.h"
#include "./LedConst.h"
#include "./led_service.h"

#define LOG_LEVEL 4
LOG_MODULE_REGISTER(ble_task, LOG_LEVEL);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_LED_SERV_VAL),
};

static void onConnected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        LOG_DBG("Connection failed (err %u)", err);
        return;
    }
    LOG_DBG("Connected");
    postMainEvent(EventBleConnection, BleConnected, 0, 0);
}

static void onDisconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_DBG("Disconnected: reason=0x%04x", reason);
    postMainEvent(EventBleConnection, BleDisconnected, 0, 0);
}

#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
static void onSecurityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    if (!err)
    {
        LOG_DBG("Security changed: %s level %u", addr, level);
    }
    else
    {
        LOG_DBG("Security failed: %s level %u err %d", addr, level, err);
    }
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = onConnected,
    .disconnected = onDisconnected,
#ifdef CONFIG_BT_LBS_SECURITY_ENABLED
    .security_changed = onSecurityChanged,
#endif
};

#if defined(CONFIG_BT_LBS_SECURITY_ENABLED)
static void onAuthPasskeyDisplay(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Passkey for %s: %06u", addr, passkey);
}

static void onAuthCancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Pairing cancelled: %s", addr);
}

static void onPairingComplete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void onPairingFailed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_DBG("Pairing failed conn: %s, reason %d", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = onAuthPasskeyDisplay,
    .cancel = onAuthCancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = onPairingComplete,
    .pairing_failed = onPairingFailed};
#else
static struct bt_conn_auth_cb conn_auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;
#endif

bool ble_init(struct led_service_cb *cb)
{
    int err;

    LOG_DBG("ble_init() ...");

    if (IS_ENABLED(CONFIG_BT_LBS_SECURITY_ENABLED))
    {
        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err)
        {
            LOG_ERR("Failed to register authorization callbacks: err=%d", err);
            return false;
        }

        err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
        if (err)
        {
            LOG_ERR("Failed to register authorization info callbacks: err=%d", err);
            return false;
        }
    }

    err = bt_enable(NULL);
    if (err)
    {
        LOG_ERR("Bluetooth init failed: err=%d", err);
        return false;
    }

    LOG_DBG("Bluetooth initialized");

    if (IS_ENABLED(CONFIG_SETTINGS))
    {
        settings_load();
    }

    err = ble_led_service_init(cb);
    if (err)
    {
        LOG_ERR("Failed to init LBS: err=%d", err);
        return false;
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err)
    {
        LOG_ERR("Advertising failed to start: err=%d", err);
        return false;
    }

    LOG_DBG("Advertising successfully started");
    return true;
}

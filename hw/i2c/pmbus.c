/*
 * QEMU PMBus device
 *
 * Andrew Jeffery <andrew@aj.id.au>
 *
 * Copyright 2017 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/hw.h"
#include "hw/i2c/i2c.h"
#include "hw/i2c/smbus.h"

#define PMBUS_C_PAGE                    0x00
#define PMBUS_C_CLEAR_FAULTS            0x03
#define PMBUS_C_CAPABILITY              0x19
#define  PMBUS_CAPABILITY_PEC           BIT(7)
#define  PMBUS_CAPABILITY_MAX_SPEED     GENMASK(6, 5)
#define  PMBUS_CAPABILITY_SMBALERT      BIT(4)
#define  PMBUS_CAPABILITY_RESERVED      GENMASK(3, 0)
#define PMBUS_C_VOUT_MODE               0x20
#define  PMBUS_VOUT_MODE_MODE           GENMASK(7, 5)
#define  PMBUS_VOUT_MODE_PARAM          GENMAKS(4, 0)
#define PMBUS_C_FAN_CONFIG_1_2          0x3a
#define  PMBUS_FAN_CONFIG_A_INSTALLED   BIT(7)
#define  PMBUS_FAN_CONFIG_A_RPM         BIT(6)
#define  PMBUS_FAN_CONFIG_A_PULSES      GENMAKS(5, 4)
#define  PMBUS_FAN_CONFIG_B_INSTALLED   BIT(3)
#define  PMBUS_FAN_CONFIG_B_RPM         BIT(2)
#define  PMBUS_FAN_CONFIG_B_PULSES      GENMAKS(1, 0)
#define PMBUS_C_FAN_COMMAND_1           0x3b
#define PMBUS_C_FAN_COMMAND_2           0x3c
#define PMBUS_C_FAN_CONFIG_3_4          0x3d
#define PMBUS_C_FAN_COMMAND_3           0x3e
#define PMBUS_C_FAN_COMMAND_4           0x3f
#define PMBUS_C_STATUS_BYTE             0x78
#define  PMBUS_STATUS_BYTE_BUSY         BIT(7)
#define  PMBUS_STATUS_BYTE_OFF          BIT(6)
#define  PMBUS_STATUS_BYTE_VOUT_OV      BIT(5)
#define  PMBUS_STATUS_BYTE_IOUT_OC      BIT(4)
#define  PMBUS_STATUS_BYTE_VIN_UV       BIT(3)
#define  PMBUS_STATUS_BYTE_TEMPERATURE  BIT(2)
#define  PMBUS_STATUS_BYTE_CML          BIT(1)
#define  PMBUS_STATUS_BYTE_NONE_ABOVE   BIT(0)
#define PMBUS_C_STATUS_WORD             0x79
#define PMBUS_C_STATUS_CML              0x7e
#define  PMBUS_STATUS_CML_INV_CMD       BIT(7)
#define  PMBUS_STATUS_CML_INV_DATA      BIT(6)
#define  PMBUS_STATUS_CML_PEC_FAIL      BIT(5)
#define  PMBUS_STATUS_CML_MEM_FAULT     BIT(4)
#define  PMBUS_STATUS_CML_PROC_FAULT    BIT(3)
#define  PMBUS_STATUS_CML_RSVD          BIT(2)
#define  PMBUS_STATUS_CML_OTHER_COMMS   BIT(1)
#define  PMBUS_STATUS_CML_OTHER_MEM     BIT(0)
#define PMBUS_C_STATUS_FANS_1_2         0x81
#define  PMBUS_STATUS_FANS_FAULT_A      BIT(7)
#define  PMBUS_STATUS_FANS_FAULT_B      BIT(6)
#define  PMBUS_STATUS_FANS_WARN_A       BIT(5)
#define  PMBUS_STATUS_FANS_WARN_B       BIT(4)
#define  PMBUS_STATUS_FANS_OVER_A       BIT(3)
#define  PMBUS_STATUS_FANS_OVER_B       BIT(2)
#define  PMBUS_STATUS_FANS_AIR_A        BIT(1)
#define  PMBUS_STATUS_FANS_AIR_B        BIT(0)
#define PMBUS_C_STATUS_FANS_3_4         0x82
#define PMBUS_C_READ_FAN_SPEED_1        0x90
#define PMBUS_C_READ_FAN_SPEED_2        0x91
#define PMBUS_C_READ_FAN_SPEED_3        0x92
#define PMBUS_C_READ_FAN_SPEED_4        0x93

typedef struct PMBusPage {
    uint8_t capability;
    uint8_t vout_mode;
    uint8_t fan_config_1_2;
    uint16_t fan_command_1;
    uint16_t fan_command_2;
    uint8_t fan_config_3_4;
    uint16_t fan_command_3;
    uint16_t fan_command_4;
    uint8_t status_byte;
    uint8_t status_fans_1_2;
    uint8_t status_fans_3_4;
    uint16_t fan_speed_1;
    uint8_t status_cml;
} PMBusPage;

typedef struct PMBusDevice {
    SMBusDevice smbusdev;
    uint8_t page;
    uint8_t nr_pages;
    PMBusPage *pages;
} PMBusDevice;

#ifdef DEBUG
#define dprint(...) fprintf(stderr, __VA_ARGS__)
#else
#define dprint(...)
#endif

static void pmbus_quick_cmd(SMBusDevice *dev, uint8_t read)
{
    dprint("%s:%d\n", __func__, __LINE__);
    return;
}

static void pmbus_send_byte(SMBusDevice *dev, uint8_t cmd)
{
    PMBusDevice *pmbus = (PMBusDevice *) dev;
    PMBusPage *pg = &pmbus->pages[pmbus->page];

    dprint("%s:%d: sent %u\n", __func__, __LINE__, cmd);
    switch (cmd) {
        case PMBUS_C_CLEAR_FAULTS:
            pg->status_byte = 0;
            pg->status_cml = 0;
            break;
        default:
            pg->status_byte |= PMBUS_STATUS_BYTE_CML;
            pg->status_cml |= PMBUS_STATUS_CML_INV_CMD;
            break;
    }
    return;
}

static uint8_t pmbus_receive_byte(SMBusDevice *dev)
{
    dprint("%s:%d\n", __func__, __LINE__);
    return 0;
}

static void pmbus_write_data(SMBusDevice *dev, uint8_t cmd, uint8_t *buf, int len)
{
    PMBusDevice *pmbus = (PMBusDevice *) dev;
    PMBusPage *pg = &pmbus->pages[pmbus->page];
    int i;

    dprint("%s:%d, cmd: 0x%x\n", __func__, __LINE__, cmd);

    for (i = 0; i < len; i++) {
        dprint("0x%x ", buf[i]);
    }
    dprint("\n");

    switch (cmd) {
        case PMBUS_C_PAGE:
            pmbus->page = *buf;
            break;
        case PMBUS_C_FAN_CONFIG_1_2:
            pg->fan_config_1_2 = *buf;
            break;
        case PMBUS_C_FAN_COMMAND_1:
            if (len != 2) {
                qemu_log_mask(LOG_GUEST_ERROR, "Unexpected length: %d\n", len);
                return;
            }
            memcpy(&pg->fan_command_1, buf, sizeof(pg->fan_command_1));
            break;
        case PMBUS_C_FAN_COMMAND_2:
            if (len != 2) {
                qemu_log_mask(LOG_GUEST_ERROR, "Unexpected length: %d\n", len);
                return;
            }
            memcpy(&pg->fan_command_2, buf, sizeof(pg->fan_command_2));
            break;
        case PMBUS_C_FAN_CONFIG_3_4:
            pg->fan_config_3_4 = *buf;
            break;
        case PMBUS_C_FAN_COMMAND_3:
            if (len != 2) {
                qemu_log_mask(LOG_GUEST_ERROR, "Unexpected length: %d\n", len);
                return;
            }
            memcpy(&pg->fan_command_3, buf, sizeof(pg->fan_command_3));
            break;
        case PMBUS_C_FAN_COMMAND_4:
            if (len != 2) {
                qemu_log_mask(LOG_GUEST_ERROR, "Unexpected length: %d\n", len);
                return;
            }
            memcpy(&pg->fan_command_4, buf, sizeof(pg->fan_command_4));
            break;
        case PMBUS_C_STATUS_FANS_1_2:
            pg->status_fans_1_2 = *buf;
            break;
        case PMBUS_C_STATUS_FANS_3_4:
            pg->status_fans_3_4 = *buf;
            break;
        default:
            pg->status_byte |= PMBUS_STATUS_BYTE_CML;
            pg->status_cml |= PMBUS_STATUS_CML_INV_CMD;
            break;
    }

    return;
}

static uint8_t pmbus_read_data(SMBusDevice *dev, uint8_t cmd, int n)
{
    PMBusDevice *pmbus = (PMBusDevice *) dev;
    PMBusPage *pg = &pmbus->pages[pmbus->page];
    uint8_t val = 0;

    switch (cmd) {
        case PMBUS_C_PAGE:
            val = pmbus->page;
            break;
        case PMBUS_C_CAPABILITY:
            val = pg->capability;
            break;
        case PMBUS_C_VOUT_MODE:
            val = pg->vout_mode;
            break;
        case PMBUS_C_FAN_CONFIG_1_2:
            val = pg->fan_config_1_2;
            break;
        case PMBUS_C_FAN_COMMAND_1:
            val = le16toh(pg->fan_command_1) >> (8 * n);
            break;
        case PMBUS_C_FAN_COMMAND_2:
            val = le16toh(pg->fan_command_2) >> (8 * n);
            break;
        case PMBUS_C_FAN_CONFIG_3_4:
            val = pg->fan_config_3_4;
            break;
        case PMBUS_C_FAN_COMMAND_3:
            val = le16toh(pg->fan_command_3) >> (8 * n);
            break;
        case PMBUS_C_FAN_COMMAND_4:
            val = le16toh(pg->fan_command_4) >> (8 * n);
            break;
        case PMBUS_C_STATUS_BYTE:
            val = pg->status_byte;
            break;
        case PMBUS_C_STATUS_FANS_1_2:
            val = pg->status_fans_1_2;
            break;
        case PMBUS_C_STATUS_FANS_3_4:
            val = pg->status_fans_3_4;
            break;
        case PMBUS_C_READ_FAN_SPEED_1:
            if (n == 0)
                pg->fan_speed_1 = htole16(le16toh(pg->fan_speed_1) + 100);
            val = le16toh(pg->fan_speed_1) >> (8 * n);
            break;
        default:
            pg->status_byte |= PMBUS_STATUS_BYTE_CML;
            pg->status_cml |= PMBUS_STATUS_CML_INV_CMD;
            break;
    }

    dprint("%s:%d: cmd: 0x%x, n: %d page: %d, val: 0x%x\n",
            __func__, __LINE__, cmd, n, pmbus->page, val);

    return val;
}

static int pmbus_initfn(SMBusDevice *dev)
{
    PMBusDevice *pmbus = (PMBusDevice *) dev;
    int i;

    dprint("%s:%d\n", __func__, __LINE__);

    /* Maxim MAX31785 */
    pmbus->nr_pages = 23;
    pmbus->pages = calloc(pmbus->nr_pages, sizeof(*pmbus->pages));
    if (!pmbus->pages)
        return -1;
    for (i = 0; i < pmbus->nr_pages; i++) {
        PMBusPage *pg = &pmbus->pages[i];

        pg->vout_mode = 0x40;
        if (i < 6)
            pg->fan_config_1_2 = PMBUS_FAN_CONFIG_A_INSTALLED;
    }
    return 0;
}

static Property pmbus_properties[] = {
    DEFINE_PROP_END_OF_LIST(),
};

static void pmbus_class_initfn(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    SMBusDeviceClass *sc = SMBUS_DEVICE_CLASS(klass);

    sc->init = pmbus_initfn;
    sc->quick_cmd = pmbus_quick_cmd;
    sc->send_byte = pmbus_send_byte;
    sc->receive_byte = pmbus_receive_byte;
    sc->write_data = pmbus_write_data;
    sc->read_data = pmbus_read_data;
    dc->props = pmbus_properties;
}

static const TypeInfo pmbus_info = {
    .name          = "pmbus-device",
    .parent        = TYPE_SMBUS_DEVICE,
    .instance_size = sizeof(PMBusDevice),
    .class_init    = pmbus_class_initfn,
};

static void pmbus_register_types(void)
{
    type_register_static(&pmbus_info);
}

type_init(pmbus_register_types)

/*
 * ASPEED System Control Unit
 *
 * Andrew Jeffery <andrew@aj.id.au>
 *
 * Copyright 2016 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include <inttypes.h>
#include "hw/misc/aspeed_scu.h"
#include "hw/qdev-properties.h"
#include "qemu/bitops.h"
#include "trace.h"

#define SCU_KEY 0x1688A8A8
#define SCU_IO_REGION_SIZE 0x20000

#define TO_REG(o) (o >> 2)

#define SCU00 TO_REG(0x00)
#define SCU0C TO_REG(0x0C)
#define SCU2C TO_REG(0x2C)
#define SCU3C TO_REG(0x3C)
#define SCU70 TO_REG(0x70)
#define SCU7C TO_REG(0x7C)
#define SCU80 TO_REG(0x80)
#define SCU84 TO_REG(0x84)
#define SCU88 TO_REG(0x88)
#define SCU8C TO_REG(0x8C)
#define SCU90 TO_REG(0x90)
#define SCU94 TO_REG(0x94)
#define SCU9C TO_REG(0x9C)

static uint64_t aspeed_scu_read(void *opaque, hwaddr offset, unsigned size)
{
    AspeedSCUState *s = ASPEED_SCU(opaque);

    if (TO_REG(offset) >= ARRAY_SIZE(s->regs)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Out-of-bounds read at offset 0x%" HWADDR_PRIx "\n",
                      __func__, offset);
        return 0;
    }

    switch (offset) {
        case 0x00:
        case 0x0C:
        case 0x2C:
        case 0x3C:
        case 0x70:
        case 0x7C:
        case 0x80 ... 0x9C:
            break;
        default:
            qemu_log_mask(LOG_UNIMP,
                          "%s: Read from uninitialised register 0x%" HWADDR_PRIx "\n",
                          __func__, offset);
            break;
    }

    return s->regs[TO_REG(offset)];
}

static void aspeed_scu_write(void *opaque, hwaddr offset, uint64_t data,
                             unsigned size)
{
    AspeedSCUState *s = ASPEED_SCU(opaque);

    if (TO_REG(offset) >= ARRAY_SIZE(s->regs)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Out-of-bounds write at offset 0x%" HWADDR_PRIx "\n",
                      __func__, offset);
        return;
    }

    if (TO_REG(offset) != SCU00 && s->regs[SCU00] != SCU_KEY) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: SCU is locked!\n", __func__);
        return;
    }

    switch (offset) {
        case 0x00:
        case 0x0C:
        case 0x2C:
        case 0x3C:
        case 0x70:
        case 0x80 ... 0x9C:
            break;
        case 0x7C:
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: Write to read-only offset 0x%" HWADDR_PRIx "\n",
                          __func__, offset);
            break;
        default:
            qemu_log_mask(LOG_UNIMP,
                          "%s: Write to uninitialised register 0x%" HWADDR_PRIx "\n",
                          __func__, offset);
            break;
    }

    s->regs[TO_REG(offset)] = (uint32_t) data;
}

static const MemoryRegionOps aspeed_scu_ops = {
    .read = aspeed_scu_read,
    .write = aspeed_scu_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .valid.unaligned = false,
};

static void aspeed_scu_reset(DeviceState *dev)
{
    AspeedSCUState *s = ASPEED_SCU(dev);

    s->regs[SCU00] = 0;
    s->regs[SCU0C] = 0x19FC3E8BU;
    s->regs[SCU2C] = 0x00000010U;
    s->regs[SCU3C] = 0x00000001U;
    s->regs[SCU70] = 0;
    s->regs[SCU7C] = 0x02000303U;
    s->regs[SCU80] = 0;
    s->regs[SCU84] = 0x0000F000U;
    s->regs[SCU88] = 0x01000000U;
    s->regs[SCU8C] = 0x000000FFU;
    s->regs[SCU90] = 0x0000A000U;
    s->regs[SCU94] = 0;
    s->regs[SCU9C] = 0x003FFFF3U;
}

static void aspeed_scu_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    AspeedSCUState *s = ASPEED_SCU(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &aspeed_scu_ops, s,
                          TYPE_ASPEED_SCU, SCU_IO_REGION_SIZE);

    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription vmstate_aspeed_scu = {
    .name = "aspeed.new-vic",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, AspeedSCUState, ASPEED_SCU_NR_REGS),
        VMSTATE_END_OF_LIST()
    }
};

static void aspeed_scu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = aspeed_scu_realize;
    dc->reset = aspeed_scu_reset;
    dc->desc = "ASPEED System Control Unit";
    dc->vmsd = &vmstate_aspeed_scu;
}

static const TypeInfo aspeed_scu_info = {
    .name = TYPE_ASPEED_SCU,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AspeedSCUState),
    .class_init = aspeed_scu_class_init,
};

static void aspeed_scu_register_types(void)
{
    type_register_static(&aspeed_scu_info);
}

type_init(aspeed_scu_register_types);

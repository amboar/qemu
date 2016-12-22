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
#include "hw/misc/aspeed_lpc.h"
#include "qemu/log.h"
#include "trace.h"

#define TO_REG(offset) ((offset) >> 2)
#define LPC_IO_REGION_SIZE 0x1000

static uint64_t aspeed_lpc_read(void *opaque, hwaddr offset, unsigned size)
{
    AspeedLPCState *s = ASPEED_LPC(opaque);
    int reg = TO_REG(offset);

    if (reg >= ARRAY_SIZE(s->regs)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Out-of-bounds read at offset 0x%" HWADDR_PRIx "\n",
                      __func__, offset);
        return 0;
    }

    trace_aspeed_lpc_read(offset, size, s->regs[reg]);

    return s->regs[reg];
}

static void aspeed_lpc_write(void *opaque, hwaddr offset, uint64_t data,
                             unsigned size)
{
    AspeedLPCState *s = ASPEED_LPC(opaque);
    int reg = TO_REG(offset);

    if (reg >= ARRAY_SIZE(s->regs)) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Out-of-bounds write at offset 0x%" HWADDR_PRIx "\n",
                      __func__, offset);
        return;
    }

    trace_aspeed_lpc_write(offset, size, data);

    s->regs[reg] = data;
}

static const MemoryRegionOps aspeed_lpc_ops = {
    .read = aspeed_lpc_read,
    .write = aspeed_lpc_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .valid.unaligned = false,
};

static void aspeed_lpc_reset(DeviceState *dev)
{
    AspeedLPCState *s = ASPEED_LPC(dev);

    memset(s->regs, '\0', sizeof(s->regs));
}

static void aspeed_lpc_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    AspeedLPCState *s = ASPEED_LPC(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &aspeed_lpc_ops, s,
                          TYPE_ASPEED_LPC, LPC_IO_REGION_SIZE);

    sysbus_init_mmio(sbd, &s->iomem);
}

static const VMStateDescription vmstate_aspeed_lpc = {
    .name = "aspeed.lpc",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, AspeedLPCState, ASPEED_LPC_NR_REGS),
        VMSTATE_END_OF_LIST()
    }
};

static void aspeed_lpc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = aspeed_lpc_realize;
    dc->reset = aspeed_lpc_reset;
    dc->desc = "ASPEED System Control Unit";
    dc->vmsd = &vmstate_aspeed_lpc;
}

static const TypeInfo aspeed_lpc_info = {
    .name = TYPE_ASPEED_LPC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AspeedLPCState),
    .class_init = aspeed_lpc_class_init,
};

static void aspeed_lpc_register_types(void)
{
    type_register_static(&aspeed_lpc_info);
}

type_init(aspeed_lpc_register_types);

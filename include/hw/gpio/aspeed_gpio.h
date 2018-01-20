/*
 * ASPEED GPIO Controller
 *
 * Andrew Jeffery <andrew@aj.id.au>
 *
 * Copyright 2017 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */
#ifndef ASPEED_GPIO_H
#define ASPEED_GPIO_H

#include "hw/sysbus.h"

#define TYPE_ASPEED_GPIO "aspeed.gpio"
#define ASPEED_GPIO(obj) OBJECT_CHECK(AspeedGPIOState, (obj), TYPE_ASPEED_GPIO)

#define ASPEED_SCU_NR_REGS (0x1F0 >> 2)

typedef struct AspeedGPIOState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;

    qemu_irq irq;

    uint32_t regs[ASPEED_SCU_NR_REGS];
    uint32_t silicon_rev;
} AspeedSCUState;

#endif

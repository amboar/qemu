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
#ifndef ASPEED_LPC_H
#define ASPEED_LPC_H

#include "hw/sysbus.h"

#define TYPE_ASPEED_LPC "aspeed.lpc"
#define ASPEED_LPC(obj) OBJECT_CHECK(AspeedLPCState, (obj), TYPE_ASPEED_LPC)

#define ASPEED_LPC_NR_REGS (0x260 >> 2)

typedef struct AspeedLPCState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;

    uint32_t regs[ASPEED_LPC_NR_REGS];
} AspeedLPCState;

#endif /* ASPEED_SCU_H */

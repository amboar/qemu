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
#ifndef ASPEED_SCU_H
#define ASPEED_SCU_H

#include "hw/sysbus.h"

#define TYPE_ASPEED_SCU "aspeed.scu"
#define ASPEED_SCU(obj) OBJECT_CHECK(AspeedSCUState, (obj), TYPE_ASPEED_SCU)

#define ASPEED_SCU_NR_REGS (0x1B4 >> 2)

typedef struct AspeedSCUState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    MemoryRegion iomem;

    uint32_t regs[ASPEED_SCU_NR_REGS];

    /* Reset properties */
    uint32_t scu04_rst;
    uint32_t scu08_rst;
    uint32_t scu0c_rst;
    uint32_t scu24_rst;
    uint32_t scu70_rst;
    uint32_t scu7c_rst;
    uint32_t scu88_rst;
    uint32_t scu8c_rst;
    uint32_t scu9c_rst;
} AspeedSCUState;

#endif /* ASPEED_SCU_H */

/*
 * ASPEED AST2500 SoC
 *
 * Joel Stanley <joel@jms.id.au>
 *
 * Copyright 2016 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#ifndef AST2500_H
#define AST2500_H

#include "hw/arm/arm.h"
#include "hw/intc/aspeed_vic.h"
#include "hw/timer/aspeed_timer.h"

typedef struct AST2500State {
    /*< private >*/
    DeviceState parent;

    /*< public >*/
    ARMCPU *cpu;
    MemoryRegion iomem;
    AspeedVICState vic;
    AspeedTimerCtrlState timerctrl;
} AST2500State;

#define TYPE_AST2500 "ast2500"
#define AST2500(obj) OBJECT_CHECK(AST2500State, (obj), TYPE_AST2500)

#define AST2500_SDRAM_BASE       0x80000000

#endif /* AST2500_H */

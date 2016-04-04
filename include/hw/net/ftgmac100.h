/*
 *  Copyright 2016 IBM Corp.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */

#ifndef FTGMAC100_H
#define FTGMAC100_H

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "net/net.h"

#define TYPE_FTGMAC100  "ftgmac100"

#define FTGMAC100(obj) \
    OBJECT_CHECK(Ftgmac100State, obj, TYPE_FTGMAC100)

#define CFG_MAXFRMLEN   9220    /* Max. frame length */
#define CFG_REGSIZE     (0x100 / 4)

typedef struct Ftgmac100State {
    /*< private >*/
    SysBusDevice parent;

    /*< public >*/
    MemoryRegion mmio;

    QEMUBH *bh;
    qemu_irq irq;
    NICState *nic;
    NICConf conf;
    AddressSpace *dma;
    QEMUTimer *qtimer;

    bool phycr_rd;

    struct {
        uint8_t  buf[CFG_MAXFRMLEN];
        uint32_t len;
    } txbuff;

    uint32_t hptx_idx;
    uint32_t tx_idx;
    uint32_t rx_idx;

    /* HW register cache */
    uint32_t regs[CFG_REGSIZE];
} Ftgmac100State;

#endif

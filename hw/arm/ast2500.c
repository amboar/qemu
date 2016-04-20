/*
 * AST2500 SoC
 *
 * Andrew Jeffery <andrew@aj.id.au>
 * Jeremy Kerr <jk@ozlabs.org>
 * Joel Stanley <joel@jms.id.au>
 *
 * Copyright 2016 IBM Corp.
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu-common.h"
#include "cpu.h"
#include "exec/address-spaces.h"
#include "hw/arm/ast2500.h"
#include "hw/char/serial.h"

#define AST2500_UART_5_BASE      0x00184000
#define AST2500_IOMEM_SIZE       0x00200000
#define AST2500_IOMEM_BASE       0x1E600000
#define AST2500_VIC_BASE         0x1E6C0000
#define AST2500_SCU_BASE         0x1E6E2000
#define AST2500_TIMER_BASE       0x1E782000

static const int uart_irqs[] = { 9, 32, 33, 34, 10 };
static const int timer_irqs[] = { 16, 17, 18, 35, 36, 37, 38, 39, };

/*
 * IO handlers: simply catch any reads/writes to IO addresses that aren't
 * handled by a device mapping.
 */

static uint64_t ast2500_io_read(void *p, hwaddr offset, unsigned size)
{
    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " [%u]\n",
                  __func__, offset, size);
    return 0;
}

static void ast2500_io_write(void *opaque, hwaddr offset, uint64_t value,
                unsigned size)
{
    qemu_log_mask(LOG_UNIMP, "%s: 0x%" HWADDR_PRIx " <- 0x%" PRIx64 " [%u]\n",
                  __func__, offset, value, size);
}

static const MemoryRegionOps ast2500_io_ops = {
    .read = ast2500_io_read,
    .write = ast2500_io_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void ast2500_init(Object *obj)
{
    AST2500State *s = AST2500(obj);

    s->cpu = cpu_arm_init("arm1176");

    object_initialize(&s->vic, sizeof(s->vic), TYPE_ASPEED_VIC);
    object_property_add_child(obj, "vic", OBJECT(&s->vic), NULL);
    qdev_set_parent_bus(DEVICE(&s->vic), sysbus_get_default());

    object_initialize(&s->timerctrl, sizeof(s->timerctrl), TYPE_ASPEED_TIMER);
    object_property_add_child(obj, "timerctrl", OBJECT(&s->timerctrl), NULL);
    qdev_set_parent_bus(DEVICE(&s->timerctrl), sysbus_get_default());

    object_initialize(&s->scu, sizeof(s->scu), TYPE_ASPEED_SCU);
    object_property_add_child(obj, "scu", OBJECT(&s->scu), NULL);
    qdev_set_parent_bus(DEVICE(&s->scu), sysbus_get_default());
}

static void ast2500_realize(DeviceState *dev, Error **errp)
{
    int i;
    AST2500State *s = AST2500(dev);
    Error *err = NULL;

    /* IO space */
    memory_region_init_io(&s->iomem, NULL, &ast2500_io_ops, NULL,
            "ast2500.io", AST2500_IOMEM_SIZE);
    memory_region_add_subregion_overlap(get_system_memory(), AST2500_IOMEM_BASE,
            &s->iomem, -1);

    /* VIC */
    object_property_set_int(OBJECT(&s->vic),
			    0xFFFFCF07FFF8FFFFULL, "sense", &err);
    object_property_set_int(OBJECT(&s->vic),
			    0x000000F800070000ULL, "dual_edge", &err);
    object_property_set_int(OBJECT(&s->vic),
			    0xFFFCCF07FFF8FFFFULL, "event", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    object_property_set_bool(OBJECT(&s->vic), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->vic), 0, AST2500_VIC_BASE);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->vic), 0,
                       qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_IRQ));
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->vic), 1,
                       qdev_get_gpio_in(DEVICE(s->cpu), ARM_CPU_FIQ));

    /* Timer */
    object_property_set_bool(OBJECT(&s->timerctrl), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->timerctrl), 0, AST2500_TIMER_BASE);
    for (i = 0; i < ARRAY_SIZE(timer_irqs); i++) {
        qemu_irq irq = qdev_get_gpio_in(DEVICE(&s->vic), timer_irqs[i]);
        sysbus_connect_irq(SYS_BUS_DEVICE(&s->timerctrl), i, irq);
    }

    /* SCU */
    object_property_set_int(OBJECT(&s->scu), 0xEFF43E8BU, "scu0c", &err);
    object_property_set_int(OBJECT(&s->scu), 0x93000400U, "scu24", &err);
    /* SCU70 bit 23: 0 24Mhz. bit 11:9: 0b001 AXI:ABH ratio 2:1 */
    object_property_set_int(OBJECT(&s->scu), 0x00000200U, "scu70", &err);
    /* AST2500 revision A1 */
    object_property_set_int(OBJECT(&s->scu), 0x04010303U, "scu7c", &err);
    object_property_set_int(OBJECT(&s->scu), 0x03000000U, "scu88", &err);
    object_property_set_int(OBJECT(&s->scu), 0x00000000U, "scu8c", &err);
    object_property_set_int(OBJECT(&s->scu), 0x023FFFF3U, "scu9c", &err);

    object_property_set_bool(OBJECT(&s->scu), true, "realized", &err);
    if (err) {
        error_propagate(errp, err);
	return;
    }
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->scu), 0, AST2500_SCU_BASE);

    /* UART - attach an 8250 to the IO space as our UART5 */
    if (serial_hds[0]) {
        qemu_irq uart5 = qdev_get_gpio_in(DEVICE(&s->vic), uart_irqs[4]);
        serial_mm_init(&s->iomem, AST2500_UART_5_BASE, 2,
                       uart5, 38400, serial_hds[0], DEVICE_LITTLE_ENDIAN);
    }
}

static void ast2500_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = ast2500_realize;

    /*
     * Reason: creates an ARM CPU, thus use after free(), see
     * arm_cpu_class_init()
     */
    dc->cannot_destroy_with_object_finalize_yet = true;
}

static const TypeInfo ast2500_type_info = {
    .name = TYPE_AST2500,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AST2500State),
    .instance_init = ast2500_init,
    .class_init = ast2500_class_init,
};

static void ast2500_register_types(void)
{
    type_register_static(&ast2500_type_info);
}

type_init(ast2500_register_types)

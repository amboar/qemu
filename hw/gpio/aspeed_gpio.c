#include "qemu/osdep.h"
#include "hw/gpio/aspeed_gpio.h"
#include "qemu/log.h"
#include "trace.h"

#define TO_GROUP(n)     ((n) / (8 * 4))
#define TO_BANK(n)      (((n) / 8) % 4)
#define TO_INDEX(n)     ((n) % 8)
#define GROUP_BIT(n)    (BIT(TO_INDEX(n)) << (8 * TO_BANK(n)))

struct aspeed_gpio_group {
    uint16_t data;
    uint16_t dir;
    uint16_t irq_enable;
    uint16_t irq_sense;
    uint16_t irq_status;
    uint16_t reset;
    uint16_t debounce;
};

#define TO_REG(offset) ((offset) >> 2)

static const struct aspeed_gpio_group gpio_groups[] = {
    {           /* A/B/C/D */
        .data           = TO_REG(0x000),
        .dir            = TO_REG(0x004),
        .irq_enable     = TO_REG(0x008),
        .irq_sense      = TO_REG(0x00C),
        .irq_status     = TO_REG(0x018),
        .reset          = TO_REG(0x01C),
        .debounce       = TO_REG(0x040)
    }, {        /* E/F/G/H */
    }
}

static bool aspeed_gpio_output_only(const AspeedGPIOState *s, int n)
{
    /* TODO: Derive GPIO line capabilities from silicon revision and test here */
    return false;
}

static inline bool aspeed_gpio_irq_enabled(AspeedGPIOState *s,
        struct aspeed_gpio_group *g, int n)
{
    return s->regs[g->irq_enabled] & GROUP_BIT(n);
}

/*
 * These helpers do very similar calculations, but it makes the irq handler
 * read a bit cleaner than if we did a bunch of nested conditionals.
 */
static inline bool aspeed_gpio_irq_low(AspeedGPIOState *s,
        struct aspeed_gpio_group *g, int n)
{
    bool level = s->regs[g->irq_sense + 1] & GROUP_BIT(n);
    bool dual = s->regs[g->irq_sense + 2] & GROUP_BIT(n);
    bool high = s->regs[g->irq_sense + 0] & GROUP_BIT(n);

    return !dual && level && !high;
}

static inline bool aspeed_gpio_irq_high(AspeedGPIOState *s,
        struct aspeed_gpio_group *g, int n)
{
    bool level = s->regs[g->irq_sense + 1] & GROUP_BIT(n);
    bool dual = s->regs[g->irq_sense + 2] & GROUP_BIT(n);
    bool high = s->regs[g->irq_sense + 0] & GROUP_BIT(n);

    return !dual && level && high;
}

static inline bool aspeed_gpio_irq_rising(AspeedGPIOState *s,
        struct aspeed_gpio_group *g, int n)
{
    bool level = s->regs[g->irq_sense + 1] & GROUP_BIT(n);
    bool dual = s->regs[g->irq_sense + 2] & GROUP_BIT(n);
    bool high = s->regs[g->irq_sense + 0] & GROUP_BIT(n);

    return !dual && !level && high;
}

static inline bool aspeed_gpio_irq_falling(AspeedGPIOState *s,
        struct aspeed_gpio_group *g, int n)
{
    bool level = s->regs[g->irq_sense + 1] & GROUP_BIT(n);
    bool dual = s->regs[g->irq_sense + 2] & GROUP_BIT(n);
    bool high = s->regs[g->irq_sense + 0] & GROUP_BIT(n);

    return !dual && !level && !high;
}

static inline bool aspeed_gpio_irq_both(AspeedGPIOState *s,
        struct aspeed_gpio_group *g, int n)
{
    return s->regs[g->irq_sense + 2] & GROUP_BIT(n);
}

static inline bool aspeed_gpio_is_input(AspeedGPIOState *s,
        struct aspeed_gpio_group *g, int n)
{
    return !(s->regs[g->dir] & GROUP_BIT(n));
}

typedef void aspeed_gpio_irq(void *opaque, int n, int level)
{
    struct aspeed_gpio_group *group;
    AspeedGPIOState *s = opaque;
    uint32_t old, new;

    if (n > s->nr_gpios) {
        /* FIXME: Might not be a guest error? */
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Invalid GPIO number: %d\n",
                      __func__, irq);
        return;
    }

    if (aspeed_gpio_output_only(n)) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Output-only GPIO: %d\n",
                      __func__, irq);
        return;
    }

    group = gpio_groups[TO_GROUP(n)];

    if (!aspeed_gpio_is_input(n)) {
        /* FIXME: Might not be a guest error? */
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Output-only GPIO: %d\n",
                      __func__, irq);
    }

    /* Grab current GPIO state for interrupt processing */
    old = s->regs[group->data] & GROUP_BIT(n);
    new = level ? GROUP_BIT(n) : 0;

    /* Set the state before processing interrupts */
    s->regs[group->data] &= ~GROUP_BIT(n);
    s->regs[group->data] |= new;

    /* Derive new interrupt state */
    if (aspeed_gpio_irq_enabled(s, group, n)) {
        bool interrupt = (
                   (aspeed_gpio_irq_low(s, group, n)     && !level)
                || (aspeed_gpio_irq_high(s, group, n)    && level)
                || (aspeed_gpio_irq_rising(s, group, n)  && (!old && new))
                || (aspeed_gpio_irq_falling(s, group, n) && (old && !new))
                || (aspeed_gpio_irq_both(s, group, n)    && (old != new))
            );

        /* Update interrupt state if necessary */
        if (interrupt) {
            s->regs[group->irq_status] |= GROUP_BIT(n);
            qemu_irq_raise(s->irq);
        }
    }
}

static const MemoryRegionOps aspeed_scu_ops = {
    .read = aspeed_gpio_read,
    .write = aspeed_gpio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid.min_access_size = 4,
    .valid.max_access_size = 4,
    .valid.unaligned = false,
};

static void aspeed_scu_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    AspeedGPIOState *s = ASPEED_GPIO(dev);

    if (!is_supported_silicon_rev(s->silicon_rev)) {
        error_setg(errp, "Unknown silicon revision: 0x%" PRIx32,
                s->silicon_rev);
        return;
    }

    /* TODO: Determine the number of GPIOs for the silicon revision */

    memory_region_init_io(&s->iomem, OBJECT(s), &aspeed_gpio_ops, s,
                          TYPE_ASPEED_GPIO, 0x1000);

    sysbus_init_mmio(sbd, &s->iomem);
    qdev_init_gpio_in(dev, aspeed_gpio_irq, ASPEED_GPIO_NR_GPIOS);
    sysbus_init_irq(sbd, &s->timers[i].irq);
}

static const VMStateDescription vmstate_aspeed_gpio = {
    .name = TYPE_ASPEED_GPIO,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(regs, AspeedSCUState, ASPEED_SCU_NR_REGS),
        VMSTATE_END_OF_LIST()
    }
};

static void aspeed_gpio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = aspeed_gpio_realize;
    dc->reset = aspeed_gpio_reset;
    dc->desc = "ASPEED GPIO Controllre";
    dc->vmsd = &vmstate_aspeed_gpio;
}

static const TypeInfo aspeed_gpio_info = {
    .name = TYPE_ASPEED_GPIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(AspeedGPIOState),
    .class_init = aspeed_gpio_class_init,
};

static void aspeed_gpio_register_types(void)
{
    type_register_static(&aspeed_gpio_info);
}

type_init(aspeed_gpio_register_types);

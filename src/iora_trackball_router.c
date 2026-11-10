#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <drivers/behavior.h>
#include <drivers/input_processor.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

#include "iora_trackball_router.h"

#define ROUTER DT_NODELABEL(iora_trackball_router)
#define AUTO_MOUSE DT_NODELABEL(iora_auto_mouse)
#define KEY_PROCESSOR DT_NODELABEL(iora_trackball_keys)
#define MOUSE_LAYER DT_PHA(AUTO_MOUSE, processor, param1)
#define SWAP_SETTING "iora/trackball_swapped"
#define LAYER_BIT(node, prop, idx) BIT(DT_PROP_BY_IDX(node, prop, idx))
#define SCROLL_LAYERS DT_FOREACH_PROP_ELEM_SEP(KEY_PROCESSOR, layers, LAYER_BIT, (|))
#define SCROLL_PROCESSOR_COUNT MAX(DT_PROP_LEN(ROUTER, auxiliary_scroll),                         \
                                   DT_PROP_LEN(ROUTER, primary_scroll))

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// Bit zero selects the handedness. Every toggle also invalidates old input work.
static atomic_t role_epoch;

uint32_t iora_trackball_role_epoch(void) { return (uint32_t)atomic_get(&role_epoch); }

#if IS_ENABLED(CONFIG_SETTINGS)
static int load_swap_setting(const char *name, size_t len, settings_read_cb read_cb, void *arg) {
    if (name) {
        return -ENOENT;
    }

    uint8_t swapped;
    if (len != sizeof(swapped)) {
        return -EINVAL;
    }
    ssize_t read = read_cb(arg, &swapped, sizeof(swapped));
    if (read < 0) {
        return read;
    }
    if (read != sizeof(swapped) || swapped > 1) {
        return -EINVAL;
    }
    if ((iora_trackball_role_epoch() & 1u) != swapped) {
        atomic_inc(&role_epoch);
        zmk_keymap_layer_deactivate(MOUSE_LAYER);
    }
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(iora_trackball_swap, SWAP_SETTING, NULL, load_swap_setting, NULL,
                               NULL);
#endif

struct trackball_report {
    uint32_t epoch;
    bool open;
    bool scroll;
    int16_t remainders[SCROLL_PROCESSOR_COUNT][2];
};

#define PROCESSOR_ENTRY(node, prop, idx)                                                         \
    {                                                                                           \
        .dev = DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node, prop, idx)),                                 \
        .param1 = DT_PHA_BY_IDX_OR(node, prop, idx, param1, 0),                                    \
        .param2 = DT_PHA_BY_IDX_OR(node, prop, idx, param2, 0),                                    \
        .track_remainders = DT_PROP(DT_PHANDLE_BY_IDX(node, prop, idx), track_remainders),         \
    }

static const struct zmk_input_processor_entry auxiliary_scroll[] = {
    DT_FOREACH_PROP_ELEM_SEP(ROUTER, auxiliary_scroll, PROCESSOR_ENTRY, (,))};
static const struct zmk_input_processor_entry primary_scroll[] = {
    DT_FOREACH_PROP_ELEM_SEP(ROUTER, primary_scroll, PROCESSOR_ENTRY, (,))};

static int route_scroll(struct input_event *event, struct zmk_input_processor_state *state,
                        struct trackball_report *report, bool primary) {
    const struct zmk_input_processor_entry *processors = primary ? primary_scroll : auxiliary_scroll;
    size_t count = primary ? ARRAY_SIZE(primary_scroll) : ARRAY_SIZE(auxiliary_scroll);
    size_t axis = event->code == INPUT_REL_Y;

    for (size_t i = 0; i < count; i++) {
        const struct zmk_input_processor_entry *processor = &processors[i];
        state->remainder = processor->track_remainders ? &report->remainders[i][axis] : NULL;
        int ret = zmk_input_processor_handle_event(processor->dev, event, processor->param1,
                                                   processor->param2, state);
        if (ret != ZMK_INPUT_PROC_CONTINUE) {
            return ret;
        }
    }
    return ZMK_INPUT_PROC_CONTINUE;
}

static int route_motion(const struct device *dev, struct input_event *event, uint32_t side,
                        uint32_t param2, struct zmk_input_processor_state *state) {
    static struct trackball_report reports[IORA_TRACKBALL_SIDE_COUNT];

    if (event->type != INPUT_EV_REL || (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }
    if (side >= ARRAY_SIZE(reports)) {
        return -EINVAL;
    }

    struct trackball_report *report = &reports[side];
    if (!report->open) {
        uint32_t epoch = iora_trackball_role_epoch();
        if (report->epoch != epoch) {
            memset(report->remainders, 0, sizeof(report->remainders));
            report->epoch = epoch;
        }
        uint32_t active = zmk_keymap_layer_state() | BIT(zmk_keymap_layer_default());
        report->scroll = (active & (SCROLL_LAYERS)) != 0;
    }
    // Keep both axes of an in-progress sensor report on the same route.
    report->open = !event->sync;
    bool primary = side != (report->epoch & 1u);
    const struct device *processor = primary ? DEVICE_DT_GET(AUTO_MOUSE)
                                             : DEVICE_DT_GET(KEY_PROCESSOR);
    int ret = zmk_input_processor_handle_event(processor, event, report->epoch, side, state);
    if (ret != ZMK_INPUT_PROC_CONTINUE || (primary && !report->scroll)) {
        return ret;
    }
    return route_scroll(event, state, report, primary);
}

static const struct zmk_input_processor_driver_api router_api = {.handle_event = route_motion};

DEVICE_DT_DEFINE(ROUTER, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                 &router_api);

static int swap_pressed(struct zmk_behavior_binding *binding,
                        struct zmk_behavior_binding_event event) {
    uint32_t epoch = (uint32_t)atomic_inc(&role_epoch) + 1u;
    zmk_keymap_layer_deactivate(MOUSE_LAYER);
#if IS_ENABLED(CONFIG_SETTINGS)
    const uint8_t swapped = epoch & 1u;
    int ret = settings_save_one(SWAP_SETTING, &swapped, sizeof(swapped));
    if (ret < 0) {
        LOG_ERR("Failed to save trackball roles: %d", ret);
    }
#endif
    return ZMK_BEHAVIOR_OPAQUE;
}

static int swap_released(struct zmk_behavior_binding *binding,
                         struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api swap_api = {
    .binding_pressed = swap_pressed,
    .binding_released = swap_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

BEHAVIOR_DT_DEFINE(DT_NODELABEL(trackball_swap), NULL, NULL, NULL, NULL, POST_KERNEL,
                   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &swap_api);

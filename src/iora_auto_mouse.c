#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>
#include <drivers/behavior.h>
#include <drivers/input_processor.h>
#include <zmk/behavior.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#define AUTO_MOUSE_NODE DT_NODELABEL(iora_auto_mouse)
#define MOUSE_LAYER DT_PHA(AUTO_MOUSE_NODE, processor, param1)
#define MOUSE_TIMEOUT_MS DT_PHA(AUTO_MOUSE_NODE, processor, param2)
#define SETTINGS_ROOT "iora"
#define DISABLED_SETTING "auto_mouse_disabled"

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// A missing setting leaves auto mouse enabled.
static atomic_t auto_mouse_disabled;

#if IS_ENABLED(CONFIG_SETTINGS)
static int load_auto_mouse_setting(const char *name, size_t len, settings_read_cb read_cb,
                                   void *cb_arg) {
    const char *next;
    if (!settings_name_steq(name, DISABLED_SETTING, &next) || next) {
        return -ENOENT;
    }

    uint8_t disabled;
    if (len != sizeof(disabled)) {
        return -EINVAL;
    }
    ssize_t read = read_cb(cb_arg, &disabled, sizeof(disabled));
    if (read < 0) {
        return read;
    }
    if (read != sizeof(disabled) || disabled > 1) {
        return -EINVAL;
    }

    atomic_set(&auto_mouse_disabled, disabled);
    if (disabled) {
        zmk_keymap_layer_deactivate(MOUSE_LAYER);
    }
    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(iora_auto_mouse_settings, SETTINGS_ROOT, NULL,
                               load_auto_mouse_setting, NULL, NULL);
#endif

static int process_motion(const struct device *dev, struct input_event *event, uint32_t param1,
                          uint32_t param2, struct zmk_input_processor_state *state) {
    if (atomic_get(&auto_mouse_disabled) || event->type != INPUT_EV_REL ||
        (event->code != INPUT_REL_X && event->code != INPUT_REL_Y) || event->value == 0) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    const struct device *temp_layer = DEVICE_DT_GET(DT_PHANDLE(AUTO_MOUSE_NODE, processor));
    return zmk_input_processor_handle_event(temp_layer, event, MOUSE_LAYER, MOUSE_TIMEOUT_MS, state);
}

static const struct zmk_input_processor_driver_api auto_mouse_processor_api = {
    .handle_event = process_motion,
};

DEVICE_DT_DEFINE(AUTO_MOUSE_NODE, NULL, NULL, NULL, NULL, POST_KERNEL,
                 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &auto_mouse_processor_api);

static int on_layer_state_changed(const zmk_event_t *event) {
    const struct zmk_layer_state_changed *change = as_zmk_layer_state_changed(event);

    if (!change->state) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    // Clear the prior mouse layer on entry so the new layer starts with its own keys.
    if ((change->layer >= 1 && change->layer <= 3) ||
        (change->layer == MOUSE_LAYER && atomic_get(&auto_mouse_disabled))) {
        zmk_keymap_layer_deactivate(MOUSE_LAYER);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(iora_auto_mouse, on_layer_state_changed);
ZMK_SUBSCRIPTION(iora_auto_mouse, zmk_layer_state_changed);

static int control_pressed(struct zmk_behavior_binding *binding,
                           struct zmk_behavior_binding_event event) {
    const struct device *device = zmk_behavior_get_binding(binding->behavior_dev);
    const struct zmk_behavior_binding *fallback = device->config;

    if (fallback) {
        if (event.layer == MOUSE_LAYER) {
            zmk_keymap_layer_deactivate(MOUSE_LAYER);
            // Record the underlying layer for both this press and the physical release.
            return zmk_keymap_position_state_changed(event.source, event.position, true,
                                                    event.timestamp);
        }
        return zmk_behavior_invoke_binding(fallback, event, true);
    }
    const uint8_t disabled = !atomic_xor(&auto_mouse_disabled, 1);
    if (disabled) {
        zmk_keymap_layer_deactivate(MOUSE_LAYER);
    }
#if IS_ENABLED(CONFIG_SETTINGS)
    int ret = settings_save_one(SETTINGS_ROOT "/" DISABLED_SETTING, &disabled, sizeof(disabled));
    if (ret < 0) {
        LOG_ERR("Failed to save auto mouse setting: %d", ret);
    }
#endif
    return ZMK_BEHAVIOR_OPAQUE;
}

static int control_released(struct zmk_behavior_binding *binding,
                            struct zmk_behavior_binding_event event) {
    const struct device *device = zmk_behavior_get_binding(binding->behavior_dev);
    const struct zmk_behavior_binding *fallback = device->config;

    return fallback ? zmk_behavior_invoke_binding(fallback, event, false) : ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api auto_mouse_control_api = {
    .binding_pressed = control_pressed,
    .binding_released = control_released,
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
    .get_parameter_metadata = zmk_behavior_get_empty_param_metadata,
#endif
};

#define DT_DRV_COMPAT askey_auto_mouse_control
#define FALLBACK_CONFIG(n)                                                                        \
    static const struct zmk_behavior_binding fallback_##n =                                        \
        ZMK_KEYMAP_EXTRACT_BINDING(0, DT_DRV_INST(n));
#define AUTO_MOUSE_CONTROL(n)                                                                    \
    BUILD_ASSERT(DT_INST_PROP_LEN_OR(n, bindings, 0) <= 1, "Expected at most one fallback binding"); \
    COND_CODE_1(DT_INST_NODE_HAS_PROP(n, bindings), (FALLBACK_CONFIG(n)), ())                         \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL,                                                   \
                            COND_CODE_1(DT_INST_NODE_HAS_PROP(n, bindings), (&fallback_##n), (NULL)), \
                            POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                     \
                            &auto_mouse_control_api);

DT_INST_FOREACH_STATUS_OKAY(AUTO_MOUSE_CONTROL)

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <drivers/input_processor.h>
#include <zmk/behavior.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>
#include <zmk/virtual_key_position.h>

#define TRACKBALL_KEYS DT_NODELABEL(iora_trackball_keys)
#define SETTINGS_LAYER DT_PROP(TRACKBALL_KEYS, settings_layer)
#define COUNTS_PER_TAP DT_PROP(TRACKBALL_KEYS, counts_per_tap)
#define REPEAT_INTERVAL_MS DT_PROP(TRACKBALL_KEYS, repeat_interval_ms)
#define LAYER_BIT(node, prop, idx) BIT(DT_PROP_BY_IDX(node, prop, idx))
#define KEY_MODE_LAYERS DT_FOREACH_PROP_ELEM_SEP(TRACKBALL_KEYS, layers, LAYER_BIT, (|))

BUILD_ASSERT(SETTINGS_LAYER < ZMK_KEYMAP_LAYERS_LEN);
BUILD_ASSERT(DT_PROP_LEN(TRACKBALL_KEYS, up_positions) == DT_PROP_LEN(TRACKBALL_KEYS, layers));
BUILD_ASSERT(DT_PROP_LEN(TRACKBALL_KEYS, down_positions) == DT_PROP_LEN(TRACKBALL_KEYS, layers));
BUILD_ASSERT(COUNTS_PER_TAP > 0 && COUNTS_PER_TAP <= INT32_MAX - 2048);
BUILD_ASSERT(REPEAT_INTERVAL_MS > 0);

static const uint8_t key_mode_layers[] = DT_PROP(TRACKBALL_KEYS, layers);
static const uint8_t up_positions[] = DT_PROP(TRACKBALL_KEYS, up_positions);
static const uint8_t down_positions[] = DT_PROP(TRACKBALL_KEYS, down_positions);

struct trackball_tap {
    atomic_val_t generation;
    uint32_t timestamp;
    uint8_t position;
    uint8_t listener;
    uint8_t mode;
};

K_MSGQ_DEFINE(trackball_taps, sizeof(struct trackball_tap), 1, 4);
static atomic_t layer_generation;

static int active_key_mode(void) {
    uint32_t active = (zmk_keymap_layer_state() | BIT(zmk_keymap_layer_default())) & (KEY_MODE_LAYERS);
    if (!active) {
        return -1;
    }

    // Follow Studio's layer order when multiple key-mode layers are active.
    for (int index = ZMK_KEYMAP_LAYERS_LEN - 1; index >= 0; index--) {
        zmk_keymap_layer_id_t layer = zmk_keymap_layer_index_to_id(index);
        if (layer == ZMK_KEYMAP_LAYER_ID_INVAL || !(active & BIT(layer))) {
            continue;
        }
        for (size_t mode = 0; mode < ARRAY_SIZE(key_mode_layers); mode++) {
            if (layer == key_mode_layers[mode]) {
                return mode;
            }
        }
    }
    return -1;
}

static int on_layer_changed(const zmk_event_t *event) {
    const struct zmk_layer_state_changed *change = as_zmk_layer_state_changed(event);
    if (BIT(change->layer) & (KEY_MODE_LAYERS)) {
        atomic_inc(&layer_generation);
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(iora_trackball_keys, on_layer_changed);
ZMK_SUBSCRIPTION(iora_trackball_keys, zmk_layer_state_changed);

static void send_trackball_taps(void *unused1, void *unused2, void *unused3) {
    struct trackball_tap tap;
    atomic_val_t last_generation = -1;
    int last_mode = -1;
    uint32_t last_sent = 0;

    while (k_msgq_get(&trackball_taps, &tap, K_FOREVER) == 0) {
        uint32_t now = k_uptime_get_32();
        if (tap.generation != atomic_get(&layer_generation) || active_key_mode() != tap.mode ||
            now - tap.timestamp >= REPEAT_INTERVAL_MS ||
            (tap.generation == last_generation && tap.mode == last_mode &&
             now - last_sent < REPEAT_INTERVAL_MS)) {
            continue;
        }

        const struct zmk_behavior_binding *slot =
            zmk_keymap_get_layer_binding_at_idx(SETTINGS_LAYER, tap.position);
        if (!slot) {
            continue;
        }

        // Use the same binding for press and release even if Studio edits the slot mid-tap.
        struct zmk_behavior_binding binding = *slot;
        if (!binding.behavior_dev ||
            strcmp(binding.behavior_dev, DEVICE_DT_NAME(DT_NODELABEL(kp))) != 0) {
            continue;
        }
        struct zmk_behavior_binding_event event = {
            .layer = SETTINGS_LAYER,
            .position = ZMK_VIRTUAL_KEY_POSITION_BEHAVIOR_INPUT_PROCESSOR(tap.listener, 0),
            .timestamp = k_uptime_get(),
            .source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
        };
        last_generation = tap.generation;
        last_mode = tap.mode;
        last_sent = now;
        zmk_behavior_invoke_binding(&binding, event, true);
        zmk_behavior_invoke_binding(&binding, event, false);
    }
}

K_THREAD_DEFINE(iora_trackball_worker, 2048, send_trackball_taps, NULL, NULL, NULL,
                K_PRIO_PREEMPT(1), 0, 0);

static int process_motion(const struct device *dev, struct input_event *event, uint32_t param1,
                          uint32_t param2, struct zmk_input_processor_state *state) {
    static atomic_val_t report_generation;
    static uint32_t last_tap;
    static int32_t distance;
    static bool sent_tap;
    static bool report_open;
    static int report_mode = -1;

    if (event->type != INPUT_EV_REL || (event->code != INPUT_REL_X && event->code != INPUT_REL_Y)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    // A layer change between X and Y applies to the next sensor report.
    if (!report_open) {
        atomic_val_t generation = atomic_get(&layer_generation);
        int mode = active_key_mode();
        if (generation != report_generation || mode != report_mode) {
            distance = 0;
            sent_tap = false;
        }
        report_generation = generation;
        report_mode = mode;
    }
    report_open = !event->sync;

    if (report_mode < 0) {
        return ZMK_INPUT_PROC_CONTINUE;
    }
    if (event->code == INPUT_REL_Y) {
        // PMW3610 reports signed 12-bit motion. Keep at most one step while rate limited.
        int32_t movement = CLAMP(event->value, -2048, 2047);
        int32_t total = distance + movement;
        uint32_t now = k_uptime_get_32();
        if ((total <= -COUNTS_PER_TAP || total >= COUNTS_PER_TAP) &&
            (!sent_tap || now - last_tap >= REPEAT_INTERVAL_MS)) {
            struct trackball_tap tap = {
                .generation = report_generation,
                .timestamp = now,
                .position = total < 0 ? up_positions[report_mode] : down_positions[report_mode],
                .listener = state->input_device_index,
                .mode = report_mode,
            };
            if (k_msgq_put(&trackball_taps, &tap, K_NO_WAIT) == 0) {
                last_tap = now;
                sent_tap = true;
            }
            distance = total % COUNTS_PER_TAP;
        } else {
            distance = CLAMP(total, -COUNTS_PER_TAP, COUNTS_PER_TAP);
        }
    }
    return ZMK_INPUT_PROC_STOP;
}

static const struct zmk_input_processor_driver_api trackball_keys_api = {
    .handle_event = process_motion,
};

DEVICE_DT_DEFINE(TRACKBALL_KEYS, NULL, NULL, NULL, NULL, POST_KERNEL,
                 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &trackball_keys_api);

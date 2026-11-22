/*
 * Copyright (c) 2026 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_ble_profile_transform

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zmk/endpoints.h>

#include <dt-bindings/zmk/input_transform.h>

struct ibpt_config {
    size_t x_codes_size;
    size_t y_codes_size;
    size_t profiles_size;
    uint8_t type;

    const uint16_t *x_codes;
    const uint16_t *y_codes;
    const int32_t *profiles;
};

static int code_idx(uint16_t code, const uint16_t *list, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (list[i] == code) {
            return i;
        }
    }

    return -ENODEV;
}

static bool is_selected_ble_profile(const struct ibpt_config *cfg) {
    struct zmk_endpoint_instance endpoint = zmk_endpoints_selected();

    if (endpoint.transport != ZMK_TRANSPORT_BLE) {
        return false;
    }

    for (size_t i = 0; i < cfg->profiles_size; i++) {
        if (cfg->profiles[i] == endpoint.ble.profile_index) {
            return true;
        }
    }

    return false;
}

static int ibpt_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                             uint32_t param2, struct zmk_input_processor_state *state) {
    const struct ibpt_config *cfg = dev->config;

    if (event->type != cfg->type || !is_selected_ble_profile(cfg)) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if ((param1 & INPUT_TRANSFORM_X_INVERT &&
         code_idx(event->code, cfg->x_codes, cfg->x_codes_size) >= 0) ||
        (param1 & INPUT_TRANSFORM_Y_INVERT &&
         code_idx(event->code, cfg->y_codes, cfg->y_codes_size) >= 0)) {
        event->value = -event->value;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api ibpt_driver_api = {
    .handle_event = ibpt_handle_event,
};

static int ibpt_init(const struct device *dev) { return 0; }

#define IBPT_INST(n)                                                                               \
    static const uint16_t ibpt_x_codes_##n[] = DT_INST_PROP(n, x_codes);                           \
    static const uint16_t ibpt_y_codes_##n[] = DT_INST_PROP(n, y_codes);                           \
    static const int32_t ibpt_profiles_##n[] = DT_INST_PROP(n, profiles);                          \
    static const struct ibpt_config ibpt_config_##n = {                                            \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .x_codes_size = DT_INST_PROP_LEN(n, x_codes),                                              \
        .y_codes_size = DT_INST_PROP_LEN(n, y_codes),                                              \
        .profiles_size = DT_INST_PROP_LEN(n, profiles),                                            \
        .x_codes = ibpt_x_codes_##n,                                                               \
        .y_codes = ibpt_y_codes_##n,                                                               \
        .profiles = ibpt_profiles_##n,                                                             \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, &ibpt_init, NULL, NULL, &ibpt_config_##n, POST_KERNEL,                 \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ibpt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IBPT_INST)

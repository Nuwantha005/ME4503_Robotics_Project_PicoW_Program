#include "json_protocol.h"
#include "shared_state.h"
#include "websocket.h"
#include "led_test.h"
#include "cJSON.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// ============================================================================
// json_protocol.cpp
//
// Converts between JSON strings and RobotCommand/RobotConfig/RobotState
// structures using the cJSON library.
// ============================================================================

void json_protocol_process(const char* json_str) {
    cJSON* root = cJSON_Parse(json_str);
    if (!root) {
        printf("[json] ERROR: Failed to parse JSON: %s\n", json_str);
        return;
    }

    cJSON* type_item = cJSON_GetObjectItem(root, "type");
    if (!type_item || !cJSON_IsString(type_item)) {
        printf("[json] ERROR: Missing or invalid 'type' field in JSON\n");
        cJSON_Delete(root);
        return;
    }

    const char* type = type_item->valuestring;

    if (strcmp(type, "cmd") == 0) {
        // High frequency command
        RobotCommand cmd = read_command_snapshot();

        cJSON* x_item = cJSON_GetObjectItem(root, "x");
        if (x_item && cJSON_IsNumber(x_item)) cmd.x = (float)x_item->valuedouble;

        cJSON* y_item = cJSON_GetObjectItem(root, "y");
        if (y_item && cJSON_IsNumber(y_item)) cmd.y = (float)y_item->valuedouble;

        cJSON* rot_item = cJSON_GetObjectItem(root, "rot");
        if (rot_item && cJSON_IsNumber(rot_item)) cmd.rot = (float)rot_item->valuedouble;

        cJSON* obs_item = cJSON_GetObjectItem(root, "obstacle_enabled");
        if (obs_item && cJSON_IsBool(obs_item)) cmd.obstacle_enabled = cJSON_IsTrue(obs_item);

        cJSON* mode_item = cJSON_GetObjectItem(root, "mode");
        if (mode_item && cJSON_IsString(mode_item)) {
            if (strcmp(mode_item->valuestring, "manual") == 0) {
                cmd.mode = Mode::MANUAL;
            } else if (strcmp(mode_item->valuestring, "line_follow") == 0) {
                cmd.mode = Mode::LINE_FOLLOW;
            }
        }

        cJSON* scheme_item = cJSON_GetObjectItem(root, "scheme");
        if (scheme_item && cJSON_IsString(scheme_item)) {
            if (strcmp(scheme_item->valuestring, "buttons") == 0) {
                cmd.control_scheme = ControlScheme::BUTTONS;
            } else if (strcmp(scheme_item->valuestring, "joystick") == 0) {
                cmd.control_scheme = ControlScheme::JOYSTICK;
            } else if (strcmp(scheme_item->valuestring, "tilt") == 0) {
                cmd.control_scheme = ControlScheme::TILT;
            } else if (strcmp(scheme_item->valuestring, "field_centric") == 0) {
                cmd.control_scheme = ControlScheme::FIELD_CENTRIC;
            }
        }

        cmd.last_update_ms = to_ms_since_boot(get_absolute_time());
        write_command(cmd);

    } else if (strcmp(type, "config") == 0) {
        // Config change
        RobotConfig cfg = read_config_snapshot();

        cJSON* speed = cJSON_GetObjectItem(root, "max_speed_mps");
        if (speed && cJSON_IsNumber(speed)) cfg.max_speed_mps = (float)speed->valuedouble;

        cJSON* accel = cJSON_GetObjectItem(root, "max_accel_mps2");
        if (accel && cJSON_IsNumber(accel)) cfg.max_accel_mps2 = (float)accel->valuedouble;

        cJSON* stop_dist = cJSON_GetObjectItem(root, "obstacle_stop_distance_mm");
        if (stop_dist && cJSON_IsNumber(stop_dist)) cfg.obstacle_stop_distance_mm = (float)stop_dist->valuedouble;

        cJSON* kp = cJSON_GetObjectItem(root, "pid_kp");
        if (kp && cJSON_IsNumber(kp)) cfg.pid_kp = (float)kp->valuedouble;

        cJSON* ki = cJSON_GetObjectItem(root, "pid_ki");
        if (ki && cJSON_IsNumber(ki)) cfg.pid_ki = (float)ki->valuedouble;

        cJSON* kd = cJSON_GetObjectItem(root, "pid_kd");
        if (kd && cJSON_IsNumber(kd)) cfg.pid_kd = (float)kd->valuedouble;

        cJSON* r = cJSON_GetObjectItem(root, "wheel_radius_m");
        if (r && cJSON_IsNumber(r)) cfg.wheel_radius_m = (float)r->valuedouble;

        cJSON* lx = cJSON_GetObjectItem(root, "wheelbase_lx_m");
        if (lx && cJSON_IsNumber(lx)) cfg.wheelbase_lx_m = (float)lx->valuedouble;

        cJSON* ly = cJSON_GetObjectItem(root, "wheelbase_ly_m");
        if (ly && cJSON_IsNumber(ly)) cfg.wheelbase_ly_m = (float)ly->valuedouble;

        cJSON* enc = cJSON_GetObjectItem(root, "encoder_counts_per_rev");
        if (enc && cJSON_IsNumber(enc)) cfg.encoder_counts_per_rev = (float)enc->valuedouble;

        cJSON* gear = cJSON_GetObjectItem(root, "motor_gear_ratio");
        if (gear && cJSON_IsNumber(gear)) cfg.motor_gear_ratio = (float)gear->valuedouble;

        write_config(cfg);

    } else if (strcmp(type, "test") == 0) {
        cJSON* target_item = cJSON_GetObjectItem(root, "target");
        cJSON* action_item = cJSON_GetObjectItem(root, "action");

        if (target_item && cJSON_IsString(target_item) && action_item && cJSON_IsString(action_item)) {
            const char* target = target_item->valuestring;
            const char* action = action_item->valuestring;

            if (strcmp(target, "led") == 0) {
                if (strcmp(action, "toggle") == 0) {
                    // Call the isolated test module directly on Core 0
                    bool new_state = led_test_toggle();

                    // Update local state in RobotState so telemetry reflects it
                    RobotState state = read_state_snapshot();
                    state.led_on = new_state;
                    write_state_snapshot(state);

                    // Send test_result JSON back to the client immediately
                    char resp_buf[128];
                    if (json_protocol_build_test_result("led", new_state ? "on" : "off", resp_buf, sizeof(resp_buf))) {
                        ws_send_text(resp_buf, strlen(resp_buf));
                    }
                }
            }
        }
    } else {
        printf("[json] Unknown message type: %s\n", type);
    }

    cJSON_Delete(root);
}

bool json_protocol_build_telemetry(const RobotState& state, char* out_buf, size_t max_len) {
    cJSON* root = cJSON_CreateObject();
    if (!root) return false;

    cJSON_AddStringToObject(root, "type", "telemetry");
    cJSON_AddNumberToObject(root, "uptime_ms", state.uptime_ms);
    cJSON_AddBoolToObject(root, "led_on", state.led_on);

    // Write default/zeroed values for remaining telemetry fields to conform to Phase 1 plan
    cJSON* speeds = cJSON_CreateFloatArray(&state.wheel_speed_fl, 4);
    if (speeds) {
        cJSON_AddItemToObject(root, "wheel_speeds", speeds);
    }

    cJSON* odom = cJSON_CreateObject();
    if (odom) {
        cJSON_AddNumberToObject(odom, "x", state.odom_x_m);
        cJSON_AddNumberToObject(odom, "y", state.odom_y_m);
        cJSON_AddNumberToObject(odom, "theta", state.odom_theta_rad);
        cJSON_AddItemToObject(root, "odom", odom);
    }

    int ir_int[8];
    for (int i = 0; i < 8; i++) ir_int[i] = state.ir_decay_us[i];
    cJSON* ir = cJSON_CreateIntArray(ir_int, 8);
    if (ir) {
        cJSON_AddItemToObject(root, "ir", ir);
    }
    cJSON_AddNumberToObject(root, "line_position", state.line_position);

    int tof_int[4];
    for (int i = 0; i < 4; i++) tof_int[i] = state.tof_mm[i];
    cJSON* tof = cJSON_CreateIntArray(tof_int, 4);
    if (tof) {
        cJSON_AddItemToObject(root, "tof", tof);
    }
    cJSON_AddNumberToObject(root, "battery_v", state.battery_voltage);

    char* rendered = cJSON_PrintUnformatted(root);
    bool success = false;
    if (rendered) {
        if (strlen(rendered) < max_len) {
            strcpy(out_buf, rendered);
            success = true;
        }
        cJSON_free(rendered);
    }

    cJSON_Delete(root);
    return success;
}

bool json_protocol_build_test_result(const char* target, const char* result_state, char* out_buf, size_t max_len) {
    cJSON* root = cJSON_CreateObject();
    if (!root) return false;

    cJSON_AddStringToObject(root, "type", "test_result");
    cJSON_AddStringToObject(root, "target", target);
    cJSON_AddStringToObject(root, "state", result_state);

    char* rendered = cJSON_PrintUnformatted(root);
    bool success = false;
    if (rendered) {
        if (strlen(rendered) < max_len) {
            strcpy(out_buf, rendered);
            success = true;
        }
        cJSON_free(rendered);
    }

    cJSON_Delete(root);
    return success;
}

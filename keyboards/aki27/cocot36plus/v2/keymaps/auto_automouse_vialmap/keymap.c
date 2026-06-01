/*
Copyright 2022 aki27

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Generated from Vial export: 20260511_テンキー数字への対応_ゲームレイヤに矢印追加.vil

#include QMK_KEYBOARD_H
#include <stdio.h>
#include "quantum.h"
#include "twpair_on_jis.h"

#define MIDI_INITIAL_VELOCITY 117

#define MS_BTN1 KC_MS_BTN1
#define MS_BTN2 KC_MS_BTN2
#define MS_BTN3 KC_MS_BTN3


// ----------------------------------------------------------------
// Mouse Jiggler
// JIGGLE_TOG でトグル。一定間隔でカーソルを微小移動しスリープを防止する。
// ジグラー中はオートマウスレイヤーを無効化し、LED（アンダーグロウ）を白く点滅。
// ----------------------------------------------------------------
#define JIGGLE_INTERVAL 60000  // カーソル移動間隔 (ms)。変更可。
#define JIGGLE_AMPLITUDE 20    // 移動量 (pixel)。4px以下はWindows加速で無視されTeams離席になる
#define JIGGLE_BLINK_MS  500   // LED点滅間隔 (ms)

static bool    jiggle_active     = false;
static uint16_t jiggle_move_timer  = 0;  // 次の移動までのタイマー
static uint16_t jiggle_blink_timer = 0;  // LED点滅タイマー
static bool    jiggle_led_on     = false;
static bool    jiggle_pending    = false; // pointing_device_task_userへの移動フラグ
static int8_t  jiggle_dir        = 1;    // 移動方向 (+1 / -1)


// ----------------------------------------------------------------
// Tap Dance
// TD(0): シングルタップ → KC_Q  /  ダブルタップ → KC_ESC
// ※ VIAL_ENABLE時はVialがtap_dance_actions[]を動的管理するため
//   ここでは配列定義を行わない。動作はVial GUIまたは.vilロードで設定する。
// ----------------------------------------------------------------
enum {
    TD_Q_ESC = 0,  // keymaps内のTD(TD_Q_ESC)参照用インデックス
};


// ----------------------------------------------------------------
// Combo
// ※ VIAL_ENABLE時はVialがkey_combos[]を動的管理するため
//   ここでは配列定義を行わない。動作はVial GUIまたは.vilロードで設定する。
//
// .vilに定義されているコンボ（参照用）:
//   Q+W         → Tab   (layer0/1: TD, layer2: KC_Q)
//   Q+/         → Tab   (layer0/1)
//   Q+W+E       → S-Tab (layer0/1: TD, layer2: KC_Q → Esc)
//   Q+/+U       → S-Tab (layer0/1)
// ----------------------------------------------------------------


// ----------------------------------------------------------------
// Auto mouse state machine
// ----------------------------------------------------------------
enum click_state {
    NONE = 0,
    WAITING,    // トラックボール入力を検知、有効化待機中
    CLICKABLE,  // マウスレイヤー有効、クリック受付中
    CLICKING,   // クリック押下中
    SCROLLING   // スクロール中
};

enum click_state state;
uint16_t click_timer;

uint16_t to_clickable_time = 100;  // WAITING → CLICKABLE 移行時間 (ms)
uint16_t to_reset_time     = 800;  // CLICKABLE → NONE タイムアウト (ms)

const uint16_t click_layer = 4;

int16_t scroll_v_mouse_interval_counter;
int16_t scroll_h_mouse_interval_counter;
int16_t scroll_v_threshold = 50;
int16_t scroll_h_threshold = 50;

int16_t after_click_lock_movement = 0;
int16_t mouse_record_threshold    = 30;
int16_t mouse_move_count_ratio    = 5;


// ----------------------------------------------------------------
// Keymap
// ----------------------------------------------------------------

// .vilのUSERコード対応表:
//   USER00 = CPI_SW     USER01 = SCRL_SW    USER02 = ROT_R15   USER03 = ROT_L15
//   USER04 = SCRL_MO    USER05 = SCRL_TO    USER06 = SCRL_IN   USER07 = AM_TOG
//   USER08 = SET_US_MODE                    USER09 = SET_JIS_MODE

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

    // Layer 0: ベース (mod-tap + LT, MHEN/HENK)
    [0] = LAYOUT(
        TD(TD_Q_ESC),  KC_W,          KC_E,    KC_R,    KC_T,              KC_Y,           KC_U,            KC_I,    KC_O,    KC_P,
        LCTL_T(KC_A),  KC_S,          KC_D,    KC_F,    KC_G,              KC_H,           KC_J,            KC_K,    KC_L,    RCTL_T(KC_MINS),
        LSFT_T(KC_Z),  LGUI_T(KC_X),  KC_C,    KC_V,    KC_B,              KC_N,           KC_M,            KC_COMM, KC_DOT,  RSFT_T(KC_SLSH),
                       LALT_T(KC_INT5), LT(5, KC_SPC), LSFT_T(KC_APP),    LT(6, KC_ENT),  LT(5, KC_BSPC), RALT_T(KC_INT4)
    ),

    // Layer 1: 代替ベース (Dvorak系配列)
    [1] = LAYOUT(
        TD(TD_Q_ESC),  KC_SLSH,       KC_U,    KC_Y,    KC_COMM,           KC_J,           KC_D,            KC_H,    KC_G,    KC_W,
        LCTL_T(KC_I),  KC_O,          KC_E,    KC_A,    KC_DOT,            KC_K,           KC_T,            KC_N,    KC_S,    RCTL_T(KC_R),
        LSFT_T(KC_Z),  LGUI_T(KC_X),  KC_C,    KC_V,    KC_MINS,           KC_M,           KC_L,            KC_F,    KC_B,    RSFT_T(KC_P),
                       LALT_T(KC_INT5), LT(5, KC_SPC), LSFT_T(KC_APP),    LT(6, KC_ENT),  LT(5, KC_BSPC), RALT_T(KC_INT4)
    ),

    // Layer 2: 標準 QWERTY (シンプル配列、右手にカーソルキー)
    [2] = LAYOUT(
        KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,              KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,
        KC_A,    KC_S,    KC_D,    KC_F,    KC_G,              KC_H,    KC_J,    KC_K,    KC_UP,   KC_SCLN,
        KC_LSFT, KC_X,    KC_C,    KC_V,    KC_B,              KC_N,    KC_M,    KC_LEFT, KC_DOWN, KC_RGHT,
                          KC_LCTL, KC_SPC,  KC_LSFT,           KC_ENT,  MO(5),   KC_BSPC
    ),

    // Layer 3: 未使用
    [3] = LAYOUT(
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,           XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,           XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
        XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,           XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
                          XXXXXXX, XXXXXXX, XXXXXXX,           XXXXXXX, XXXXXXX, XXXXXXX
    ),

    // Layer 4: オートマウス (クリック・スクロール・ブラウザ操作)
    [4] = LAYOUT(
        KC_ESC,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,           KC_BTN1, KC_BTN2, KC_BTN3, KC_TRNS, KC_TRNS,
        KC_LCTL, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,           SCRL_MO, SCRL_MO, KC_TRNS, KC_TRNS, KC_RCTL,
        KC_LSFT, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,           KC_WBAK, KC_WFWD, KC_TRNS, KC_TRNS, KC_RSFT,
                          LALT_T(KC_F13), SCRL_MO, KC_BTN1,    KC_ESC,  LT(5, KC_BSPC), RALT_T(KC_F14)
    ),

    // Layer 5: 記号・カーソル・マウスボタン (LT5ホールド)
    [5] = LAYOUT(
        LSFT(KC_1), LSFT(KC_2), LSFT(KC_3), LSFT(KC_4), LSFT(KC_5),    LSFT(KC_6), LSFT(KC_7), LSFT(KC_8), LSFT(KC_9), LSFT(KC_0),
        KC_LCTL,    KC_GRV,     KC_MINS,    KC_EQL,     KC_BTN1,        KC_LEFT,    KC_DOWN,    KC_UP,      KC_RGHT,    KC_SCLN,
        KC_LSFT,    KC_BSLS,    KC_LBRC,    KC_RBRC,    KC_BTN2,        KC_HOME,    KC_PGDN,    KC_PGUP,    KC_END,     KC_QUOT,
                                KC_INS,     MO(7),      KC_LSFT,        KC_ESC,     MO(7),      KC_DEL
    ),

    // Layer 6: テンキー・ファンクション (LT6ホールド、NumLock対応)
    [6] = LAYOUT(
        KC_SLSH,        LSFT(KC_8), KC_MINS,    LSFT(KC_EQL), KC_EQL,         LSFT(KC_1), KC_KP_7, KC_KP_8, KC_KP_9, KC_NUM_LOCK,
        LCTL_T(KC_F1),  KC_F2,      KC_F3,      KC_F4,        KC_F5,          KC_DOT,     KC_KP_4, KC_KP_5, KC_KP_6, RCTL_T(KC_F11),
        LSFT_T(KC_F6),  KC_F7,      KC_F8,      KC_F9,        KC_F10,         KC_KP_0,    KC_KP_1, KC_KP_2, KC_KP_3, RSFT_T(KC_F12),
                                    LALT_T(KC_F13), XXXXXXX,  XXXXXXX,        XXXXXXX,    XXXXXXX, RALT_T(KC_F14)
    ),

    // Layer 7: システム設定 (MO(7)で遷移)
    [7] = LAYOUT(
        DF(0),       ROT_L15,  ROT_R15,  SCRL_SW,  CPI_SW,            XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX, DF(1),
        DF(2),       RGB_HUI,  RGB_VAI,  RGB_MOD,  RGB_TOG,           XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX, JIGGLE_TOG,
        SET_US_MODE, RGB_HUD,  RGB_VAD,  RGB_RMOD, QK_BOOT,           XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX, SET_JIS_MODE,
                               XXXXXXX,  XXXXXXX,  XXXXXXX,           XXXXXXX,    XXXXXXX, XXXXXXX
    ),
};


// ----------------------------------------------------------------
// Encoder map (.vilのencoder_layoutから生成)
// Layer 2: MIDI音量  /  Layer 5,6: PC音量  /  他: PageUp/Down
// ----------------------------------------------------------------
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(KC_PGUP, KC_PGDN) },
    [1] = { ENCODER_CCW_CW(KC_PGUP, KC_PGDN) },
    [2] = { ENCODER_CCW_CW(MI_VELD, MI_VELU) },  // MIDI volume
    [3] = { ENCODER_CCW_CW(XXXXXXX, XXXXXXX) },
    [4] = { ENCODER_CCW_CW(KC_PGUP, KC_PGDN) },
    [5] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },  // PC volume
    [6] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },  // PC volume
    [7] = { ENCODER_CCW_CW(XXXXXXX, XXXXXXX) },
};
#endif


// ----------------------------------------------------------------
// Mouse record: これらのキーはオートマウスレイヤーを解除しない
// ----------------------------------------------------------------
bool is_mouse_record_kb(uint16_t keycode, keyrecord_t* record) {
    switch (keycode) {
        case KC_LCTL:
        case KC_LSFT:
        case SCRL_MO:
            return true;
        default:
            return false;
    }
    return is_mouse_record_user(keycode, record);
}


// ----------------------------------------------------------------
// Key processing
// ----------------------------------------------------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {

    switch (keycode) {
        // マウスボタン処理 (クリック状態を管理)
        case KC_MS_BTN1:
        case KC_MS_BTN2:
        case KC_MS_BTN3:
        case KC_MS_BTN4:
        case KC_MS_BTN5:
        {
            report_mouse_t currentReport = pointing_device_get_report();
            uint8_t btn = 1 << (keycode - KC_MS_BTN1);
            if (record->event.pressed) {
                currentReport.buttons |= btn;
                state = CLICKING;
                after_click_lock_movement = 30;
            } else {
                currentReport.buttons &= ~btn;
                enable_click_layer();
            }
            pointing_device_set_report(currentReport);
            pointing_device_send();
            return false;
        }

        // スクロールモード (SCRL_MO押下中)
        case SCRL_MO:
            if (record->event.pressed) {
                state = SCROLLING;
            } else {
                enable_click_layer();  // リリース時にクリックレイヤーを再有効化
            }
            return false;

        // マウスジグラー トグル
        case JIGGLE_TOG:
            if (record->event.pressed) {
                jiggle_active = !jiggle_active;
                if (jiggle_active) {
                    // ジグラー開始: タイマーリセット・オートマウス無効化
                    jiggle_move_timer  = timer_read();
                    jiggle_blink_timer = timer_read();
                    jiggle_led_on      = true;
                    jiggle_dir         = 1;
                    disable_click_layer();
                    set_auto_mouse_enable(false);
                } else {
                    // ジグラー停止: オートマウス設定を復元
                    jiggle_led_on = false;
                    if (cocot_config.auto_mouse) {
                        set_auto_mouse_enable(true);
                    }
                }
            }
            return false;
    }

    // JIS変換 (マウス系処理の後に適用)
    if (cocot_config.jis) {
        return twpair_on_jis(keycode, record);
    }

    return true;
}


// ----------------------------------------------------------------
// RGB: レイヤーに応じてアンダーグロウ色を変更
// ----------------------------------------------------------------
#ifdef RGB_MATRIX_ENABLE

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    HSV hsv;

    if (jiggle_active) {
        // ジグラーON: 白色で点滅
        uint8_t brightness = jiggle_led_on ? rgblight_get_val() : 0;
        hsv = (HSV){0, 0, brightness};
    } else {
        int is_layer = get_highest_layer(layer_state | default_layer_state);
        hsv = (HSV){0, 255, rgblight_get_val()};
        if      (is_layer == 1) { hsv.h = 128; }  // CYAN
        else if (is_layer == 2) { hsv.h = 85;  }  // GREEN
        else if (is_layer == 3) { hsv.h = 43;  }  // YELLOW
        else if (is_layer == 4) { hsv.h = 0;   }  // RED
        else if (is_layer == 5) { hsv.h = 191; }  // PURPLE
        else if (is_layer == 6) { hsv.h = 64;  }  // CHARTREUSE
        else if (is_layer == 7) { hsv.h = 224; }
        else                    { hsv.h = 11;  }  // CORAL (layer 0)
    }

    RGB rgb = hsv_to_rgb(hsv);
    for (uint8_t i = led_min; i <= led_max; i++) {
        if (HAS_FLAGS(g_led_config.flags[i], 0x02)) {
            rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
        }
    }
    return false;
}

#endif


// ----------------------------------------------------------------
// Auto mouse click layer functions
// ----------------------------------------------------------------
void enable_click_layer(void) {
    layer_on(click_layer);
    click_timer = timer_read();
    state = CLICKABLE;
}

void disable_click_layer(void) {
    state = NONE;
    layer_off(click_layer);
    scroll_v_mouse_interval_counter = 0;
    scroll_h_mouse_interval_counter = 0;
}

int16_t my_abs(int16_t num) {
    if (num < 0) { num = -num; }
    return num;
}

int16_t mmouse_move_y_sign(int16_t num) {
    if (num < 0) { return -1; }
    return 1;
}

bool is_clickable_mode(void) {
    return state == CLICKABLE || state == CLICKING || state == SCROLLING;
}


// ----------------------------------------------------------------
// Pointing device: ステートマシンによるクリック・スクロール制御
// ----------------------------------------------------------------
report_mouse_t pointing_device_task_user(report_mouse_t mouse_report) {

    // ジグラー: housekeeping_task_userからのフラグを受けてX移動を注入
    if (jiggle_pending) {
        jiggle_pending   = false;
        mouse_report.x  += JIGGLE_AMPLITUDE * jiggle_dir;
        mouse_report.y  += JIGGLE_AMPLITUDE * jiggle_dir;
        jiggle_dir       = -jiggle_dir;
    }

    int16_t current_x = mouse_report.x;
    int16_t current_y = mouse_report.y;
    int16_t current_h = 0;
    int16_t current_v = 0;

    if (current_x != 0 || current_y != 0) {

        switch (state) {
            case CLICKABLE:
                click_timer = timer_read();
                break;

            case CLICKING:
                after_click_lock_movement -= my_abs(current_x) + my_abs(current_y);
                if (after_click_lock_movement > 0) {
                    current_x = 0;
                    current_y = 0;
                }
                break;

            case SCROLLING:
            {
                int8_t rep_v = 0;
                int8_t rep_h = 0;

                // 垂直方向を2倍の感度で優先
                if (my_abs(current_y) * 2 > my_abs(current_x)) {
                    scroll_v_mouse_interval_counter += current_y;
                    while (my_abs(scroll_v_mouse_interval_counter) > scroll_v_threshold) {
                        if (scroll_v_mouse_interval_counter < 0) {
                            scroll_v_mouse_interval_counter += scroll_v_threshold;
                            rep_v += scroll_v_threshold;
                        } else {
                            scroll_v_mouse_interval_counter -= scroll_v_threshold;
                            rep_v -= scroll_v_threshold;
                        }
                    }
                } else {
                    scroll_h_mouse_interval_counter += current_x;
                    while (my_abs(scroll_h_mouse_interval_counter) > scroll_h_threshold) {
                        if (scroll_h_mouse_interval_counter < 0) {
                            scroll_h_mouse_interval_counter += scroll_h_threshold;
                            rep_h += scroll_h_threshold;
                        } else {
                            scroll_h_mouse_interval_counter -= scroll_h_threshold;
                            rep_h -= scroll_h_threshold;
                        }
                    }
                }

                current_h = -rep_h / scroll_h_threshold;
                current_v =  rep_v / scroll_v_threshold;
                current_x = 0;
                current_y = 0;
            }
                break;

            case WAITING:
                if (timer_elapsed(click_timer) > to_clickable_time) {
                    enable_click_layer();
                }
                break;

            default:
                click_timer = timer_read();
                state = WAITING;
        }

    } else {

        switch (state) {
            case CLICKING:
            case SCROLLING:
                break;

            case CLICKABLE:
                if (timer_elapsed(click_timer) > to_reset_time) {
                    disable_click_layer();
                }
                break;

            case WAITING:
                if (timer_elapsed(click_timer) > 50) {
                    state = NONE;
                }
                break;

            default:
                state = NONE;
        }
    }

    mouse_report.x = current_x;
    mouse_report.y = current_y;
    mouse_report.h = current_h;
    mouse_report.v = current_v;

    return mouse_report;
}


// ----------------------------------------------------------------
// Housekeeping: ジグラータイマー管理（メインループ毎に呼ばれる）
// ----------------------------------------------------------------
void housekeeping_task_user(void) {
    if (!jiggle_active) return;

    // カーソル移動フラグをセット
    if (timer_elapsed(jiggle_move_timer) > JIGGLE_INTERVAL) {
        jiggle_move_timer = timer_read();
        jiggle_pending    = true;
    }

    // LED点滅トグル
    if (timer_elapsed(jiggle_blink_timer) > JIGGLE_BLINK_MS) {
        jiggle_blink_timer = timer_read();
        jiggle_led_on      = !jiggle_led_on;
    }
}

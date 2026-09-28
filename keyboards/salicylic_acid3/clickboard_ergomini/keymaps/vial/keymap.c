/* Copyright 2025 Salicylic_acid3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * aki27/cocot36plus から移植した機能:
 *
 * 1) JIS/US 配列切り替え
 *   元実装: keyboards/aki27/cocot36plus/twpair_on_jis.c (eswai 氏の twpair_on_jis)
 *   US キーキャップの刻印どおりの文字・記号を、OS が JIS 配列認識のまま出力する。
 *   例: Shift+2 → @
 *   切り替えは SET_US_MODE / SET_JIS_MODE の2つのユーザキーコードで行い、
 *   状態は EEPROM の user 領域 (eeconfig_update_user) に永続化する。
 *
 * 2) マウスジグラー (JIGGLE_TOG)
 *   一定間隔でカーソルを微小移動しスリープ／離席判定を防止する。
 *
 *   ★ 移植上の変更点（重要）
 *   cocot36plus 版は pointing_device_task_user()（トラックボール）で
 *   mouse_report に移動量を注入していた。しかし ergomini は
 *   ポインティングデバイス非搭載のため pointing_device_task_user() が
 *   呼ばれず、同じ実装では動かない。
 *   そこで QMK 標準のマウス移動キーコード (QK_MOUSE_CURSOR_*) を
 *   tap_code16() で送信する方式に置き換えた。
 *     tap_code16 → register_code → IS_MOUSE_KEYCODE 分岐
 *       → register_mouse → mousekey_on → mousekey_send
 *       → host_mouse_send            (実際にカーソルが動く)
 *   1回のキーコードで MOUSEKEY_MOVE_DELTA（既定 8px）動くため、
 *   JIGGLE_TAPS 回送って移動量を確保する。
 *   （MOUSEKEY_MOVE_DELTA を config.h で上書きする案は却下。
 *     ergomini の .vil は Layer4 で KC_MS_* を Vial 割当済みのため、
 *     グローバル値を変えると通常のマウスキー操作感に影響する。）
 *   なお ergomini は RGB 非搭載のため、cocot36plus 版の「LED点滅」と
 *   「オートマウスレイヤー無効化」は移植対象外。
 */

#include QMK_KEYBOARD_H
#include "twpair_on_jis.h"

// ----------------------------------------------------------------
// ユーザキーコード
//   Vial の User タブに表示するには keymaps/vial/vial.json の
//   customKeycodes と「順番・個数」を一致させること。
//   customKeycodes[0] -> QK_KB_0, customKeycodes[1] -> QK_KB_1 ...
//   ※ 末尾に追加するので既存割当 (SET_US/JIS) は壊れない。
// ----------------------------------------------------------------
enum custom_keycodes {
    SET_US_MODE = QK_KB_0,
    SET_JIS_MODE,
    JIGGLE_TOG,
};

// EEPROM (user 領域 4byte) に保存する設定。
//   bit0: JIS モード有効フラグ
// Vial のダイナミックキーマップは別領域のため干渉しない。
#define USER_CONFIG_JIS_BIT 0x00000001

static bool jis_mode = false;

// ----------------------------------------------------------------
// マウスジグラー
// ----------------------------------------------------------------
#define JIGGLE_INTERVAL 60000  // カーソル移動間隔 (ms)
#define JIGGLE_TAPS     3      // 1回の移動で送るマウスキーコード数
                               // (MOUSEKEY_MOVE_DELTA=8px × 3 ≒ 24px。
                               //  4px 以下は Windows の加速で無視され
                               //  離席判定されるため、余裕を持たせる)
#define JIGGLE_IDLE_MS  10000  // 直近この時間 (ms) に入力があればジグルをスキップ。
                               // ジグラーの目的は「離席時にPCを起こしておく」ことなので、
                               // 入力中 (= 離席していない) は動かす必要がない。
                               // ドラッグ中の選択範囲拡大やゲーム中の視点ズレを防ぐ。
                               // ジグル自身はマウスキー経由で matrix activity を
                               // 更新しないため、自分で自分をブロックしない。

static bool     jiggle_active     = false;
// 【型に注意】自前タイマーは uint32_t / timer_read32 / timer_elapsed32 で統一する。
// timer_read()/timer_elapsed() は 16bit で 65535ms（65.5秒）ごとに一周するため、
// スキップでタイマーを据え置きにすると位相がずれ、入力後に離席した際の初回発火が
// 最大で約60秒遅れる（ジグラー本来の用途＝離席時にPCを起こす、が損なわれる）。
// 実機例: cocot36plus では同型の欠陥が『1回目は発火するが2回目以降しない』として顕在化。
static uint32_t jiggle_move_timer = 0;  // 次の移動までのタイマー
static int8_t   jiggle_dir        = 1;  // 移動方向 (+1=右 / -1=左)

// ----------------------------------------------------------------
// ジグラー状態の「カーソル合図」
//   ergomini は RGB・オーディオ非搭載で、ユーザーに伝えられる出力が
//   カーソル移動だけ。そこで JIGGLE_TOG を押した瞬間に大きく往復させ、
//   押すだけで ON/OFF が体感できるようにする。
//     ON  -> 右へ振って戻す（往復2回）
//     OFF -> 左へ振って戻す（往復1回）
//   往復なので正味の移動量はゼロ → 作業位置がずれない。
//   送信は register_mouse() の default 節で 1 タップ = 即時送信（MK_3_SPEED 無効）。
//   ※ ホスト側のポインタ加速を前提に、往復の「振れ幅」で ON/OFF を区別する。
// ----------------------------------------------------------------
#define JIGGLE_SIGNAL_TAPS 12  // 1ストロークあたりのタップ数 (12×8px≒96px)

static void jiggle_signal(bool turning_on) {
    uint16_t fwd = turning_on ? QK_MOUSE_CURSOR_RIGHT : QK_MOUSE_CURSOR_LEFT;
    uint16_t back = turning_on ? QK_MOUSE_CURSOR_LEFT : QK_MOUSE_CURSOR_RIGHT;

    // ON: 右へ大きく → 戻す → もう一度右へ → 戻す（往復2回）
    // OFF: 左へ大きく → 戻す（往復1回）
    uint8_t cycles = turning_on ? 2 : 1;
    for (uint8_t c = 0; c < cycles; c++) {
        for (uint8_t i = 0; i < JIGGLE_SIGNAL_TAPS; i++) tap_code16(fwd);
        for (uint8_t i = 0; i < JIGGLE_SIGNAL_TAPS; i++) tap_code16(back);
    }
}

// ----------------------------------------------------------------
// キーマップ
// ----------------------------------------------------------------
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
         KC_TAB,    KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,        KC_Y,    KC_U,    KC_I,    KC_O,    KC_P, KC_BSPC,
        KC_LCTL,    KC_A,    KC_S,    KC_D,    KC_F,    KC_G,        KC_H,    KC_J,    KC_K,    KC_L, KC_SCLN,  KC_ENT,
        KC_LSFT,    KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,        KC_N,    KC_M, KC_COMM,  KC_DOT, KC_SLSH,   MO(1),
        KC_LEFT,   KC_UP,KC_RIGHT,  KC_SPC,  KC_SPC,  KC_SPC,      KC_SPC,  KC_SPC,  KC_SPC, KC_LEFT,   KC_UP,KC_RIGHT,
                 KC_DOWN,                                                                             KC_DOWN
    ),
    [1] = LAYOUT(
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
                 _______,                                                                             _______
    ),
    [2] = LAYOUT(
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
                 _______,                                                                             _______
    ),
    [3] = LAYOUT(
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
                 _______,                                                                             _______
    ),
    [4] = LAYOUT(
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
                 _______,                                                                             _______
    ),
    [5] = LAYOUT(
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
                 _______,                                                                             _______
    ),
    [6] = LAYOUT(
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
        _______, _______, _______, _______, _______, _______,     _______, _______, _______, _______, _______, _______,
                 _______,                                                                             _______
    )
};

// ----------------------------------------------------------------
// JIS/US モード判定
// ----------------------------------------------------------------
bool is_jis_mode(void) {
    return jis_mode;
}

// ----------------------------------------------------------------
// 起動時に EEPROM から読み出す
// ----------------------------------------------------------------
void keyboard_post_init_user(void) {
    jis_mode = (eeconfig_read_user() & USER_CONFIG_JIS_BIT) != 0;
}

// ----------------------------------------------------------------
// EEPROM リセット時は US モードに戻す
// ----------------------------------------------------------------
void eeconfig_init_user(void) {
    eeconfig_update_user(0);
    jis_mode = false;
}

// ----------------------------------------------------------------
// キー処理
// ----------------------------------------------------------------
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case SET_US_MODE:
            if (record->event.pressed) {
                jis_mode = false;
                eeconfig_update_user(0);
            }
            return false;

        case SET_JIS_MODE:
            if (record->event.pressed) {
                jis_mode = true;
                eeconfig_update_user(USER_CONFIG_JIS_BIT);
            }
            return false;

        // マウスジグラー トグル
        case JIGGLE_TOG:
            if (record->event.pressed) {
                jiggle_active = !jiggle_active;
                if (jiggle_active) {
                    jiggle_move_timer = timer_read32();
                    jiggle_dir        = 1;
                }
                // 押した瞬間にカーソルで ON/OFF を合図する
                // （ergomini は LED 非搭載のため、これが唯一のフィードバック）
                jiggle_signal(jiggle_active);
            }
            return false;
    }

    // JIS モード時のみ US→JIS 変換を適用
    if (jis_mode) {
        return twpair_on_jis(keycode, record);
    }

    return true;
}

// ----------------------------------------------------------------
// Housekeeping: ジグラータイマー管理（メインループ毎に呼ばれる）
//   移動はマウス移動キーコードの送信で行う
// ----------------------------------------------------------------
void housekeeping_task_user(void) {
    if (!jiggle_active) return;

    if (timer_elapsed32(jiggle_move_timer) > JIGGLE_INTERVAL) {
        // 直近 JIGGLE_IDLE_MS に入力があればスキップ（離席していない）。
        // ここでタイマーを進めないので、入力が止まって JIGGLE_IDLE_MS 経過した
        // 時点で即座に移動する（最大 JIGGLE_INTERVAL 待たされない）。
        if (last_input_activity_elapsed() < JIGGLE_IDLE_MS) return;

        jiggle_move_timer = timer_read32();

        // 左右交互にマウス移動キーコードを送って往復させる
        uint16_t kc = (jiggle_dir > 0) ? QK_MOUSE_CURSOR_RIGHT : QK_MOUSE_CURSOR_LEFT;
        for (uint8_t i = 0; i < JIGGLE_TAPS; i++) {
            tap_code16(kc);
        }
        jiggle_dir = -jiggle_dir;
    }
}

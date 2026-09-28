# cocot36plus v2 / auto_automouse_vialmap キーマップ 設計資料

> 旧 `my_auto_mouse/DESIGN.md`（2026-04-22作成・試作版）を、現行キーマップ
> `auto_automouse_vialmap` の実態に合わせて改訂したもの。
> ハードウェア仕様（§2, §3, §8, §9）はキーマップ非依存なので原本から継承。
> 改訂日: 2026-09-28

## 1. 概要

| 項目 | 内容 |
|------|------|
| キーボード | cocot36plus v2 |
| キーマップ名 | auto_automouse_vialmap |
| MCU | RP2040 |
| ファームウェア | vial-qmk (aki27kbd fork) |
| USB VID/PID | 0x1727 / 0x0014 |
| デバイスバージョン | 2.0.0 |

### 主な特徴

- **自前オートマウスレイヤー**：QMKの `POINTING_DEVICE_AUTO_MOUSE_ENABLE` に加え、独自ステートマシンでクリック・スクロールの状態を細かく制御
- **JIS/US配列切替**：`twpair_on_jis` によりソフトウェア的にUS/JIS配列を切り替え可能。設定は **EEPROMに永続化**される
- **マウスジグラー**：一定間隔でカーソルを微小移動しPCスリープを防止。**入力中はスキップ**する
- **MIDI対応**：`MIDI_ENABLE` + `MIDI_ADVANCED` による高度なMIDI入力
- **Vial対応**：GUIからリアルタイムでキーマップ・エンコーダ割当を変更可能

---

## 2. ハードウェア構成

### マトリクス

| 項目 | 設定 |
|------|------|
| 行数 / 列数 | 4行 × 10列 |
| ダイオード方向 | COL2ROW |
| 行ピン | GP18, GP17, GP16, GP15 |
| 列ピン | GP24, GP23, GP22, GP21, GP20, GP13, GP12, GP11, GP10, GP9 |

### ロータリーエンコーダ

| 項目 | 設定 |
|------|------|
| 実装方式 | `ENCODER_MAP_ENABLE`（キーマップテーブル方式） |

### LED

| 項目 | 設定 |
|------|------|
| ドライバ | WS2812（GP0） |
| 総LED数 | 45灯 |
| アンダーグロウ | 9灯（flags: 2） |
| キーバックライト | 36灯（flags: 4） |
| 最大輝度 | 50 |

### トラックボール

| 項目 | 設定 |
|------|------|
| センサー | PMW3360 |
| CPI選択肢 | 200 / 400 / 800 / 1600 / 3200 |
| CPI初期値 | 800（index: 3） |

---

## 3. ビルド設定

### rules.mk

```makefile
VIA_ENABLE         = yes
VIAL_ENABLE        = yes
ENCODER_MAP_ENABLE = yes
MIDI_ENABLE        = yes   # MIDI controls
TAP_DANCE_ENABLE   = yes   # Tap Dance: VialがEEPROMで動的管理
COMBO_ENABLE       = yes   # Combo: VialがEEPROMで動的管理
SRC += keyboards/aki27/cocot36plus/twpair_on_jis.c   # JIS/US切替
```

### config.h

```c
// Vial必須設定
#define VIAL_KEYBOARD_UID {0xC8, 0x4D, 0xC8, 0xD5, 0xD8, 0x28, 0x66, 0x16}
#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {1, 9}

#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DEFAULT_LAYER  4      // オートマウスレイヤー番号
#define AUTO_MOUSE_TIME           650    // オートマウスレイヤー維持時間 (ms)
#define AUTO_MOUSE_DELAY          200    // 有効化までのディレイ (ms)
#define AUTO_MOUSE_DEBOUNCE       25     // デバウンス時間 (ms)

#define MIDI_ADVANCED                    // 高度なMIDI機能を有効化

#define TAPPING_TERM 200                 // .vilのtapping_term(ID=7)に合わせた値
```

---

## 4. レイヤー設計

### キーマップ一覧

```
レイヤー0  ベース（mod-tap + LT、MHEN/HENK）
レイヤー1  代替ベース（Dvorak系配列）
レイヤー2  標準QWERTY（シンプル配列、右手にカーソルキー）
レイヤー3  未使用
レイヤー4  オートマウス（クリック・スクロール）
レイヤー5  記号・ナビゲーション
レイヤー6  テンキー・ファンクション
レイヤー7  設定（レイヤー切替・RGB・JIS/US・ジグラー）
```

### レイヤー0：ベース

```
┌───────┬───┬───┬───┬───┐   ┌───┬───┬───┬───┬───────────┐
│TD(Q/ESC)│ W │ E │ R │ T │   │ Y │ U │ I │ O │ P         │
├───────┼───┼───┼───┼───┤   ├───┼───┼───┼───┼───────────┤
│Ctrl/A │ S │ D │ F │ G │   │ H │ J │ K │ L │ Ctrl/-    │
├───────┼───┼───┼───┼───┤   ├───┼───┼───┼───┼───────────┤
│Shift/Z│GUI/X│ C │ V │ B │   │ N │ M │ , │ . │ Shift(/  )│
└───────┴───┴───┼───┼───┤   ├───┼───┼───┴───┴───────────┘
                │Alt│LT5 Spc│Shift/App│   │LT6 Ent│LT5 Bsp│Alt(INT4)
                └───┴───┴───┘   └───┴───┴───┘
```

- `TD(TD_Q_ESC)`: シングルタップ → `KC_Q` / ダブルタップ → `KC_ESC`（Vialで動的管理）
- mod-tap（`LCTL_T` など）と `LT()` を多用した実運用配列

### レイヤー5：記号・ナビゲーション

```
┌───┬───┬───┬───┬───┐   ┌───┬───┬───┬───┬───┐
│ ! │ @ │ # │ $ │ % │   │ ^ │ & │ * │ ( │ ) │
├───┼───┼───┼───┼───┤   ├───┼───┼───┼───┼───┤
│Ctrl│ ` │ - │ = │MB1│   │ ← │ ↓ │ ↑ │ → │ ; │
├───┼───┼───┼───┼───┤   ├───┼───┼───┼───┼───┤
│Shft│ \ │ [ │ ] │MB2│   │Home│PgDn│PgUp│End│ ' │
└───┴───┴───┼───┼───┤   ├───┼───┼───┴───┴───┘
            │Ins│MO(7)│Shft│   │Esc│MO(7)│Del│
            └───┴───┴───┘   └───┴───┴───┘
```

### レイヤー6：テンキー・ファンクション

```
┌───┬───┬───┬───┬───┐   ┌───┬───┬───┬───┬────────┐
│ / │ * │ - │ + │ = │   │ ! │ 7 │ 8 │ 9 │NumLock │
├───┼───┼───┼───┼───┤   ├───┼───┼───┼───┼────────┤
│F1 │F2 │F3 │F4 │F5 │   │ . │ 4 │ 5 │ 6 │F11     │
├───┼───┼───┼───┼───┤   ├───┼───┼───┼───┼────────┤
│F6 │F7 │F8 │F9 │F10│   │ 0 │ 1 │ 2 │ 3 │F12     │
└───┴───┴───┼───┼───┤   ├───┼───┼───┴───┴────────┘
            │F13│    │    │   │    │    │F14
            └───┴───┴───┘   └───┴───┴───┘
```

### レイヤー4：オートマウス

```
┌───────┬───┬───┬───┬───┐   ┌───┬───┬───┬───┬───┐
│ ESC   │   │   │   │   │   │MB1│MB2│MB3│   │   │
├───────┼───┼───┼───┼───┤   ├───┼───┼───┼───┼───┤
│ Ctrl  │   │   │   │   │   │SCL│SCL│   │   │Ctrl│
├───────┼───┼───┼───┼───┤   ├───┼───┼───┼───┼───┤
│ Shift │   │   │   │   │   │WbK│WfW│   │   │Shft│
└───────┴───┴───┼───┼───┤   ├───┼───┼───┴───┴───┘
                │F13│SCL│MB1│   │ESC│LT5 Bsp│F14│
                └───┴───┴───┘   └───┴───┴───┘
MB1=左クリック  MB2=右クリック  MB3=中クリック  SCL=スクロールモード(SCRL_MO)
```

### レイヤー7：設定

| キー | 機能 |
|------|------|
| `DF(0)` / `DF(1)` / `DF(2)` | ベースレイヤー切替 |
| `ROT_L15` / `ROT_R15` | トラックボール回転角 -15° / +15° |
| `SCRL_SW` / `CPI_SW` | スクロール係数 / CPI 切替 |
| `RGB_HUI/HUD/VAI/VAD/MOD/RMOD/TOG` | RGB制御 |
| `SET_US_MODE` / `SET_JIS_MODE` | **JIS/US配列切替（EEPROM永続化）** |
| `JIGGLE_TOG` | **マウスジグラー ON/OFF** |
| `QK_BOOT` | ブートローダー起動 |

---

## 5. ロータリーエンコーダ設計

| レイヤー | 反時計回り (CCW) | 時計回り (CW) |
|---------|----------------|--------------|
| 0, 1, 4 | PageUp | PageDown |
| 2 | `MI_VELD` | `MI_VELU`（MIDI音量） |
| 3, 7 | 未割当 | 未割当 |
| 5, 6 | `KC_VOLD` | `KC_VOLU`（PC音量） |

---

## 6. オートマウス ステートマシン

### 状態遷移図

```
                   トラックボール入力
       ┌──────────────────────────────────────────────────┐
       ▼                                                  │
┌────────────┐  100ms経過  ┌─────────────┐              │
│  WAITING   │────────────▶│  CLICKABLE  │──────────────┘
└────────────┘             └─────────────┘
       │                    │    │    │
       │ 50ms無入力          │    │    │ 800ms無入力
       ▼                    │    │    ▼
┌────────────┐              │    │  ┌──────┐
│    NONE    │◀─────────────┘    └─▶│ NONE │
└────────────┘  クリック/スクロール終了 └──────┘
                    後にCLICKABLEへ

CLICKABLE + クリック押下 ──▶ CLICKING
CLICKING  + クリックリリース ▶ CLICKABLE

CLICKABLE + SCRL_MO押下  ──▶ SCROLLING
SCROLLING + SCRL_MO リリース ▶ CLICKABLE
```

### 状態定義

| 状態 | 説明 |
|------|------|
| `NONE` | 通常状態。マウスレイヤー無効 |
| `WAITING` | トラックボール入力を検知。100ms待機中 |
| `CLICKABLE` | マウスレイヤー有効。クリック入力受付中 |
| `CLICKING` | クリックボタン押下中。誤移動ロック作動 |
| `SCROLLING` | SCRL_MO押下によるスクロール入力中 |

### タイミングパラメータ

| パラメータ | 値 | 説明 |
|-----------|-----|------|
| `to_clickable_time` | 100 ms | WAITING → CLICKABLE 移行時間 |
| `to_reset_time` | 800 ms | CLICKABLE → NONE タイムアウト |
| `after_click_lock_movement` | 30 単位 | クリック後の誤移動防止量 |

---

## 7. スクロール処理設計

スクロールモード（`SCROLLING` 状態）では、`pointing_device_task_user` 内でX/Y移動量を閾値カウンタ方式によりスクロール量に変換する。

### 方向判定ロジック

```
abs(current_y) * 2 > abs(current_x)
  → 垂直スクロール優先（縦に2倍の感度）
それ以外
  → 水平スクロール
```

### スクロール閾値

| パラメータ | 値 | 説明 |
|-----------|-----|------|
| `scroll_v_threshold` | 50 | 垂直スクロール実行閾値 |
| `scroll_h_threshold` | 50 | 水平スクロール実行閾値 |

### 変換式

```c
current_h = -rep_h / scroll_h_threshold;  // 水平（正負反転）
current_v =  rep_v / scroll_v_threshold;  // 垂直
```

---

## 8. トラックボール処理設計（cocot36plus.c）

`pointing_device_task_kb` にて、センサー生値に以下の処理を順次適用する。

```
センサー生値 (x, y)
  │
  ▼
[1] 回転補正
    rad = angle_array[rotation_angle] × (π/180) × -1
    rotated_x = -(x·cos(rad) - y·sin(rad))
    rotated_y =   x·sin(rad) + y·cos(rad)
  │
  ▼
[2] EWMAスムージング（係数 0.7）
    smoothed = prev × 0.7 + rotated × 0.3
  │
  ▼
[3] ダイナミックマルチプライヤー
    magnitude = sqrt(sx² + sy²)
    multiplier = clamp(1.0 + magnitude/10.0,  0.5, 3.0)
    smoothed × sensitivity_multiplier(1.5) × dynamic_multiplier
  │
  ├─── スクロールモード → h/v スクロール変換（scrl_div ビットシフト方式）
  │
  └─── 通常モード → アキュムレータ方式で x/y に反映
                    x_accumulator += smoothed_x × sensitivity(0.5)
                    mouse_report.x = (int8_t)x_accumulator（1以上で出力）
```

### 回転角テーブル

```
{ -90, -75, -60, -45, -30, -15, 0, 15, 30, 45, 60, 75, 90 }
初期値インデックス: 3 → -45°
```

---

## 9. RGB LEDインジケーター

アクティブレイヤーに応じてアンダーグロウ（flags=0x02）の色を変更する。
**ジグラーON時は白色で点滅**（`JIGGLE_BLINK_MS` 間隔）。

| レイヤー | 色 | Hue値 |
|---------|-----|-------|
| 0（ベース） | CORAL | 11 |
| 1 | CYAN | 128 |
| 2 | GREEN | 85 |
| 3 | YELLOW | 43 |
| 4（マウス） | RED | 0 |
| 5 | PURPLE | 191 |
| 6 | CHARTREUSE | 64 |
| 7 | — | 224 |
| ジグラーON | WHITE（点滅） | — |

---

## 10. JIS/US配列切替

`twpair_on_jis.c` をソースに追加し、`process_record_kb` 内で `cocot_config.jis` フラグにより処理を分岐する。

```c
// cocot36plus.c / process_record_kb
if (cocot_config.jis) {
    return twpair_on_jis(keycode, record);
}
```

| キーコード | 動作 |
|-----------|------|
| `SET_US_MODE` (QK_KB_8) | jisフラグ → false（EEPROM保存） |
| `SET_JIS_MODE` (QK_KB_9) | jisフラグ → true（EEPROM保存） |

---

## 11. EEPROM永続化項目

`cocot_config_t`（uint64_t raw）に以下を格納し、`eeconfig_update_kb` で保存する。

> ⚠️ **重要**: `eeconfig_update_kb()` の引数は **`uint32_t`（4バイト）**。
> `offset 4` 以降のフィールドは **EEPROMに保存されない**。
> 永続化したいフラグは **offset 3 以内にビットで同居**させること。
> （旧実装は `jis` を offset 10 に置いていたため保存されなかった）

| フィールド | オフセット | 型 | 初期値 | 説明 |
|-----------|-----------|-----|-------|------|
| `cpi_idx` | 0 | uint8_t | 3（800cpi） | CPIインデックス |
| `scrl_div` | 1 | uint8_t | 4 | スクロール分割係数インデックス |
| `rotation_angle` | 2 | uint8_t | 3（-45°） | 回転角インデックス |
| `auto_mouse` | 3 (bit0) | bool:1 | true | オートマウス有効フラグ |
| `jis` | 3 (bit1) | bool:1 | false | JIS配列モードフラグ |
| `scrl_inv` | 3 (bit2〜) | bool | true | スクロール反転フラグ ※保存対象外 |
| `scrl_mode` | — | bool | false | スクロールモード状態 ※保存対象外 |
| `last_mouse` | 8 | report_mouse_t | — | 前回マウスレポート ※保存対象外 |

---

## 12. カスタムキーコード一覧

`.vil` の USERコード（USER00〜USER10）と `vial.json` の `customKeycodes` は順序対応。

| キーコード | QK_KB番号 | .vil USER | 機能 |
|-----------|----------|-----------|------|
| `CPI_SW` | QK_KB_0 | USER00 | CPIを順次切替（200→400→800→1600→3200） |
| `SCRL_SW` | QK_KB_1 | USER01 | スクロール分割係数を順次切替 |
| `ROT_R15` | QK_KB_2 | USER02 | センサー回転角を+15°（時計回り） |
| `ROT_L15` | QK_KB_3 | USER03 | センサー回転角を-15°（反時計回り） |
| `SCRL_MO` | QK_KB_4 | USER04 | スクロールモード（押している間のみ） |
| `SCRL_TO` | QK_KB_5 | USER05 | スクロールモード（トグル） |
| `SCRL_IN` | QK_KB_6 | USER06 | スクロール方向反転切替 |
| `AM_TOG` | QK_KB_7 | USER07 | QMK標準オートマウスON/OFF |
| `SET_US_MODE` | QK_KB_8 | USER08 | US配列モードに設定 |
| `SET_JIS_MODE` | QK_KB_9 | USER09 | JIS配列モードに設定 |
| `JIGGLE_TOG` | QK_KB_10 | USER10 | マウスジグラー ON/OFF |

---

## 13. レイヤー遷移とオートマウス制御

`layer_state_set_kb`（cocot36plus.c）によりレイヤー変更時に以下を制御する。

| レイヤー | スクロールモード | オートマウス |
|---------|---------------|------------|
| 1〜2 | ON | 無効（競合防止） |
| 3〜7 | OFF | 変更なし |
| 0（default） | OFF | `cocot_config.auto_mouse` に従う |

---

## 14. マウスジグラー

一定間隔でカーソルを微小移動し、PCのスリープ／Teams離席判定を防ぐ。

### パラメータ

| パラメータ | 値 | 説明 |
|-----------|-----|------|
| `JIGGLE_INTERVAL` | 60000 ms | カーソル移動間隔 |
| `JIGGLE_AMPLITUDE` | 20 px | 移動量（4px以下はWindows加速で無視される） |
| `JIGGLE_BLINK_MS` | 500 ms | LED点滅間隔 |
| `JIGGLE_IDLE_MS` | 10000 ms | 直近この時間内に入力があればスキップ |

### 実装方式

cocot36plus は**トラックボール経由**で注入する（ergomini のキーコード方式とは異なる）。

```
housekeeping_task_user()          ← 毎ループ呼ばれる
  └─ 間隔経過 & 入力なし判定
       └─ jiggle_pending = true

pointing_device_task_user()       ← トラックボールレポート処理時
  ├─ [先頭] 本物のトラックボール入力を jiggle_idle_timer に記録（注入前！）
  └─ jiggle_pending なら mouse_report.x += AMPLITUDE * dir
```

### 入力中スキップ（自己ブロック対策）

**問題**: ジグル自身の移動が `pointing_device_task()` の戻り値 true を招き、
`keyboard.c` の `last_pointing_device_activity_trigger()` 経由で
`last_input_modification_time` を更新してしまう（＝自分の移動を「入力」と誤認）。

**対策（方式B）**: スキップ判定を2系統にする。

| 判定対象 | 使用する値 | 自己ブロック |
|---------|-----------|------------|
| キー入力 | `last_matrix_activity_elapsed()` | 構造的に不可能（ジグルは matrix を汚さない） |
| トラックボール | `jiggle_idle_timer`（注入**前**に記録した値） | 構造的に不可能（注入後の値を見ない） |

```c
if (last_matrix_activity_elapsed() < JIGGLE_IDLE_MS ||
    timer_elapsed(jiggle_idle_timer) < JIGGLE_IDLE_MS) {
    return;   // スキップ。タイマーは進めない（入力停止後10秒で即再開）
}
```

- スキップ時はタイマーをリセットしない → 入力が止まれば即座にジグル再開
- トラックボール操作中はPC側のスリープタイマーもリセットされるため、ジグルは不要
- 操作中にジグルが発火するとカーソルがぶれるため、スキップが望ましい

---

## 15. 既知の注意点

- **`v2/keymaps/my_auto_mouse/`** は本キーマップの**試作版**だった（2026-04-21作成）。
  2026-09-28 に削除。keymap.c 以下のバックアップは `D:\keyboard_etc\backup_20260421\`
  ほか計4箇所に存在し、DESIGN.md 原本も同 backup 内に `DESIGN.md.orig_20260928` として保存済み。
- **`eeconfig_update_kb()` は4バイト制約**（§11参照）。フラグ追加時は offset 3 以内に置くこと。
- Vial画面のキー配置が実機と一致しないのは**上流（Salicylic氏）の仕様**。修正対象外。

---

*原本作成日: 2026-04-22（my_auto_mouse 用）*
*改訂日: 2026-09-28（auto_automouse_vialmap 用に改訂・ジグラー§14追加・EEPROM制約追記）*

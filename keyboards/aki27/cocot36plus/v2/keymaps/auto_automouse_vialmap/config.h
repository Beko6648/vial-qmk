// Vial必須設定
// UID: .vilファイルのuid(1614022428223622600)をリトルエンディアン8バイト16進変換したもの
// vialキーマップと同一キーボードのため同じUIDを使用
#define VIAL_KEYBOARD_UID {0xC8, 0x4D, 0xC8, 0xD5, 0xD8, 0x28, 0x66, 0x16}
// アンロックコンボ: row0/col1(W) + row0/col9(P) を同時押し
#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {1, 9}

#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DEFAULT_LAYER 4
#define AUTO_MOUSE_TIME 30000   // マウスレイヤー自動離脱までの時間(ms)。最後のトラックボール操作から30秒。
                                // ※keymap.c の to_reset_time と揃えること（短い方が勝つ）
#define AUTO_MOUSE_DELAY 200
#define AUTO_MOUSE_DEBOUNCE 25

#define MIDI_ADVANCED

// ----------------------------------------------------------------
// ロータリーエンコーダの接点バウンス対策（2026-09-29）
// PER601（PER60シリーズ）は機械式接点で Contact Bounce 5ms max。
// 旧ポーリング実装にデバウンスが無く、「時々逆方向」と誤判定していた。
// 1デテント=2遷移、上限60RPMで遷移間隔16.7msなので 6ms で安全。
// 有効化には quantum/encoder.c 側の対応実装が必要（ENCODER_DEBOUNCE_MS）。
// ----------------------------------------------------------------
#define ENCODER_DEBOUNCE_MS 6

// .vilのtapping_term(ID=7)に合わせた値。EEPROMリセット後もこの値が使われる
#define TAPPING_TERM 200

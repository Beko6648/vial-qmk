// Vial必須設定
// UID: .vilファイルのuid(1614022428223622600)をリトルエンディアン8バイト16進変換したもの
// vialキーマップと同一キーボードのため同じUIDを使用
#define VIAL_KEYBOARD_UID {0xC8, 0x4D, 0xC8, 0xD5, 0xD8, 0x28, 0x66, 0x16}
// アンロックコンボ: row0/col1(W) + row0/col9(P) を同時押し
#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {1, 9}

#define POINTING_DEVICE_AUTO_MOUSE_ENABLE
#define AUTO_MOUSE_DEFAULT_LAYER 4
#define AUTO_MOUSE_TIME 650
#define AUTO_MOUSE_DELAY 200
#define AUTO_MOUSE_DEBOUNCE 25

#define MIDI_ADVANCED

// .vilのtapping_term(ID=7)に合わせた値。EEPROMリセット後もこの値が使われる
#define TAPPING_TERM 200

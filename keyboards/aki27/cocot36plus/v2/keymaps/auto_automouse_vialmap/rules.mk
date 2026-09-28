VIA_ENABLE         = yes
VIAL_ENABLE        = yes
ENCODER_MAP_ENABLE = yes
MIDI_ENABLE        = yes   # MIDI controls
TAP_DANCE_ENABLE   = yes   # Tap Dance: TD()キーコードを有効化。動作はVialがEEPROMで動的管理（keymap.cに定義なし）
COMBO_ENABLE       = yes   # Combo: キーコードを有効化。動作はVialがEEPROMで動的管理（keymap.cに定義なし）

# JIS/US配列切替 (twpair_on_jis) をキーボード直下から取り込む
SRC += keyboards/aki27/cocot36plus/twpair_on_jis.c

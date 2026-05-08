# Breakout

Breakout は、UIAPduino 向けの ch32fun ベースのゲーム用プロジェクトです。  
現在は最小構成のサンプルとして、1 画面で遊べるブロック崩しを入れています。

## 構成

- `src`: アプリ本体。現在はブロック崩し固有のロジックを主に `main.c` に配置
- `include`: プロジェクト共通設定ヘッダ (`funconfig.h`, `ch32v00x_conf.h`)
- `lib/gamelib`: ゲーム非依存の共通ライブラリ
  - `src`: 描画、入力、サウンド、LCD、tick、共通初期化 (`gamefunc.c`) などの実装
  - `inc`: `gamelib` の公開ヘッダ
- `lib/ch32v00x`: WCH SPL 風ペリフェラル実装
- `tools`: `firmware.bin` を配布用にコピーする補助スクリプト

## 現在のサンプル動作

- `LEFT` / `RIGHT`: パドル移動
- ブロックを全部壊すとクリア
- ボールを落とすと残機減少
- `ACTION`: ゲームオーバー後 / クリア後にリトライ

## Hardware Wiring

### ST7735 (SPI)

- `SCK`: `PC5`
- `MOSI`: `PC6`
- `CS`: `PC3`
- `DC`: `PD0`
- `RST`: `PC7`

### Buttons

- `LEFT`: `PA1`
- `RIGHT`: `PC4`
- `ACTION`: `PD2`
- `UP`: `PC1`
- `DOWN`: `PC2`

### Speaker

- `SPK_OUT`: `PD6` (`TIM2 CH3`, remap_3)

## Build / Upload

```bash
cd /Users/ooe/src/Breakout
pio run
pio run -t upload
```

`pio run` のたびに `build/firmware.bin` と `tools/.../firmware.bin` が更新されます。

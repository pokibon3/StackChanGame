# StackChanGame

`jumpSC` は、CH32V003 + ST7735 LCD 向けの小型シューティングゲームです。  
現在の実装は `src/main.c` に集約されており、`termGFX` スプライトを使って自機、敵、ボス、弾を描画します。

## Reference

このプロジェクトは、[yugi-tech-lab/BM-GamePod](https://github.com/yugi-tech-lab/BM-GamePod) を参考にしています。  
特に、ゲーム機構成や表示・入力まわりの考え方は上記リポジトリを参考にしつつ、このリポジトリ向けに調整しています。

## Current Game

- `UP` / `DOWN`: 自機の上下移動
- `ACTION`: ショット
- 通常ミス時: `Miss!`
- ライフ 0 時: `GAME OVER`
- クリア時: `GAME CLEAR`
- ボス戦中にミスした場合は、そのボス戦の続きから再開

## Project Layout

- `src`
  - アプリ本体。現在は主に `main.c`
  - `jumpSC.ino`: 元の Arduino 実装
- `include`
  - プロジェクト共通設定ヘッダ
- `lib/gamelib`
  - LCD、描画、入力、サウンド、tick などの共通ライブラリ
- `lib/ch32v00x`
  - CH32V003 向け低レベル実装
- `tools`
  - `firmware.bin` の出力補助スクリプト

## Hardware

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
cd /Users/ooe/src/jumpSC
pio run
pio run -t upload
```

`pio run` により `build/firmware.bin` と配布用 `firmware.bin` が更新されます。

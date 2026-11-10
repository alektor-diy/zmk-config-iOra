# iOra v0.9.0 のZMKファームウェア実装仕様

## 対象基板とコントローラー

- 対象は `iOra_v0.9.0` の `iOra_L_xiao/iOra_L.kicad_sch`、`iOra_L_xiao/iOra_L.kicad_pcb`、`iOra_R_xiao/iOra_R.kicad_sch`、`iOra_R_xiao/iOra_R.kicad_pcb`。左右の回路図とPCBのパッド接続を照合した。
- 左右それぞれに Seeed Studio XIAO nRF52840 Plus（`U1`）を1台搭載する。各側24キー、合計48キー。左右でキーの電気配線は共通で、実装位置は左右反転している。
- iOra基板上にOLED、ロータリーエンコーダー、RGB LED、左右間通信専用のコネクタはない。左右それぞれの `FPC1` はトラックボール用信号と電源を引き出している。

## キースキャン配線（左右共通）

- 各側は4行×6列のマトリクス。押下時の電流経路は `col → SW → ダイオードのアノード → カソード → row`。ZMKでは `zmk,kscan-gpio-matrix` の `diode-direction = "col2row"` を使う。列は `GPIO_ACTIVE_HIGH` で駆動し、行は `GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN` で読む。[ZMK v0.3のキースキャン設定](https://v0-3-branch.zmk.dev/docs/config/kscan)
- `row-gpios` は次の順で定義する。`P0.xx` は `&gpio0`、`P1.xx` は `&gpio1` のピン番号を表す。D番号はXIAOの端子名。
  - `row0` — `P0.04`（D4、`U1` パッド5）
  - `row1` — `P0.10`（D15、`U1` パッド19）
  - `row2` — `P0.05`（D5、`U1` パッド6）
  - `row3` — `P0.31`（D16、`U1` パッド20）
- `col-gpios` は次の順で定義する。
  - `col0` — `P0.03`（D1、`U1` パッド2）
  - `col1` — `P0.19`（D12、`U1` パッド16）
  - `col2` — `P0.28`（D2、`U1` パッド3）
  - `col3` — `P1.01`（D13、`U1` パッド17）
  - `col4` — `P0.29`（D3、`U1` パッド4）
  - `col5` — `P0.09`（D14、`U1` パッド18）
- `P0.09` と `P0.10` は両側でNFC機能からGPIOへ切り替える。ZMK v0.3では `CONFIG_NFCT_PINS_AS_GPIOS=y`。Zephyr 4.1へ移行したZMKでは `&uicr` に `nfct-pins-as-gpios` を指定する。D4/D5の既定のI²CとD14/D15のUARTも、同じピンを使う構成では無効にする。[Seeedのピン配置](https://wiki.seeedstudio.com/XIAO_BLE/#hardware-overview)・[ZMKのNFC端子設定の移行手順](https://zmk.dev/blog/2025/12/09/zephyr-4-1#nrf52840-nfc-pins-as-gpio)
- `P0.31` はXIAO内蔵の `VBAT` 分圧回路にも接続されている。両側で `row3` をGPIO入力として使い、XIAO標準の `vbatt` を使う測定経路は無効にする。電池残量測定には後述の `UNREG` を使う。[ZephyrのXIAO Plus資料](https://docs.zephyrproject.org/latest/boards/seeed/xiao_nrf52840_plus/doc/index.html#connections-and-ios)

## キー番号と物理配列

- 左右とも電気上の列は `col0` から `col5` へ外側から内側の順。各行の `col0`〜`col5` に対応するスイッチは `row0` が `SW1`〜`SW6`、`row1` が `SW7`〜`SW12`、`row2` が `SW13`〜`SW18`、`row3` が `SW19`〜`SW24`。全24交点にスイッチが1個ずつある。
- 両側のPCBを表面から見て、各行を左から右へ読む順序は次のとおり。
  - 上から1段目（`row0`）— 左 `SW1, SW2, SW3, SW4, SW5, SW6` ／ 右 `SW6, SW5, SW4, SW3, SW2, SW1`
  - 上から2段目（`row1`）— 左 `SW7, SW8, SW9, SW10, SW11, SW12` ／ 右 `SW12, SW11, SW10, SW9, SW8, SW7`
  - 上から3段目（`row2`）— 左 `SW13, SW14, SW15, SW16, SW17, SW18` ／ 右 `SW18, SW17, SW16, SW15, SW14, SW13`
  - 最下段（`row3`）— 左 `SW19, SW20, SW21, SW22, SW23, SW24` ／ 右 `SW24, SW23, SW22, SW21, SW20, SW19`。外側から3個目と4個目の間に電源スイッチの位置があり、そこでキー間隔が空く。電気上は `col2` の次が `col3` で、キー位置の追加は不要。
- 共通の `zmk,matrix-transform` を `rows = <4>`、`columns = <12>` とし、左を列0〜5、右を列6〜11に割り当てる場合、右側の `col-offset` は6。各行 `r` の `map` は物理的な左から右の順で `RC(r,0)`〜`RC(r,5)`、続けて `RC(r,11)`〜`RC(r,6)` とする。この順に48個のキーバインディングを対応させる。[ZMKのmatrix transform設定](https://zmk.dev/docs/config/layout)
- 右 `SW18` は回路図ではスイッチの2番が `col5`、PCBでは1番が `col5`。無極性のスイッチの両端が入れ替わっており、ダイオードの向きとマトリクス座標 `row2 / col5` は一致する。

## トラックボールの配線と電源

- センサー基板は `pwm3610_breakout_board_M2` の `pmw3610_pcb.kicad_sch` と `pmw3610_pcb.kicad_pcb` を照合した。センサーは `PMW3610DM-SUDU`。コネクタはiOra側 `FPC1`、センサー側 `J2` ともに `AFC01-S08FCA-00`（8極、0.5 mmピッチ）。
- 左右でXIAO側の信号割り当ては共通。以下はiOra左 `FPC1`／iOra右 `FPC1`／センサー `J2` のパッド番号。
  - `TB_nRESET` — `P1.13`（D8、`U1` パッド9）、左1／右8／`J2` 8。
  - `TB_MOTION` — `P1.05`（D18、`U1` パッド22）、左2／右7／`J2` 7。
  - `TB_SCLK` — `P1.14`（D9、`U1` パッド10）、左3／右6／`J2` 6。
  - `TB_SDIO` — `P1.07`（D19、`U1` パッド23）、左4／右5／`J2` 5。データ信号はこの1本。
  - NC — 左5／右4／`J2` 4。
  - `TB_nCS` — `P1.15`（D10、`U1` パッド11）、左6／右3／`J2` 3。
  - `VSYS_3.3V` — XIAOの3V3出力からセンサー基板の `VIN` へ、左7／右2／`J2` 2。
  - `GND` — 左8／右1／`J2` 1。
- 左は `FPC1` の `n` 番を `J2` の `9−n` 番へ、右は `FPC1` と `J2` の同じ番号同士へ接続する。実装時は使用するケーブルでこの対応になることを通電前に導通確認する。
- センサー基板は `VIN` の3.3 Vから `U2`（`TLV74318PDBVR`）でセンサーの `VDD` 用1.8 Vを生成する。XIAOの3.3 V信号を直結するため、両側ともセンサー基板の `JP1` は1–2を短絡し、3を開放して `VDDIO = VIN = 3.3 V` とする。`nRESET` はセンサー基板の `R1`（10 kΩ）で `VDDIO` にプルアップされている。
- センサーは `SCLK`、双方向 `SDIO`、`nCS` の3線SPIを使う。付属テストFWと同じ方式で `SPIM_MOSI` と `SPIM_MISO` の両方を `P1.07` に割り当て、`SPIM_SCK` は `P1.14`、`spi-max-frequency` は2 MHzとする。`nCS`、`MOTION`、`nRESET` はアクティブLow。[ZephyrのPMW3610バインディング](https://docs.zephyrproject.org/latest/build/dts/api/bindings/input/pixart,pmw3610.html)
- ZMK v0.3では、`efogdev/zmk-pmw3610-driver` の `pixart,pmw3610` を起点とする。`nCS` は `P1.15`、`irq-gpios` は `P1.05` のアクティブLow・プルアップ。起動時はドライバーがSPI経由でセンサーをリセットする。ハードウェアリセット線 `P1.13` をドライバーの電源管理で使う場合は、同ドライバーの `rst-gpios` にアクティブLowで指定する。[PMW3610ドライバーの実装](https://github.com/efogdev/zmk-pmw3610-driver/blob/main/src/pmw3610.c)

## 電源と電池電圧入力（左右共通）

- 各側に単3形1本用ホルダー `BT1` がある。保護回路後、昇圧前の `UNREG` がXIAOのD0（`P0.02`／`AIN0`、`U1` パッド1）につながっている。FWで電池電圧を扱う場合、この地点の電圧を読む。
- `UNREG` は `MCP1640T-I/CHY`（`U101`）の入力。回路図上の3.5 V昇圧出力は `SS14`（`D501`）を経て `BAT+` となり、XIAOの `VBAT`（`U1` パッド32）に入る。`BAT+` はセルの生電圧とは別ネット。`FPC1` の3.3 VはXIAOの3V3出力から供給される。
- スライドスイッチ `SW500` は昇圧ICの `EN` を `UNREG` と `GND` の間で切り替える。キースキャンに割り当てるGPIOはない。
- 初回FWから `sekigon-gonnoc/zmk-feature-non-lipo-battery-management` で電圧を残量に換算し、BLEホストに通知する。左右で `CONFIG_ZMK_NON_LIPO_BATTERY_MANAGEMENT=y` とし、`zmk,battery` に `compatible = "zmk,non-lipo-battery"` のノードを指定する。`io-channels = <&adc 0>` で `UNREG`（AIN0）を測定する。ZMKの残量取得は `CONFIG_ZMK_BATTERY_REPORTING_FETCH_MODE_STATE_OF_CHARGE` でモジュールが算出した残量率を使う。[電池管理モジュールの実装](https://github.com/sekigon-gonnoc/zmk-feature-non-lipo-battery-management/blob/main/src/non_lipo_battery_management.c)・[ZMK v0.3の残量取得設定](https://github.com/zmkfirmware/zmk/blob/v0.3/app/Kconfig#L347-L365)
- 左右両方の残量をBLEホストに通知する。左centralは自側の残量に加え、右peripheralの残量を取得して中継する。ZMK v0.3ではcentral側の `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING` と `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_PROXY` を有効にする。ホスト画面での左右別表示は、対象OSで確認する。[ZMK v0.3の電池残量設定](https://v0-3-branch.zmk.dev/docs/config/battery#peripheral-battery-monitoring)
- 残量換算は、エネループなどの単3形ニッケル水素充電池を基準とする。初回FWでは100%を1,400 mV、0%を1,100 mV、USB非給電時の低電圧シャットダウンを1,050 mV以下、接続待ち時のスリープを5分とする。iOraのしきい値は、ニッケル水素充電池で実測した `UNREG` を基に調整する。[電池管理モジュールの設定項目](https://github.com/sekigon-gonnoc/zmk-feature-non-lipo-battery-management#configuration-options)

## ヒアリングで確認した構成

- 初回FWから左右両方のトラックボールを有効にする。センサーは両側とも PixArt PMW3610。左右のXIAOがそれぞれ自側のセンサーを読み取る。
- トラックボールの初期割り当ては、右手側がマウスカーソル移動、左手側が縦・横スクロール。左はベースレイヤーからスクロールとして使えるようにする。
- 左のスクロールでは、取り付け向きに応じて正規化したX/Yに対し、Y反転（`INPUT_TRANSFORM_Y_INVERT`）、両軸の1/16スケーリング（`zip_xy_scaler 1 16`）、Xを横ホイール・Yを縦ホイールに変換（`zip_xy_to_scroll_mapper`）の順に処理する。左センサーの初期解像度は800 CPIとする。[ZMKの入力コード変換](https://zmk.dev/docs/keymaps/input-processors/code-mapper)
- 右のカーソル操作は800 CPI、X/Yの倍率1:1を初期値とする。感度の微調整は実機確認後に行う。
- 左手側をcentral、右手側をperipheralとする。左のトラックボールはcentralのローカル入力、右はperipheralから `zmk,input-split` 経由で受け取り、左から両方の入力をホストへ送る。[ZMK v0.3のポインティングデバイス実装ガイド](https://v0-3-branch.zmk.dev/docs/development/hardware-integration/pointing)
- 左右の基板間はBLEで接続する。
- centralからホストへのキーボード・マウス入力は、BLEとUSBの両方に対応する。
- 初回FWはZMK `v0.3` 系で作る。`config/west.yml` とGitHub Actionsのビルドワークフローは `v0.3` にそろえる。左右のビルドは既存のXIAO用ターゲット `seeeduino_xiao_ble` を出発点とし、Plusの追加端子は上記のnRF52840 GPIO番号で指定する。
- 初回FWでZMK Studioを有効にする。左centralのビルドに `CONFIG_ZMK_STUDIO=y` とUSB用の `studio-rpc-usb-uart` スニペットを指定する。Studio用の `zmk,physical-layout` はキーマップおよびマトリクス変換と同じ順で48キーの `keys` を持たせる。左右のPCBから各側のキー位置と回転を取得し、両側を表示用に並べる。[ZMK v0.3のStudioビルド設定](https://v0-3-branch.zmk.dev/docs/features/studio#building)・[物理レイアウトの定義](https://v0-3-branch.zmk.dev/docs/development/hardware-integration/physical-layouts)
- iOraの初期キーマップは、同じ物理配列を持つreNeraの現在のQMK/Vial設定を基準にする。reNeraから書き出した `.vil` のレイヤー、キー割り当て、設定済みのコンボやマクロなどを確認し、同じ物理位置に対応するZMKの動作へ移植する。[Vialのレイアウト保存処理](https://github.com/vial-kb/vial-gui/blob/main/src/main/python/protocol/keyboard_comm.py#L333-L367)

## reNeraから移植する初期キーマップ

- Vial書き出し `reNera.vil` の機器UIDを、ローカルの `D:/GitHub/vial-qmk/keyboards/renera/keymaps/vial/config.h` と照合した。物理順は同じディレクトリの `vial.json` と、reNeraの `keyboard.json` の `LAYOUT` 定義で一致する。各段の左6キーはVialの行0〜3、右6キーは行4〜7で、どちらも列0〜5の順に並ぶ。iOraでは前述の物理順に対応させるため、右手側の `SW` 番号は降順になる。
- 以下はVialのキーコードを、各段で物理的に左から右へ並べたもの。斜線の左が左手側、右が右手側。`KC_NO` は `&none`、`KC_TRNS` は `&trans` に対応する。
- レイヤー0（ベース）
  - 1段目 — `KC_ESCAPE KC_Q KC_W KC_E KC_R KC_T` ／ `KC_Y KC_U KC_I KC_O KC_P KC_BSPACE`
  - 2段目 — `KC_TAB KC_A KC_S KC_D KC_F KC_G` ／ `KC_H KC_J KC_K KC_L KC_SLASH KC_ENTER`
  - 3段目 — `KC_LSHIFT KC_Z KC_X KC_C KC_V KC_B` ／ `KC_N KC_M KC_COMMA KC_DOT KC_UP KC_RSHIFT`
  - 4段目 — `KC_LCTRL KC_LGUI KC_LALT KC_DELETE KC_SPACE LT1(KC_F7)` ／ `LT2(KC_F10) KC_NO KC_MINUS KC_LEFT KC_DOWN KC_RIGHT`
- レイヤー1（数字・記号）
  - 1段目 — `KC_NO KC_NO KC_NO KC_NO KC_NO KC_NO` ／ `KC_7 KC_8 KC_9 KC_MINUS KC_EQUAL KC_JYEN`
  - 2段目 — `KC_NO KC_NO KC_NO KC_NO KC_NO KC_NO` ／ `KC_4 KC_5 KC_6 KC_NO KC_LBRACKET KC_RBRACKET`
  - 3段目 — `KC_NO KC_NO KC_NO KC_NO KC_NO KC_NO` ／ `KC_1 KC_2 KC_3 KC_SCOLON KC_QUOTE KC_NONUS_HASH`
  - 4段目 — `KC_NO KC_NO KC_NO KC_NO KC_LSHIFT KC_NO` ／ `KC_NO KC_0 KC_DOT KC_NO KC_NO KC_RO`
- レイヤー2（ファンクションキー）
  - 1段目 — `KC_NO KC_F1 KC_F2 KC_F3 KC_F4 KC_F5` ／ 右は全キー `KC_NO`。
  - 2段目 — `KC_NO KC_F6 KC_F7 KC_F8 KC_F9 KC_F10` ／ 右は全キー `KC_NO`。
  - 3段目 — `KC_NO KC_F11 KC_F12 KC_NO KC_NO KC_NO` ／ 右は全キー `KC_NO`。
  - 4段目 — reNeraでは左右とも全キー `KC_NO`。iOraでは左の6キーを、物理的な左端から `SW19`＝BLE 1（`&bt BT_SEL 0`）、`SW20`＝BLE 2（`&bt BT_SEL 1`）、`SW21`＝BLE 3（`&bt BT_SEL 2`）、`SW22`＝BLE 4（`&bt BT_SEL 3`）、`SW23`＝BLE 5（`&bt BT_SEL 4`）、`SW24`＝現在選択中のホストのペアリング消去（`&bt BT_CLR`）に割り当てる。[ZMK v0.3のBluetooth操作](https://v0-3-branch.zmk.dev/docs/keymaps/behaviors/bluetooth)
  - 4段目右端 — iOraでは右手側の `SW19`（位置47、ベースでは右矢印）にStudioのロック解除 `&studio_unlock` を割り当てる。右手側の最下段内側で `LT2(KC_F10)` を長押しし、離れた最下段の右端を押す。[ZMK v0.3のStudio設定](https://v0-3-branch.zmk.dev/docs/features/studio#keymap-changes)
- iOraのブートローダー操作は、各手の最下段内側のレイヤータップを長押しし、その対角にある同じ側の上段外側キーを押す。通常の入力と区別しやすい組み合わせにし、押した側だけが書き込みモードに入る。書き込み時は対象側をUSB接続する。初回書き込みは対象側のXIAOのリセットボタンを2回押す。[ZMK v0.3のリセット動作](https://v0-3-branch.zmk.dev/docs/keymaps/behaviors/reset#split-keyboards)・[XIAO Plusの書き込み手順](https://docs.zephyrproject.org/latest/boards/seeed/xiao_nrf52840_plus/doc/index.html#flashing)
  - 左手側 — `LT1(KC_F7)` を長押しし、レイヤー1の左上 `SW1`（位置0、ベースではEsc）の `&bootloader` を押す。
  - 右手側 — `LT2(KC_F10)` を長押しし、レイヤー2の右上 `SW1`（位置11、ベースではBackspace）の `&bootloader` を押す。
- レイヤー3と4は全キー `KC_TRNS`。書き出しには有効なマクロ、タップダンス、キーオーバーライド、エンコーダー割り当てがない。
- レイヤータップは `&lt 1 F7` と `&lt 2 F10`。短押しはF7／F10、長押し中はそれぞれレイヤー1／2。書き出しの判定時間は200 ms。日本語配列で使うキーは `KC_JYEN` を `INTERNATIONAL_3`、`KC_RO` を `INTERNATIONAL_1`、`KC_NONUS_HASH` を `NON_US_HASH` に対応させる。[ZMK v0.3のレイヤータップ](https://github.com/zmkfirmware/zmk/blob/v0.3/app/dts/behaviors/layer_tap.dtsi)・[ZMK v0.3のキーコード](https://github.com/zmkfirmware/zmk/blob/v0.3/app/include/dt-bindings/zmk/keys.h)
- コンボは次の7件。括弧内はiOraの物理順における0始まりの `key-positions`。判定時間は書き出しと同じ50 msを使う。マウスボタンはreNeraと同じく全構成キーの解放まで保持する設定とし、2キーと3キーのクリックが重なる組み合わせを実機で確認する。[ZMKのコンボ仕様](https://zmk.dev/docs/keymaps/combos)
  - Esc + Q（`0 1`）→ `GRAVE`
  - P + Backspace（`10 11`）→ Print Screen
  - Left + Down（`45 46`）→ Home
  - Down + Right（`46 47`）→ End
  - D + F（`15 16`）→ 左クリック
  - S + D（`14 15`）→ 右クリック
  - S + D + F（`14 15 16`）→ 中クリック

## 実装・実機での確認事項

- **トラックボール** — 左右のセンサーの起動とスリープ復帰、取り付け向きに合わせた軸変換、左右同時操作時の応答を確認する。軸変換はX/Y入れ替えから始める。カーソル感度と縦横スクロールの方向・速度は実機で調整する。
- **電池残量の実機確認** — 負荷時とUSB給電時の `UNREG` の実測値、低電圧時の動作、スリープ復帰を確認する。`row3` は内蔵分圧回路が接続された状態で、電池給電時とUSB給電時の両方で6キーの押下・解放を確認する。

# iOra ユーザーガイド

iOraは左右にトラックボールを備えた48キーの分割キーボードです。左手側がPCとUSBまたはBluetoothでつながり、右手側は左手側とBluetoothでつながります。

![iOraのデフォルト配置。左の親指側にF7／L1、右の親指側にF10／L2](docs/images/layer-0.svg)

- [全レイヤーの配置と同時押しの図](iOra-keymap-cheatsheet.md)
- [Bluetoothの接続と切り替え](#bluetooth)
- [ZMK Studioでキーを変更する](#zmk-studioでキーを変更する)
- [DYA Studioでコンボを変更する](#dya-studioでコンボを変更する)
- [設定ファイルで動作を変更する](#設定ファイルで動作を変更する)
- [ファームウェアを書き込む](#ファームウェアを書き込む)

## Bluetooth

1. 両側の電源を入れます。Bluetoothで使うときは、左手側からUSBケーブルを抜きます。
2. 右手の **F10／L2** を長押ししながら、左手最下段の **BLE 1〜5** のいずれかを押して接続枠を選びます。
3. PCのBluetooth設定で **iOra** を選びます。次のPCは未使用の枠に登録し、以降は同じキー操作で切り替えます。

選択中の枠だけを消去するには、**F10／L2** を長押ししながら **BLE Clear** の位置を押します。そのPCへつなぎ直す場合は、PC側のBluetooth設定からも既存のiOraを削除して再ペアリングします。[ZMKのBluetooth操作](https://v0-3-branch.zmk.dev/docs/keymaps/behaviors/bluetooth)

![L2の左最下段にBLE 1〜5とBLE Clear。右下にStudio Unlock](docs/images/layer-2.svg)

## ZMK Studioでキーを変更する

普段使うキーやレイヤー上の割り当ては、ZMK Studioから変更できます。WindowsでBluetooth接続する場合はデスクトップ版を使います。USB接続なら左手側をPCにつなぎ、Chrome／EdgeでWeb版も使えます。[ZMK Studioの使い方](https://v0-3-branch.zmk.dev/docs/features/studio)・[Web版](https://zmk.studio/)・[デスクトップ版](https://zmk.studio/download)

1. ZMK Studioで **iOra** を選んで接続します。
2. **F10／L2** を長押ししながら、右手の右下にある **Studio Unlock**（通常は右矢印）を押します。位置は上のLayer 2の図で確認できます。
3. 変更したいレイヤーとキーを選び、割り当てを変更して **Save** を押します。オートマウスの切り替えキーを移すときは **Auto Mouse Toggle** を選びます。

Studioで保存したキーやレイヤー名は、通常のFW更新後も優先されます。**Restore Stock Settings** を使うとStudioで保存した変更を消去し、書き込んだFWのキーマップに戻せます。[ZMKの設定保存](https://zmk.dev/docs/config/settings)

### トラックボールの左右を切り替える

**F7／L1を長押ししながら、左下の左Ctrlの右隣**（通常はGUIキー）を1回押します。押すたびに左右の役割が入れ替わり、電源を入れ直しても最後の設定を使います。Studioでは **Trackball Swap** を好きなキーに割り当てられます。[切り替えキーの図](docs/images/layer-1.svg)

### トラックボールにキーを割り当てる

1. Studioで **Trackball Settings**（L5）を開きます。
2. L1は最上段、L2は2段目、L3は3段目の左端2枠を編集します。左端が上方向、その右隣が下方向です。**Key Press** から好きなキーコードを選びます。初期値はどの段も音量アップ・ダウンです。
3. **Save** を押します。L1・L2・L3でキー入力を担当するボールを転がすと、そのレイヤーに対応するキーが入力されます。初期状態は左ボールで、左右を入れ替えると右ボールです。無効にしたい方向は **None** にします。複数が有効なときは、優先順位が最も高いレイヤーの設定を使います。

![L5の設定枠。最上段がL1、2段目がL2、3段目がL3。各段の左端が上方向、右隣が下方向](docs/images/layer-5.svg)

## DYA Studioでコンボを変更する

`dev/dya-studio` はDYA Studio対応FWです。cormoran版ZMK（独自Studio RPC対応、Zephyr 4.1）とRuntime Comboモジュールを使います。初回移行では、このブランチからビルドした **左右両方** のFWを書き込んでください。

生成済みFW: [左手用 iOra_L.uf2](iOra_L.uf2)・[右手用 iOra_R.uf2](iOra_R.uf2)。現在は通常使用中のハングを調べるため、スタック容量を増やした切り分け版です。DYA接続・コンボ表示・両ボールの基本動作は前版で確認済みですが、今回の安定性改善は未確認です。[ハング調査記録](docs/dya-stability-investigation.md)・[ビルド・検証記録](docs/dya-studio-build.md)を参照してください。

1. 左手側をUSBでPCにつなぎ、Chrome／Edgeで [DYA Studio](https://studio.dya.cormoran.works/) を開き、USB接続でiOraを選びます。
2. **F10／L2＋右下のStudio Unlock** でロックを解除します。
3. **Macros & Combos** 画面のコンボ一覧から対象を選び、押すキーの位置、出力する動作、有効レイヤー、タイムアウト、slow-releaseなどを編集します。
4. 編集した内容を保存し、再起動後も反映されることを確認します。初期値のあるコンボは **Reset to Default** でFW内の初期値に戻せます。

無線で設定する場合は、同じPCへBluetoothでキー入力できる状態にしてUSBを抜き、**Studio Unlockを押してから** DYA Studioの **Bluetoothで接続** を選びます。Satelliteは `fix/dya-studio-ble-advertising` ブランチを参照し、Unlock中はStudioへ広告制御を譲り、ロック後に状態通知を再開する構成です。この版での無線接続の実機確認はまだ行っていません。

コンボは最大16個です。スロット0〜10に従来の11個を同じキー位置・動作・レイヤー・50 msのタイムアウトで登録し、11〜15は追加用に空けています。マウスの左／右／中クリックとオートマウス中の親指コンボも編集できます。コンボの保存値はスロット番号に対応するため、FW側の初期値を変更する際は既存の `slot` 番号を維持してください。保存済みのコンボ設定はFW内の初期値より優先されます。

キー割り当てとL5経由のトラックボールキー設定もDYA Studioで編集できます。Runtime Macroは含めていません。

### PMW3610センサーの設定と診断

センサーには [cormoran/zmk-driver-pmw3610-with-custom-studio-rpc](https://github.com/cormoran/zmk-driver-pmw3610-with-custom-studio-rpc) を使用します。現在のUF2では、Zephyr 4.1のXIAO標準SPI2がセンサー端子と競合する問題を修正し、左右のボール動作をユーザー実機で確認済みです。

この版も **左右両方** を更新してください。Studio Unlock後、DYA Studioの **診断 → トラックボールセンサー（PMW3610）** でセンサーの認識・初期化状態を確認できます。右手のセンサーへの診断・設定は左手経由のsplit RPC relayを有効にしています。動かない場合は、センサーのReady、Init error、Product IDを確認してください。

診断の状態行はセンサーごとに1行表示され、この構成では上が左（ローカル）、下が右（リレー取得）です。**センサーのライブビューをストリーミング中は、対象センサーの通常入力が停止します。** ボール操作に戻る前に **ストリーミング停止** を押してください。

DYAの **トラックボール → PMW3610ドライバー** からセンサーのCPIや軸設定などを変更・保存できる構成です。初期値は従来と同じ800 CPI、XY入れ替え＋両軸反転、起動待機1秒、レポート間隔8 msです。左右の設定IDは `left`／`right` に分けています。

**プロセッサー** 欄はiOra独自のボール役割分岐を使っているため、DYAのRuntime Input Processorとしては表示されません。この欄の「見つからない」と、センサー自体の初期化失敗は別です。スクロール倍率、ボールのキー入力感度、オートマウスの時間は引き続き下記の設定ファイルで変更します。

移行後は左右のキー入力、両方のボール、オートマウス、左右の役割切り替えを確認してください。重なりのある `<14 15>`／`<15 16>`／`<14 15 16>` のクリックコンボは、2キーと3キーそれぞれで動作を確認します。

[DYA導入ガイド](https://studio.dya.cormoran.works/developer-guide/modules/keymap)・[Runtime Comboの仕様](https://github.com/cormoran/zmk-feature-runtime-combo)

## 設定ファイルで動作を変更する

トラックボールの感度やFW内の初期値などは設定ファイルで変更し、FWを再ビルドします。DYA対応FWでは、コンボの追加やタイミングを上記の画面から変更できます。

| 変更したいこと | 編集する場所 | 変更方法 |
| --- | --- | --- |
| キーやレイヤー名 | [config/iora.keymap](config/iora.keymap) | 対象レイヤーの `bindings` または `display-name` を編集します。各行は左手6キー、右手6キーの順です。 |
| 同時押し（コンボ）の初期値 | [config/iora.keymap](config/iora.keymap) | `runtime_combo_defaults` の `key-positions`、`bindings`、`layers`、`timeout-ms` を編集します。既存の `slot` 番号は維持します。位置番号は左上を0とし、各段を左から右へ12個ずつ数えます。 |
| オートマウスのON/OFFキー | [config/iora.keymap](config/iora.keymap) | `&auto_mouse_toggle` を移します。初期位置はL1の左下（通常は左Ctrl）です。切り替えた状態は左手側に保存されます。 |
| トラックボールの左右切り替えキー | [config/iora.keymap](config/iora.keymap) | `&trackball_swap` を移します。初期位置はL1の左下から2番目（通常はGUIキー）です。 |
| オートマウスの解除時間・入力直後の待機時間 | [iora_left.overlay](boards/shields/iora/iora_left.overlay) | `processor = <&zip_temp_layer 4 800>` の最後が解除までの時間、`require-prior-idle-ms = <200>` がキー入力直後の待機時間です。単位はmsです。 |
| ボールのキー入力の感度・連続入力間隔 | [iora_left.overlay](boards/shields/iora/iora_left.overlay) | `counts-per-tap = <48>` を小さくすると短い移動で反応します。`repeat-interval-ms = <40>` は連続入力の最短間隔です。 |
| スクロールの速さ | [iora_left.overlay](boards/shields/iora/iora_left.overlay) | `auxiliary-scroll` は通常時のスクロール、`primary-scroll` はL1〜3のスクロールの設定です。各 `zip_xy_scaler 1 16` の分母を小さくすると速くなります。 |
| トラックボールのセンサー感度 | [iora.dtsi](boards/shields/iora/iora.dtsi) | `cpi = <800>` を変更します。左右共通の設定なので、変更後は両側のFWを書き込みます。 |
| ディープスリープまでの時間 | [config/iora.conf](config/iora.conf) | `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT` を変更します。iOraの初期値は3,600,000 ms（1時間）です。変更後は両側のFWを書き込みます。 |

たとえばミュートを割り当てるなら、対象のキーを `&kp C_MUTE` に置き換えます。`&none` は入力なし、`&trans` は下の有効なレイヤーの割り当てを使います。オートマウスのクリック位置を変更する場合は、L4の割り当てとコンボに加えて、`iora_left.overlay` の `excluded-positions` も合わせます。[ZMKキーコード一覧](https://v0-3-branch.zmk.dev/docs/keymaps/list-of-keycodes)・[ZMKコンボの設定](https://zmk.dev/docs/keymaps/combos)

### GitHub Actionsで再ビルドする

1. このリポジトリをGitHubでフォークします。フォークの **Actions** でワークフローを有効にします。
2. 設定を編集してコミットをプッシュします。または **Actions → Build ZMK firmware → Run workflow** で、編集したブランチを選びます。
3. 成功した実行の **Artifacts** から `firmware` をダウンロードし、ZIPを展開します。`iOra_L.uf2` が左手用、`iOra_R.uf2` が右手用です。[ZMKのビルドと書き込み手順](https://v0-3-branch.zmk.dev/docs/user-setup)

## ファームウェアを書き込む

1. このブランチの [左手用UF2](iOra_L.uf2) と [右手用UF2](iOra_R.uf2)、またはこのブランチのGitHub Actionsで再ビルドしたUF2を用意します。DYA対応への初回移行時は両方を更新します。
2. 書き込む側をUSBでPCにつなぎ、ブートローダーに入れます。左側は **F7／L1を長押し＋左上のEscの位置**、右側は **F10／L2を長押し＋右上のBkspの位置**です。右側をキー操作で起動する場合は、左右が接続されている状態で行います。キー操作が使えない場合は、対象側のXIAOのリセットボタンを2回続けて押します。
3. 表示されたUSBドライブに対応するUF2をコピーします。両側の更新が必要な場合は片側ずつ行います。

キー配置・コンボ・オートマウス設定の変更なら左側の更新で反映されます。通常のUF2で上書きした場合は、ペアリング、Studioで保存したキーマップ、オートマウスのON/OFF、ボールの左右設定が残ります。[ブートローダーキーの図](iOra-keymap-cheatsheet.md)・[ZMKの設定保存](https://zmk.dev/docs/config/settings)

# DYA Studio公開用安定版のビルド・検証記録

2026-11-17、`codex/dya-studio-stable` で公開用の左右UF2を生成。実機確認を進めた `dev/dya-studio` の構成から、ZMK Satelliteのwest依存と有効化設定を削除した。DYA対応PMW3610ドライバー、SPI2競合修正、スタック容量見直しは維持している。

Satelliteなしの公開構成はクリーンビルド・静的検査済みで、最終実機確認は未実施。Satellite入り開発版はコミット `2601b40` として元の `dev/dya-studio` ブランチに保存した。

## 開発版での実機確認履歴

初版はユーザー実機でDYA Studioの接続・コンボ表示が成功した一方、左右ともボールが動かないとの報告があった。診断対応版でもID `0xff`・初期化エラー `-5` が報告された。

修正前の生成devicetreeでは、左右とも `/soc/spi@40023000`（SPI2）が `okay` で、P1.13・P1.15・P1.14を使用していた。これはセンサーのRESET・CS・SCK端子と一致する。Zephyr 4.1のXIAO定義が有効化する未使用SPI2を `&xiao_spi { status = "disabled"; };` で無効にした。修正後、ユーザー実機で右ボール入力と左スクロールの復旧を確認済み。

ユーザー提示の診断画面では2台ともReady、Product ID `0x3e`、Revision `0x01`、Init error `0`。ライブビューのストリーミングも成功。左ボールの通常入力が止まっていた件は、ストリーミング停止後にスクロール動作を確認できた。ドライバーはキャプチャ中に通常入力を止める実装。

## ビルド環境

- Python 3.12.8、west 1.5.0、CMake 3.31.10、Ninja 1.13.2
- GNU Arm Embedded GCC 12.2.0（既存のQMK MSYS環境のコンパイラ）
- ZMK: `1fc72aaff7a42bd533c1f4c9b3f23c5317c745f4`
- Zephyr: `10ba6d0cb38b`（`v4.1.0+zmk-fixes`）
- Runtime Combo: `d9490e7b5d41c516221d131241009e81740efcee`
- Custom Settings: `c6a7fef3a3be3d3ace5de9a4b0628c6418cd1f3f`
- PMW3610: `5c34ea0eec246a1c986111417cd779b53144629a`（`zmk-driver-pmw3610-with-custom-studio-rpc`）

依存モジュールは一時ワークスペースに取得し、`build.yaml` と同じボード・shield・snippet・Kconfig指定で、新規の `build/stable-left` と `build/stable-right` へビルド。iOra自身は `ZMK_EXTRA_MODULES` として読み込み、リポジトリの `config` を使用した。GitHub Actionsの実行は行っていない。

## 生成物

| 対象 | ファイル | UF2サイズ | FLASH使用量 / 788 KiB | RAM使用量 / 256 KiB |
| --- | --- | --- | --- | --- |
| 左central | [iOra_L.uf2](../iOra_L.uf2) | 660,480 B | 330,012 B（40.90%） | 141,508 B（53.98%） |
| 右peripheral | [iOra_R.uf2](../iOra_R.uf2) | 472,576 B | 236,224 B（29.28%） | 83,776 B（31.96%） |

SHA-256:

```text
iOra_L.uf2  e318b179c4d825771e8a7329afa880ae0a318a5b276e0fcbf5709a9695d1d073
iOra_R.uf2  c75105e76ea771c035b80a2bbe70d3ae304fc01e901528e10a05962609b9255c
```

## 確認済み

- 左右ともコンパイル・リンク・UF2変換に成功。
- westの元マニフェストと展開後マニフェストにSatelliteが存在しないこと、左右の生成設定・Zephyrモジュール一覧にSatelliteが存在しないこと、ELFにSatellite／Prospectorのシンボルが存在しないことを確認。iOra側への広告回避コードも含まない。左側のStudio BLE・USB通信とUnlock広告設定は維持。
- 修正前の両側devicetreeでSPI2の競合を再現。修正後はSPI0が有効、SPI2が無効であり、他の有効なpinctrl利用デバイスがセンサーのIRQ・SDIO・RESET・SCK・CS端子を使用しないことを自動検査。ELFにもSPI2用の `spi_nrfx_spi.c` が含まれないことを確認。
- 旧キーマップと比較し、11コンボのキー位置・出力・有効レイヤー・timeout・slow-releaseが一致。スロット0〜10は重複なし。6レイヤーのキー割り当ては変更なし。
- 左側の生成設定でRuntime Combo、Studio RPC、16スロットを有効化し、ELFにコンボRPCハンドラーと初期値が含まれることを確認。右側ではRuntime Comboを有効化していない。
- 左右ともPMW3610、非LiPo電池管理が有効。Satelliteは含めない。
- 左右とも新しいPMW3610ドライバー・センサー設定・split relayが有効。古いefogdevドライバーは含まれない。左ELFにはPMW3610のStudio RPCハンドラーがあり、右にはStudioを追加していない。
- 両側の軸設定（XY入れ替え＋両軸反転）、8 ms間隔、起動待機1000 ms、split relayの240バイト上限、低優先スタック4096バイトを生成設定で確認。センサーの設定IDは生成devicetreeで左 `left`／右 `right`。
- UF2の全ブロックについてヘッダー・末尾マジック、nRF52840 family ID、連番、総ブロック数、アドレスを検査。開始位置は `0x27000`、書き込み範囲はアプリケーション領域内。
- `git diff --check` は成功。

保存済み開発版 `2601b40` と比較し、キーマップ・コンボ・独自ボール制御ソース・左右overlayに変更がないことも確認。スタック容量はシステム4096 B・input 2048 B・低優先4096 B（両側）、Studio RPC 6000 B・split中央コマンド送信1024 B（左）を維持。

依存ソース由来のコンパイラ警告（未使用関数、型・書式、宣言など）、KSCANの非推奨警告、GNU linkerのRWXセグメント警告は残る。今回のビルド環境には外部DTCがなく、ZephyrのPython devicetree処理を使用した。

## 実機での確認が必要な項目

- 左右のキー入力・Bluetooth接続・両ボールの方向・スクロール・オートマウス・ボールの役割切り替え。
- DYA StudioのUSB接続、Studio Unlock、11個の初期コンボ表示。
- コンボの変更・追加・無効化・保存・再起動後の保持・Reset to Default。
- 重複する2キー／3キーのクリックコンボと、オートマウス中の親指コンボ。
- 起動直後、スリープ復帰後のセンサー動作。

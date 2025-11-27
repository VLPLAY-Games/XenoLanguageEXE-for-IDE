# Xeno Language EXE for Windows IDE

## 🏗️ プロジェクト概要

このリポジトリは、**Xeno IDE**で使用するために特別に設計されたXenoプログラミング言語のWindows実行可能ファイル版を提供します。ArduinoベースのXenoコードがWindowsシステムでネイティブに実行できる互換性ブリッジを作成します。

## 🌉 ブリッジアーキテクチャ

### 3層ブリッジシステム

#### 1. **Arduino-Windows互換性レイヤー** (`arduino_compat.h/cpp`)
- **目的**: Windows上でArduino APIをエミュレート
- **コンポーネント**:
  - Arduino `String`を置き換える`XenoString`クラス
  - 入出力キュー付きSerialエミュレーション
  - GPIO関数スタブ (`pinMode`, `digitalWrite`など)
  - タイミング関数 (`delay`, `millis`)

#### 2. **言語ソース適応** (`auto_xeno_bridge.py`)
- **目的**: ESP32用XenoコードをWindows互換コードに自動変換
- **機能**:
  - `Arduino.h`インクルードを`arduino_compat.h`に置換
  - マクロ定義を使用して`String`を`XenoString`に変換
  - 関数競合の処理 (例: `isInteger`)
  - ディレクトリツリー全体の処理

#### 3. **Windows-IDE通信レイヤー** (`xeno_host.cpp`)
- **目的**: Xeno IDE向け実行ファイルインターフェースを提供
- **機能**:
  - IDEとの標準入出力プロトコル
  - バックグラウンドVM実行
  - コンパイルとランタイム管理
  - バージョン情報報告

## 📁 ファイル構造
```
XenoLanguageEXE-for-IDE/
├── arduino_compat.h # Arduino APIエミュレーションヘッダー
├── arduino_compat.cpp # Arduino API実装
├── auto_xeno_bridge.py # 自動コード変換スクリプト
├── xeno_host.cpp # メイン実行ファイルソース
├── CMakeLists.txt # ビルド設定
├── version.rc # バージョンリソースファイル
└── Xeno言語メインファイル
```

## 🔧 ソースからのビルド

### 前提条件
- CMake 3.10+
- Visual Studio 2017+ または MinGW-w64
- C++17互換コンパイラ

### ビルド手順
```bash
プロジェクト設定:
cmake -B build -G "Visual Studio 17 2022" -A x64

リリース版ビルド:
cmake --build build --config Release

出力: build/Release/xeno_host.exe
```
## 🔄 Xeno IDEとの統合

Xeno IDEはこのリポジトリのリリースから`xeno_host.exe`を自動的にダウンロードして使用します。統合プロセス:

1. **IDE検出**: IDEが`xeno/xeno_info.txt`でバージョン互換性を確認
2. **通信プロトコル**: IDEが標準入出力で通信
3. **実行フロー**:
   - IDEがソースコード付きで`COMPILE`コマンドを送信
   - `xeno_host.exe`がXeno言語コアを使用してコンパイル
   - IDEが`RUN`コマンドを送信してバックグラウンドスレッドで実行
   - Serialエミュレーションによるリアルタイム入出力

## 🎯 主な機能

### クロスプラットフォーム互換性
- **Arduino APIエミュレーション**: 完全なSerial、String、基本GPIOサポート
- **ESP32からWindowsへ**: プラットフォーム間のシームレスなコード移行
- **リアルタイム実行**: スレッド管理付きバックグラウンドVM

### IDE統合
- **標準プロトコル**: シンプルな行ベース通信
- **バージョン管理**: 自動更新通知
- **エラー処理**: 包括的な例外報告
- **状態管理**: 開始/停止/ステップ実行制御

### パフォーマンスとセキュリティ
- **命令制限**: 設定可能な最大実行ステップ数
- **メモリ安全性**: 境界付き文字列操作とスタック制限
- **プロセス分離**: IDEからの分離されたVMプロセス

## 🔗 関連プロジェクト

- **メインXeno言語** (ESP32): [Xeno-Languageリポジトリ](https://github.com/VLPLAY-Games/Xeno-Language)
  - ESP32マイクロコントローラ向けコア言語実装
  - このWindows適応のソースベース

- **Xeno IDE**: [XenoIDEリポジトリ](https://github.com/VLPLAY-Games/XenoLanguageIDE)
  - Windows統合開発環境
  - この実行ファイルの主要消費者

## 📥 IDEユーザー向けインストール

### 自動インストール (推奨)
1. Xeno IDEを開く
2. Settings → Xenoタブに移動
3. "Check Updates"をクリック
4. バージョンを選択し"Install"をクリック

### 手動インストール
1. Releasesから`xeno_host.exe`をダウンロード
2. IDE相対の`xeno/`ディレクトリに配置
3. 重要: 設定生成のために`xeno_host.exe`を一度実行
4. IDEを再起動

## 🛠️ 開発用途

### 変換スクリプトの実行
Xeno言語ソースファイルの変換:
```
python3 auto_xeno_bridge.py
```

### 含まれるソースファイル
ビルドには必要なXeno言語コアファイルがすべて含まれます:
- xeno_common.cpp - 共通ユーティリティとデータ構造
- xeno_compiler.cpp - コードコンパイルとパーシング
- xeno_vm.cpp - 仮想マシン実行エンジン
- xeno_security.cpp - セキュリティ制限と検証
- XenoLanguage.cpp - メインAPIインターフェース

## 📋 サポートコマンド

| コマンド | パラメータ | 説明 |
|---------|------------|-------------|
| COMPILE | ソースコード + 長さ | Xenoプログラムのコンパイル |
| RUN | なし | バックグラウンド実行 |
| STOP | なし | 実行停止 |
| STEP | なし | 単一命令実行 |
| STDIN <データ> | 入力文字列 | Serialキューへ送信 |
| SET_MAX_INSTRUCTIONS | 数値 | 実行制限の変更 |

## 🔄 バージョン互換性

| Xeno言語 | ブリッジバージョン | IDEバージョン | IDEサポートレベル |
|---------------|----------------|-------------|---------------|
| 0.1.4+ | 0.1.4.1+ | 1.0.0+ | 完全サポート |
| 0.1.3 | 0.1.3.1 | 1.0.0+ | 制限付きサポート |
| < 0.1.3 | - | - | サポート対象外 |

## 🐛 トラブルシューティング

### 一般的な問題

**"xeno_host.exeが見つかりません"**
- 手動インストール手順を実行
- ファイルが`xeno/`ディレクトリにあることを確認
- Windows Defenderがファイルを隔離していないか確認

**手動セットアップ後の"インストールエラー"**
- `xeno_host.exe`を直接一度実行
- `xeno_info.txt`が生成されたことを確認
- ディレクトリ権限を確認

**Serial入力が動作しない**
- コードが`INPUT`プロンプトを待機していることを確認
- Xenoコードで`Serial.readString()`を使用
- 入力送信前にVMが実行中か確認

**ビルド失敗**
- CMakeバージョン3.10+を確認
- Visual Studioインストールを確認
- Windows SDKがインストールされていることを確認

## 📄 ライセンス

このプロジェクトはApache License 2.0でライセンスされています - 詳細は[LICENSE](LICENSE)ファイルを参照してください。

## 🤝 貢献

Windows互換性レイヤーの改善への貢献を歓迎します:

1. Arduino APIカバレッジのギャップを報告
2. Serialエミュレーションのパフォーマンス改善
3. Windows固有の最適化を追加
4. エラー処理と診断の強化

## 📞 サポート

Windows実行ファイルに関する問題について:
1. 言語固有の問題は[メイン言語リポジトリ](https://github.com/VLPLAY-Games/Xeno-Language)を確認
2. Windows互換性の問題はこのリポジトリでissueを作成
3. 詳細なエラーメッセージはIDEコンソールで確認
4. すべてのコンポーネント間のバージョン互換性を確認
# 作業記録 (Claude Code)

Tscheme (Scheme サブセットインタプリタ) に対する 64bit 対応とセキュリティ修正の記録。

## 概要

32bit 前提だったコードの 64bit 化と、その過程および `read`/`load` で
外部 Scheme ソースを処理する際に顕在化するメモリ安全性の問題を修正した。
各修正は `-std=c99 -pedantic -Wall -Werror` でのクリーンビルドと、
`init.scm` の byte-identical なセルフブートストラップで検証済み。
メモリ安全性は ASan/UBSan ビルドおよび `fclose` 横取りシムで確認した。

## 実施した修正

### 1. 64bit アーキテクチャ対応 (47bc13b, 2026-09-22)
- ポインタ/fixnum タグ付けを `uintptr_t`/`intptr_t` ベースに書き換え
  (`EQ`, `IMM_TYPE`, `IS_FIXNUM`, `MK_FIXNUM`, `FIXNUM` — tscheme.h)。
  fixnum ペイロードは上位 62bit を自然に使うようになった。
- 32bit int 前提だった printf/sprintf の書式を修正 (io.c, subrs.c)。
- Makefile の強制 `-m32` を除去。`-Werror` の既存警告 2 件を解消 (storage.c)。

### 2. 数値リテラル/`string->number` のオーバーフロー (a61d624, 2026-09-22)
- fixnum は 62bit に拡張されたのに、読み込み側が `atoi` (32bit int) のまま
  だった。32bit を超える値が無言で壊れる。
- `read.c` と `subrs.c` の `atoi` を `strtol` に置換し、`ERANGE` を
  明示的にエラー報告するようにした。

### 3. 文字列リーダ/シンボルハッシュのバッファオーバーフロー (65f4e55, 2026-10-10)
- **read.c `do_readstring`**: グローバル `strbuf` に境界チェックなしで
  書き込んでおり、`STRBUF_SIZE` 超の文字列リテラルで溢れていた
  (`do_readtoken` にあるガードが欠落)。同じガードを追加。
- **object.c `mk_symbol`**: ハッシュを符号付き `char`/`long` で計算して
  いたため、非ASCII (高位ビットの立つ) バイトで負のハッシュとなり
  `obarray` を配列外参照していた。`unsigned char`/`unsigned long` に変更。

### 4. ポート二重 fclose / `sys:eval` 型混同 (aa934de, 2026-10-10)
- **二重 fclose (io.c, storage.c)**: `close-input-port`/`close-output-port`
  で閉じたポートセルが GC 対象になると、スイープが再度 `fclose` を呼び
  解放済み `FILE` を操作していた。close 時に `PORT_FPTR` を `NULL` にし、
  close プリミティブと `gc_sweep` の両方で `NULL` なら `fclose` しない。
- **`sys:eval` 型混同 (subrs.c)**: `SYS:EVAL` が `evaluate` に直結しており
  第2引数 (環境) を無検証で alist として walk していた。非リスト
  (例: fixnum) を渡すとタグ付き値を構造体ポインタとして参照した。
  環境が `'()` か pair であることを検証するラッパー `s_eval` を経由させた。

## 検証方法メモ
- 通常ビルド: `make`（`-Werror` 通過 + `init.scm` と `init1.scm` が一致）。
- メモリ安全性: `-fsanitize=address,undefined` でビルド。保守的 GC の
  スタック走査 (storage.c `gc_mark_locations_array`) は ASan が
  stack-buffer-overflow を報告するが、これは設計上意図された生メモリ走査
  による誤検知。必要に応じて storage.c のみ非計装でビルドして回避する。
- 二重 fclose: `dlsym(RTLD_NEXT,"fclose")` による横取りシムで検出。

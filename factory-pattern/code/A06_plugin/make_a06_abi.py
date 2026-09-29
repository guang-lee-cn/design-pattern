#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""A06 · ABI 字段错位的动图：按宿主的字段表逐格往下扫，扫到 0x10 撞上错位。

产出：docs/assets/A06_abi_scan.gif

📌 与静态图 A06_abi_layout.svg 的分工 —— 这是两张不同的图，不是同一份内容的两种形态：
   - A06_abi_layout.svg  并置对照：宿主 32 / 插件 40 两份布局摆在一起，看「差在哪」
   - A06_abi_scan.gif    过程推进：偏移刻度一格格往下走，看「怎么走到那里的」
   两者配色共用下面这套常量；改一张，另一张跟着核。

路径：SVG（本脚本生成）-> rsvg-convert 栅格化 -> Pillow 合成 GIF
用法：python3 make_a06_abi.py

⚠️ GIF 帧在本机栅格化 -> 字体必须写本机已有的 CJK 字体。
   本机 sans-serif 默认落在 DejaVu（无中文），不指定就是豆腐块。
"""
import os
import subprocess
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ASSETS = os.path.join(ROOT, 'docs', 'assets')
TMP = '/tmp/fpa06gif'
SCALE = 2

CJK_FONT = "'MiSans','Noto Sans CJK SC','Droid Sans Fallback',sans-serif"

INK, GREY, MUTED = '#1f2937', '#5b6472', '#8a929c'
PANEL, EDGE, WHITE = '#eef2f7', '#c9d4e2', '#ffffff'
BLUE = ('#e8f0fe', '#c7d7f5', '#0c447c')
RED = ('#fdf2f2', '#e8c4c4', '#9b2c2c')
GRN = ('#eef7f0', '#c2ddca', '#276749')

# 偏移刻度：四格。扫描头停在哪一格由 TAKES[*]['seg'] 指定
SEGS = ['0x00', '0x08', '0x10', '0x18']
SEGX0, SEGW, SEGY, SEGH = 44, 148, 62, 26

CARDY, CARDH, CARDW = 104, 92, 252
XL, XR = 44, 384
FW, FH = 680, 304

# ---- 五帧：扫描从 0x00 走到 0x10，然后崩，最后给出拦住它的机制 ----
TAKES = [
    dict(seg=0, seg_kind=BLUE, sub='版本号相等，看不出问题',
         head='0x00　两边都写 3',
         ltitle='宿主侧 · 它按这个读', ll=[('abi_version = 3', True), ('struct_size = 32', False)],
         rtitle='插件侧 · 它实际是', rl=[('abi_version = 3', True), ('struct_size = 40', False)],
         mark=('=', '相等，通过', GRN),
         bottom=(PANEL, EDGE, MUTED,
                 'version 相同只证明「两边的人记得改同一个数字」。',
                 'struct_size 就摆在旁边（32 ↔ 40），但没人拿它做判断。')),

    dict(seg=1, seg_kind=BLUE, sub='到这里还是对的',
         head='0x08　恰好对齐',
         ltitle='宿主侧 · 它按这个读', ll=[('name', True), ('指针 8 字节', False)],
         rtitle='插件侧 · 它实际是', rl=[('name', True), ('指针 8 字节', False)],
         mark=('=', '相等，继续', GRN),
         bottom=(PANEL, EDGE, MUTED,
                 '前面两个字段尺寸恰好相同，扫描一路绿灯。',
                 '「字段表的前缀一致」不等于「整张表一致」—— 问题在后面，不在这里。')),

    dict(seg=2, seg_kind=RED, sub='危险从这里开始',
         head='0x10　错位点',
         ltitle='宿主侧 · 它以为这里是', ll=[('read', True), ('函数指针 8 字节', False)],
         rtitle='插件侧 · 实际放的是', rl=[('sample_rate_hz = 100', True), ('4 字节 + 4 字节填充', False)],
         mark=('≠', '不是一回事', RED),
         bottom=(RED[0], RED[1], RED[2],
                 '插件在 name 后面插了一个字段，从 0x10 起整张表错开一格。',
                 '插件的 read 已经退到 0x18 —— 宿主还在 0x10 找它。')),

    dict(seg=2, seg_kind=RED, sub='跳到地址 100',
         head='0x10　那一格被拿去调用',
         ltitle='宿主侧 · 它做了什么', ll=[('取出 0x10 的 8 字节', True), ('当作函数指针 call', False)],
         rtitle='那 8 字节的真相', rl=[('sample_rate_hz', True), ('= 100 ／ 十六进制 0x64', False)],
         mark=('call 100', '跳到地址 100', RED),
         bottom=(RED[0], RED[1], RED[2],
                 '它跳到地址 100 —— 一个合法地址，链路上一路无人报警。实测 SIGSEGV，退出码 139。',
                 '崩溃点在 0x10，错处也在 0x10，但「发现它」花了整整一帧的时间。')),

    dict(seg=2, seg_kind=GRN, sub='机制能拦住',
         head='0x10　若先比过 struct_size',
         ltitle='校验 struct_size', ll=[('宿主 40 ≠ 32', True), ('拒绝加载，退出码 0', False)],
         rtitle='只校验 version', rl=[('两边都是 3', True), ('一路放行 → 崩', False)],
         mark=('⊘', '在 0x10 之前拦下', GRN),
         bottom=(GRN[0], GRN[1], GRN[2],
                 '同一份代码，多比一个 struct_size，就在进入 0x10 之前退出，而不是走到 call。',
                 'version 是约定（靠人记得改），struct_size 是机制（不靠人记）—— 要一起用。')),
]


def card(x, title, lines, kind):
    bg, ed, tx = kind
    out = ['<rect x="%d" y="%d" width="%d" height="%d" rx="6" fill="%s" stroke="%s" '
           'stroke-width="1.2"/>' % (x, CARDY, CARDW, CARDH, bg, ed)]
    out.append('<text x="%d" y="%d" font-size="10.5" fill="%s">%s</text>'
               % (x + 16, CARDY + 24, MUTED, title))
    for k, (txt, bold) in enumerate(lines):
        out.append('<text x="%d" y="%d" font-size="%s" font-weight="%s" fill="%s">%s</text>'
                   % (x + 16, CARDY + 52 + k * 22, 13.5 if bold else 12,
                      700 if bold else 400, tx, txt))
    return '\n  '.join(out)


def frame_svg(t):
    # 偏移刻度条：当前格高亮
    bars = []
    for k, lab in enumerate(SEGS):
        x = SEGX0 + k * SEGW
        active = (k == t['seg'])
        bg, ed, tx = t['seg_kind'] if active else (PANEL, EDGE, MUTED)
        bars.append('<rect x="%d" y="%d" width="%d" height="%d" rx="4" fill="%s" stroke="%s" '
                    'stroke-width="%s"/>' % (x, SEGY, SEGW - 8, SEGH, bg, ed, 1.8 if active else 1))
        bars.append('<text x="%d" y="%d" font-size="11.5" font-weight="%s" fill="%s" '
                    'text-anchor="middle">%s</text>'
                    % (x + (SEGW - 8) // 2, SEGY + 18, 700 if active else 400, tx, lab))
    # 扫描头：当前格下方一个小三角
    ax = SEGX0 + t['seg'] * SEGW + (SEGW - 8) // 2
    bars.append('<path d="M %d %d L %d %d L %d %d z" fill="%s"/>'
                % (ax, SEGY + SEGH + 4, ax - 6, SEGY + SEGH - 2, ax + 6, SEGY + SEGH - 2,
                   t['seg_kind'][1]))
    bars_svg = '\n  '.join(bars)

    bbg, bed, btx = t['bottom'][:3]
    mbg, med, mtx = t['mark'][2]

    parts = [
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d" '
        'font-family="%s">' % (FW, FH, FW, FH, CJK_FONT),
        '  <rect width="%d" height="%d" fill="%s"/>' % (FW, FH, WHITE),
        '',
        '  <text x="36" y="32" font-size="15" font-weight="700" fill="%s">%s</text>' % (INK, t['head']),
        '  <text x="644" y="32" font-size="11.5" fill="%s" text-anchor="end">%s</text>' % (MUTED, t['sub']),
        '',
        '  ' + bars_svg,
        '',
        '  ' + card(XL, t['ltitle'], t['ll'], t['seg_kind']),
        '  ' + card(XR, t['rtitle'], t['rl'], t['seg_kind']),
        '',
        '  <text x="340" y="%d" font-size="14" font-weight="700" fill="%s" text-anchor="middle">%s</text>'
        % (CARDY + 48, mtx, t['mark'][0]),
        '  <text x="340" y="%d" font-size="9.5" fill="%s" text-anchor="middle">%s</text>'
        % (CARDY + 70, mtx, t['mark'][1]),
        '',
        '  <rect x="44" y="216" width="592" height="68" rx="6" fill="%s" stroke="%s" stroke-width="1"/>'
        % (bbg, bed),
        '  <text x="60" y="240" font-size="11.5" fill="%s">%s</text>' % (btx, t['bottom'][3]),
        '  <text x="60" y="262" font-size="11.5" fill="%s">%s</text>' % (btx, t['bottom'][4]),
        '</svg>',
        '',
    ]
    return '\n'.join(parts)


def main():
    os.makedirs(TMP, exist_ok=True)
    pngs = []
    for i, t in enumerate(TAKES):
        s = os.path.join(TMP, 'frame_%02d.svg' % i)
        p = os.path.join(TMP, 'frame_%02d.png' % i)
        open(s, 'w', encoding='utf-8').write(frame_svg(t))
        subprocess.run(['rsvg-convert', '-z', str(SCALE), s, '-o', p], check=True)
        pngs.append(p)
    imgs = [Image.open(p).convert('RGB') for p in pngs]
    frames = [im.convert('P', palette=Image.ADAPTIVE, colors=128) for im in imgs]
    gp = os.path.join(ASSETS, 'A06_abi_scan.gif')
    frames[0].save(gp, save_all=True, append_images=frames[1:],
                   duration=[1400, 1100, 2000, 2000, 2600], loop=0, optimize=True, disposal=2)
    print('GIF: %s (%.1f KB, %d 帧, %dx%d)' % (gp, os.path.getsize(gp) / 1024,
                                              len(frames), FW * SCALE, FH * SCALE))


if __name__ == '__main__':
    main()

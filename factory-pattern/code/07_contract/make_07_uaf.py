#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""07 悬垂指针时序的两种形态，单一来源产出：

  docs/assets/07_uaf_timeline.svg   四拍纵向时序，一屏看全（矢量，静态）
  docs/assets/07_uaf_timeline.gif   四拍逐帧播放，6.3 秒循环（位图，动态）

路径：SVG（本脚本生成）-> rsvg-convert 栅格化 -> Pillow 合成 GIF。
用法：python3 make_07_uaf.py

⚠️ 两种交付物的字体栈不同，不能混：
  - 静态 SVG 是交付物，在读者机器上渲染 -> 必须写 Web 安全栈（回退由读者系统决定）
  - GIF 帧在本机栅格化 -> 必须写本机已有的 CJK 字体，否则中文变豆腐块
"""
import os
import subprocess
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ASSETS = os.path.join(ROOT, 'docs', 'assets')
TMP = '/tmp/fp07gif'
SCALE = 2

WEB_FONT = "ui-sans-serif, system-ui, -apple-system, 'Segoe UI', sans-serif"
MONO_WEB = "ui-monospace, Menlo, monospace"
CJK_FONT = "'MiSans','Noto Sans CJK SC','Droid Sans Fallback',sans-serif"

INK, GREY, MUTED = '#1f2937', '#5b6472', '#8a929c'
PANEL, EDGE, WHITE = '#eef2f7', '#c9d4e2', '#ffffff'
BLUE = ('#e8f0fe', '#c7d7f5', '#0c447c')   # bg, edge, text
RED = ('#fdf2f2', '#e8c4c4', '#9b2c2c')
GRN = ('#eef7f0', '#c2ddca', '#276749')

# ---- 四拍：静态与动图共用同一份文案 ----
TAKES = [
    dict(t='t1', kind=BLUE, dark=False, sub='create() 那一刻',
         head='工厂创建并缓存对象',
         obj=['Sensor 对象', 'name = "temp"', '状态：存活'],
         cache=['缓存表', 'temp → 0x6010', '（工厂存下的）'],
         right=['工厂', 'create().release()', '把地址交出去'],
         asan=('（尚未触发）', '机制：调用方与工厂谁都不知道对方删了没有'), asan_red=False,
         desc='工厂 create() 交出一个地址，同时把地址存进缓存',
         chip='内存：存活', link_red=False),
    dict(t='t2', kind=RED, dark=True, sub='危险开始的地方',
         head='调用方 delete —— 工厂不知道',
         obj=['该内存已释放', 'name = 垃圾值', 'ASan 标为已中毒'],
         cache=['缓存表', 'temp → 0x6010', '（一个字节没改）'],
         right=['调用方', 'delete s', '以为到此为止'],
         asan=('（尚未触发）', '机制：调用方与工厂谁都不知道对方删了没有'), asan_red=False,
         desc='调用方 delete s —— 内存没了，缓存里那个地址一个字节没改',
         chip='内存：已释放', link_red=False),
    dict(t='t3', kind=RED, dark=True, sub='指针值没变，目标没了',
         head='缓存的指针失效 —— 悬垂',
         obj=['该内存已释放', 'name = 垃圾值', 'ASan 标为已中毒'],
         cache=['缓存表', 'temp → 0x6010', '← 悬垂指针'],
         right=['调用方', '再次 get("temp")', '拿到同一地址'],
         asan=('（尚未触发）', '机制：调用方与工厂谁都不知道对方删了没有'), asan_red=False,
         desc='再次 get("temp") —— 从缓存拿出那个地址，它已经指向一片死内存',
         chip='缓存指针：悬垂', link_red=True),
    dict(t='t4', kind=RED, dark=True, sub='调用方毫无察觉',
         head='读已释放内存 —— 崩在这里',
         obj=['该内存已释放', 'read() 访问 name', '→ 读已释放内存'],
         cache=['缓存表', 'temp → 0x6010', '← 悬垂指针'],
         right=['调用方', 's->read()', '和正常调用一样'],
         asan=('AddressSanitizer：heap-use-after-free',
               'READ of size 8 —— basic_string::size() ← Sensor::read() ← main　（退出码 1）'),
         asan_red=True,
         desc='s->read() 访问 name —— 读已释放内存，当场被拦下',
         chip='ASan：use-after-free', link_red=True),
]

# ============ 形态 A：静态 SVG（四拍纵向时序，一屏看全）============
Y0, RH, GAP = 76, 60, 10


def static_svg():
    body = []
    for i, t in enumerate(TAKES):
        bg, ed, tx = t['kind']
        y = Y0 + i * (RH + GAP)
        body.append('<rect x="24" y="%d" width="632" height="%d" rx="6" fill="%s" stroke="%s"/>'
                    % (y, RH, bg, ed))
        body.append('<text x="44" y="%d" font-size="13" font-weight="700" fill="%s" '
                    'font-family="%s">%s</text>' % (y + 38, tx, MONO_WEB, t['t']))
        body.append('<text x="96" y="%d" font-size="12" fill="%s">%s</text>'
                    % (y + 37, INK, t['desc']))
        body.append('<text x="636" y="%d" font-size="10.5" fill="%s" text-anchor="end">%s</text>'
                    % (y + 38, tx, t['chip']))
        if i < len(TAKES) - 1:
            ay = y + RH
            body.append('<path d="M 340 %d L 340 %d" stroke="%s" stroke-width="1.3"/>'
                        % (ay + 2, ay + GAP - 2, EDGE))
            body.append('<path d="M 340 %d L 336 %d L 344 %d z" fill="%s"/>'
                        % (ay + GAP - 1, ay + GAP - 6, ay + GAP - 6, EDGE))
    ay = Y0 + 4 * RH + 3 * GAP + 16
    abg, aed, atx = RED if True else PANEL
    body.append('<rect x="24" y="%d" width="632" height="62" rx="6" fill="%s" stroke="%s"/>'
                % (ay, abg, aed))
    body.append('<text x="44" y="%d" font-size="12" font-weight="700" fill="%s">%s</text>'
                % (ay + 26, atx, TAKES[-1]['asan'][0]))
    body.append('<text x="44" y="%d" font-size="11.5" fill="%s">%s</text>'
                % (ay + 48, atx, TAKES[-1]['asan'][1]))
    h = ay + 62 + 24

    return ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d" '
            'font-family="%s">\n  <rect width="%d" height="%d" fill="%s"/>\n\n'
            '  <text x="24" y="30" font-size="16" font-weight="700" fill="%s">'
            '缓存工厂的悬垂指针：四拍</text>\n'
            '  <text x="24" y="52" font-size="12" fill="%s">'
            '同一个地址，工厂和调用方各以为对方管着它 —— 动图版见 '
            '07_uaf_timeline.gif</text>\n\n  %s\n\n</svg>\n'
            % (680, h, 680, h, WEB_FONT, 680, h, WHITE, INK, GREY, '\n  '.join(body)))


# ============ 形态 B：GIF 帧（每拍一屏，突出变化）============
FW, FH = 680, 290


def _panel(x, w, kind, dash, lines):
    bg, ed, tx = kind
    d = ' stroke-dasharray="5 4"' if dash else ''
    out = ['<rect x="%d" y="64" width="%d" height="120" rx="6" fill="%s" stroke="%s" '
           'stroke-width="1.2"%s/>' % (x, w, bg, ed, d)]
    for i, (txt, col, bold) in enumerate(lines):
        fw = ' font-weight="700"' if bold else ''
        out.append('<text x="%d" y="%d" font-size="12" fill="%s"%s>%s</text>'
                   % (x + 16, 92 + i * 24, col, fw, txt))
    return '\n  '.join(out)


def frame_svg(t):
    dead = ('（尚未触发）', '机制：调用方与工厂谁都不知道对方删了没有')
    asan = t['asan'] if t['asan_red'] else dead
    abg, aed, atx = RED if t['asan_red'] else (PANEL, EDGE, MUTED)
    lc = RED[1] if t['link_red'] else EDGE
    ac = RED[2] if t['link_red'] else MUTED
    obj_dark = t['t'] != 't1'
    obj_kind = RED if obj_dark else BLUE
    cache_kind_text = RED[2] if obj_dark else BLUE[2]

    obj_lines = [(t['obj'][0], obj_kind[2], True),
                 (t['obj'][1], obj_kind[2], False),
                 (t['obj'][2], GRN[2] if t['t'] == 't1' else obj_kind[2], False)]
    cache_lines = [(t['cache'][0], MUTED, False),
                   (t['cache'][1], cache_kind_text, obj_dark),
                   (t['cache'][2], cache_kind_text if obj_dark else MUTED, obj_dark and t['t'] == 't3')]

    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {FW} {FH}" width="{FW}" height="{FH}" font-family="{CJK_FONT}">
  <rect width="{FW}" height="{FH}" fill="{WHITE}"/>

  <text x="36" y="30" font-size="15" font-weight="700" fill="{INK}">{t['t']}　{t['head']}</text>
  <text x="644" y="30" font-size="11.5" fill="{MUTED}" text-anchor="end">{t['sub']}</text>

  {_panel(36, 190, obj_kind, obj_dark, obj_lines)}

  <path d="M 232 124 L 278 124" stroke="{lc}" stroke-width="1.6"/>
  <path d="M 286 124 L 274 119 L 274 129 z" fill="{lc}"/>
  <text x="258" y="112" font-size="9.5" fill="{ac}" text-anchor="middle">指针</text>

  {_panel(286, 140, (PANEL, EDGE, INK), False, cache_lines)}

  <path d="M 432 124 L 472 124" stroke="{EDGE}" stroke-width="1.6"/>
  <path d="M 480 124 L 468 119 L 468 129 z" fill="{EDGE}"/>

  {_panel(480, 164, (PANEL, EDGE, INK), False,
          [(t['right'][0], MUTED, False), (t['right'][1], INK, True), (t['right'][2], MUTED, False)])}

  <rect x="36" y="204" width="608" height="66" rx="6" fill="{abg}" stroke="{aed}" stroke-width="1"/>
  <text x="52" y="228" font-size="12" font-weight="700" fill="{atx}">{asan[0]}</text>
  <text x="52" y="250" font-size="11.5" fill="{atx}">{asan[1]}</text>
</svg>
'''


def main():
    sp = os.path.join(ASSETS, '07_uaf_timeline.svg')
    open(sp, 'w', encoding='utf-8').write(static_svg())
    print('静态 SVG: %s (%d B)' % (sp, os.path.getsize(sp)))

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
    gp = os.path.join(ASSETS, '07_uaf_timeline.gif')
    frames[0].save(gp, save_all=True, append_images=frames[1:],
                   duration=[1200, 1200, 1300, 2600], loop=0, optimize=True, disposal=2)
    print('GIF: %s (%.1f KB, %d 帧)' % (gp, os.path.getsize(gp) / 1024, len(frames)))


if __name__ == '__main__':
    main()

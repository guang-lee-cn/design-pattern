#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""06 · 分派链逐段推进的动图：五条链并排，一段一段点亮。

产出：docs/assets/06_dispatch_steps.gif

📌 与静态图 06_dispatch_chain.svg 的分工 —— 两张不同的图，不是同一份内容的两种形态：
   - 06_dispatch_chain.svg   并置对照：五个方案的环节 / 间接跳转 / 耗时排成一张表，看「差多少」
   - 06_dispatch_steps.gif   过程推进：五条链同时一段段往前走，看「差在哪一段」
   数据同源（`code/06_cost/bench_dispatch.cpp` 实测），配色共用下面这套常量。

⚠️ **推进的是「段」（链上的环节），不是「环」（间接跳转次数）** —— 两者别混：
   简单工厂有 2 段（strcmp → read()），但只有 1 次间接跳转（strcmp 是直接调用）。
   动图第 4 帧专门把这一点标出来。

用法：python3 make_06_dispatch.py
"""
import os
import subprocess
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
ASSETS = os.path.join(ROOT, 'docs', 'assets')
TMP = '/tmp/fp06steps'
SCALE = 2

CJK_FONT = "'MiSans','Noto Sans CJK SC','Droid Sans Fallback',sans-serif"

INK, GREY, MUTED = '#1f2937', '#5b6472', '#8a929c'
PANEL, EDGE, WHITE = '#eef2f7', '#c9d4e2', '#ffffff'
FAINT, FAINT_EDGE = '#f6f7f9', '#dfe4ea'
RED = ('#fde8e8', '#e57373', '#b3261e')

NODE = {
    'grey':  ('#eef2f7', '#c9d4e2', '#1f2937'),
    'blue':  ('#e8f0fe', '#c7d7f5', '#1f2937'),
    'red':   ('#fde8e8', '#e57373', '#b3261e'),
    'green': ('#e8f5e9', '#a5d6a7', '#1b5e20'),
}
DOT = {                      # 间接跳转点阵的配色
    'base': ('#f0a04b', '#d98428'),
    'mid':  ('#e2703a', '#c4561f'),
    'hot':  ('#b3261e', '#8f1d17'),
    'good': None,            # variant：零间接，点阵全空
}
NS_COLOR = {'base': INK, 'mid': INK, 'hot': '#b3261e', 'good': '#1b5e20'}

# ---- 五个方案：节点（段）、间接跳转次数、实测耗时（bench_dispatch.cpp）----
ROWS = [
    dict(name='0 无工厂（基线）', note='直接拿指针就调', kind='base', ind=1,
         nodes=[('p->read()', 'grey')],            ns='0.822 ns', rel='1.00x'),
    dict(name='1 简单工厂', note='一次字符串比较', kind='base', ind=1,
         nodes=[('strcmp', 'blue'), ('read()', 'grey')],  ns='1.005 ns', rel='1.22x'),
    dict(name='2 工厂方法', note='虚调用套虚调用', kind='mid', ind=2,
         nodes=[('create()', 'blue'), ('read()', 'grey')], ns='1.465 ns', rel='1.78x'),
    dict(name='3 注册表', note='最贵，却常被当成轻写法', kind='hot', ind=3,
         nodes=[('map.find', 'red'), ('function', 'red'), ('read()', 'red')],
         ns='7.592 ns', rel='9.23x'),
    dict(name='4 variant', note='零间接，比基线还快', kind='good', ind=0,
         nodes=[('visit 在编译期展开 → read() 直接内联', 'green')],
         ns='0.339 ns', rel='0.41x'),
]

# ---- 五帧：前三帧逐段推进，第四帧走完，第五帧亮代价 ----
FRAMES = [
    dict(step=1, sub='第 1 段 · 最长链 3 段',
         l1='第 1 段 —— 所有方案都得先分派一次，链的差别要往后看。',
         l2='本帧高亮的，就是刚走过去的那一段。'),
    dict(step=2, sub='第 2 段 · 最长链 3 段',
         l1='第 2 段 —— 基线只有一格，已经到函数体了；注册表才走到一半。',
         l2='简单工厂这里多一次 string 比较，但它不产生间接跳转，所以仍只算 1 次。'),
    dict(step=3, sub='第 3 段 · 最长链 3 段',
         l1='第 3 段 —— 只剩注册表还在走：map.find → std::function → read()。',
         l2='五条链里，只有它要走满三段。'),
    dict(step=9, sub='三段走完',
         l1='走完对比：注册表 3 段 · 工厂方法 2 段 · 基线 1 段；variant 也只有 1 段 —— 它在编译期就走完了。',
         l2='⚠ 「段数」和「间接跳转次数」不是一回事 —— 简单工厂 2 段，但只有 1 次间接。'),
    dict(step=9, sub='代价亮出来', show_ns=True,
         l1='注册表的 9.23x，量的不是「多了一层函数」，而是每一层间接都要读一次内存取地址。',
         l2='换成 enum 键能省掉树查找，但省不掉类型擦除那一层 —— 要连根去掉，只有 variant。'),
]

FW, FH = 680, 386
CY0, ROWH = 108, 44
GRID_X0, GRID_W, GRID_GAP = 150, 104, 12
TOTAL_W = GRID_W * 3 + GRID_GAP * 2          # 336
DOT_X0, DOT_W, DOT_GAP = 490, 18, 4
NS_X = 656


def frame_svg(t):
    step = t['step']
    show_ns = t.get('show_ns', False)
    p = []
    p.append('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d" '
             'font-family="%s">' % (FW, FH, FW, FH, CJK_FONT))
    p.append('  <rect width="%d" height="%d" fill="%s"/>' % (FW, FH, WHITE))
    p.append('')
    p.append('  <text x="24" y="28" font-size="15" font-weight="700" fill="%s">'
             '同样一次 read()，五条链各要走几段</text>' % INK)
    p.append('  <text x="%d" y="28" font-size="11.5" fill="%s" text-anchor="end">%s</text>'
             % (NS_X, MUTED, t['sub']))
    p.append('  <text x="24" y="52" font-size="11.5" fill="%s">'
             '段数看得见，代价才看得懂 —— 每一段间接都要读一次内存取地址。</text>' % GREY)
    p.append('')
    # 表头
    p.append('  <text x="24" y="76" font-size="11" font-weight="700" fill="%s">方案</text>' % MUTED)
    p.append('  <text x="%d" y="76" font-size="11" font-weight="700" fill="%s">链上的环节</text>'
             % (GRID_X0, MUTED))
    p.append('  <text x="%d" y="76" font-size="11" font-weight="700" fill="%s">间接跳转</text>'
             % (DOT_X0, MUTED))
    p.append('  <text x="%d" y="76" font-size="11" font-weight="700" fill="%s" '
             'text-anchor="end">耗时 · 相对基线</text>' % (NS_X, MUTED))
    p.append('  <line x1="24" y1="84" x2="%d" y2="84" stroke="#e3e8ef"/>' % NS_X)
    p.append('')

    for i, r in enumerate(ROWS):
        cy = CY0 + i * ROWH
        lit = min(step, len(r['nodes']))

        # 注册表整行淡红底，与静态图一致
        if r['kind'] == 'hot':
            p.append('  <rect x="16" y="%d" width="%d" height="%d" rx="6" fill="#fdf6f6"/>'
                     % (cy - 20, NS_X - 16 + 20, ROWH - 4))
        # 方案名 + 备注
        p.append('  <text x="24" y="%d" font-size="11.5" font-weight="700" fill="%s">%s</text>'
                 % (cy - 2, NS_COLOR[r['kind']], r['name']))
        p.append('  <text x="24" y="%d" font-size="10" fill="%s">%s</text>'
                 % (cy + 13, MUTED, r['note']))

        # 节点框：未点亮 = 灰虚线，点亮 = 本色；本帧新亮 = 粗边框
        multi = len(r['nodes']) > 1
        for j, (txt, kind) in enumerate(r['nodes']):
            x = GRID_X0 + j * (GRID_W + GRID_GAP)
            w = GRID_W if multi else TOTAL_W
            on = j < lit
            newest = on and (j == lit - 1) and step <= len(r['nodes'])
            if on:
                bg, ed, tx = NODE[kind]
                sw = 2.4 if newest else 1.2
            else:
                bg, ed, tx, sw = FAINT, FAINT_EDGE, MUTED, 1
            dash = '' if on else ' stroke-dasharray="4 3"'
            p.append('  <rect x="%d" y="%d" width="%d" height="28" rx="4" fill="%s" stroke="%s" '
                     'stroke-width="%s"%s/>' % (x, cy - 14, w, bg, ed, sw, dash))
            p.append('  <text x="%d" y="%d" font-size="11.5" fill="%s" text-anchor="middle">%s</text>'
                     % (x + w // 2, cy + 4, tx, txt))

        # 间接跳转点阵
        for j in range(3):
            dx = DOT_X0 + j * (DOT_W + DOT_GAP)
            if r['kind'] == 'good' or j >= r['ind']:
                p.append('  <rect x="%d" y="%d" width="%d" height="18" rx="3" fill="%s" '
                         'stroke="%s" stroke-dasharray="3 2"/>'
                         % (dx, cy - 9, DOT_W, FAINT, FAINT_EDGE))
            else:
                db, de = DOT[r['kind']]
                p.append('  <rect x="%d" y="%d" width="%d" height="18" rx="3" fill="%s" stroke="%s"/>'
                         % (dx, cy - 9, DOT_W, db, de))

        # 耗时：只有末帧亮出来
        if show_ns:
            p.append('  <text x="%d" y="%d" font-size="11.5" font-weight="700" fill="%s" '
                     'text-anchor="end">%s</text>' % (NS_X, cy + 4, NS_COLOR[r['kind']], r['ns']))
            p.append('  <text x="%d" y="%d" font-size="10.5" fill="%s" text-anchor="end">%s</text>'
                     % (NS_X, cy + 19, MUTED, r['rel']))
        else:
            p.append('  <text x="%d" y="%d" font-size="11.5" fill="%s" text-anchor="end">—</text>'
                     % (NS_X, cy + 4, FAINT_EDGE))

    # 底部结论条
    bbg, bed, btx = RED if show_ns else (PANEL, EDGE, INK)
    p.append('')
    p.append('  <rect x="24" y="310" width="632" height="56" rx="6" fill="%s" stroke="%s"/>'
             % (bbg, bed))
    p.append('  <text x="40" y="334" font-size="11.5" fill="%s">%s</text>' % (btx, t['l1']))
    p.append('  <text x="40" y="354" font-size="11.5" fill="%s">%s</text>' % (btx, t['l2']))
    p.append('</svg>')
    p.append('')
    return '\n'.join(p)


def main():
    os.makedirs(TMP, exist_ok=True)
    pngs = []
    for i, t in enumerate(FRAMES):
        s = os.path.join(TMP, 'frame_%02d.svg' % i)
        png = os.path.join(TMP, 'frame_%02d.png' % i)
        open(s, 'w', encoding='utf-8').write(frame_svg(t))
        subprocess.run(['rsvg-convert', '-z', str(SCALE), s, '-o', png], check=True)
        pngs.append(png)
    imgs = [Image.open(p).convert('RGB') for p in pngs]
    frames = [im.convert('P', palette=Image.ADAPTIVE, colors=128) for im in imgs]
    gp = os.path.join(ASSETS, '06_dispatch_steps.gif')
    frames[0].save(gp, save_all=True, append_images=frames[1:],
                   duration=[1500, 1700, 1700, 2100, 2700], loop=0, optimize=True, disposal=2)
    print('GIF: %s (%.1f KB, %d 帧, %dx%d)' % (gp, os.path.getsize(gp) / 1024,
                                              len(frames), FW * SCALE, FH * SCALE))


if __name__ == '__main__':
    main()

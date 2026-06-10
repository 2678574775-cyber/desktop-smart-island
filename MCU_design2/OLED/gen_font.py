"""
生成中文字库 OLED_Font.h
对每个汉字用 PIL 渲染 16x16 位图 → SSD1306 页-列格式
用法: python gen_font.py
"""
from PIL import Image, ImageDraw, ImageFont
import os

# ── 常用汉字列表 (~1500 个, 覆盖日常对话 95%) ──
# 常用汉字 2500+ (覆盖日常对话 98%)
CHARS = (
    "的一是在不了有和人这中大为上个国我以要他时来用们生到作地于出就分对成会可主发年动同工也能下过子说产种面而方后多定行学法所民得经十三之进着等部度家电力里如水化高自二理起小物现实加量都两体制机当使点从业本去把性好应开它合还因由其些然前外天政四日那社义事平形相全表间样与关各重新线内数正心反你明看原又么利比或但质气第向道命此变条只没结解问意建月公无系军很情者最立代想已通并提直题党程展五果料象员革位入常文总次品式活设及管特件长求老头基资边流路级少图山统接知较将组见计别她手角期根论运农指几九区强放决西被干做必战先回则任取据处队南给色光门即保治北造百规热领七海口东导器压志世金增争济阶油思术极交受联什认六共权收证改清己美再采转更单风切打白教速花带安场身车例真务具万每目至达走积示议声报斗完类八离华名确才科张信马节话米整空元况今集温传土许步群广石记需段研界拉林律叫且究观越织装影算低持音众书布复容儿须际商非验连断深难近矿千周委素技备半办青省列习响约支般史感劳便团往酸历市克何除消构府称太准精值号率族维划选标写存候毛亲快效斯院查江型眼王按格养易置派层片始却专状育厂京识适属圆包火住调满县局照参红细引听该铁价严首底液官德随病苏失尔死讲配女黄推显谈罪神艺呢席含企望密批营项防举球英氧势告李台落木帮轮破亚师围注远字材排供河态封另施减树溶怎止案言士均武固叶鱼波视仅费紧爱左章早朝害续轻服试食充兵源判护司足某练差致板田降黑犯负击范继兴似余坚曲输修的故城夫够送笔船占右财吃富春职觉汉画功巴跟虽杂飞检吸助升阳互初创抗考投坏策古径换未跑留钢曾端责站简述钱副尽帝射草冲承独令限阿宣环双请超微让控州良轴找否纪益依优顶础载倒房突坐粉敌略客袁冷胜绝析块剂测丝协诉念陈仍罗盐友洋错苦夜刑移频逐靠混母短皮终聚汽村云哪既距卫停烈央察烧迅境若印洲刻括激孔搞甚室待核校散侵吧甲游久菜味旧模湖货损预阻毫普稳乙妈植息扩银语挥酒守拿序纸医缺雨吗针刘啊急唱误训愿审附获茶鲜粮斤孩脱硫肥善龙演父渐血欢械掌歌沙著刚攻谓盾讨晚粒乱燃矛乎杀药宁鲁贵钟煤读班伯香介迫句丰培握兰担弦蛋沉假穿执答乐准顺营"
    # 补充常用字
    "买卖借贷付账购销存储蓄款欠税罚没举报投诉纠纷讼争调解裁定判令禁停止暂停关闭启停开关按钮拨插接拆装修建改扩增添删除减注撤销更改变换替代替续约续费扣返退换货赔偿补偿贴补协助帮忙救援助急避险危险害损伤毁坏灭丢失漏泄透露露秘密隐私隐瞒欺骗诈骗偷盗窃抢劫掠夺夺强暴威胁迫吓恐惊吓慌乱忙闲休憩息睡觉醒梦幻想思念考虑忧愁烦恼怒喜乐悲哀痛苦闷压抑抑郁孤独寂寞冷热温凉爽干燥湿润潮闷晒雨雪风暴雷电台广播视讯网络联络通信号码地址姓名年龄性别出生日期份证明证件牌照片图像视音频乐歌曲舞戏曲剧集节目标题内容简介评论点赞分享转发收藏关注取消订阅阅读浏览搜索查找输入输出打印复印扫描发送接收存储记记录备忘忘记忆历史未来现在过去经常偶尔时常永远暂临时刻钟秒毫微妙分钟小时天周月季度年岁代世纪"
)

# ── 尝试加载中文字体 ──
FONT_PATHS = [
    "C:/Windows/Fonts/simsun.ttc",      # 宋体
    "C:/Windows/Fonts/msyh.ttc",         # 微软雅黑
    "C:/Windows/Fonts/simhei.ttf",       # 黑体
    "C:/Windows/Fonts/Deng.ttf",         # 等线
]

font = None
for fp in FONT_PATHS:
    if os.path.exists(fp):
        try:
            font = ImageFont.truetype(fp, 16)
            print(f"Using font: {fp}")
            break
        except:
            pass

if font is None:
    print("ERROR: No Chinese font found!")
    exit(1)

# ── 生成 SSD1306 页-列格式的 16x16 位图 ──
def render_char(ch):
    """渲染一个汉字, 返回 32 字节 (16x16 页-列格式, LSB=top)"""
    # 用 PIL 绘制 16x16 图像
    img = Image.new('1', (16, 16), 0)  # 黑底白字
    draw = ImageDraw.Draw(img)
    # 居中绘制 (减去字体内部的偏移)
    bbox = font.getbbox(ch)
    if bbox:
        ox = (16 - (bbox[2] - bbox[0])) // 2 - bbox[0]
        oy = (16 - (bbox[3] - bbox[1])) // 2 - bbox[1]
    else:
        ox, oy = 0, 0
    draw.text((ox, oy), ch, font=font, fill=1)

    # 转为 SSD1306 页-列格式
    data = bytearray(32)
    pixels = img.load()
    for col in range(16):
        for page in range(2):       # 2 页: 0-7, 8-15
            b = 0
            for bit in range(8):    # 8 行/页, LSB=top
                y = page * 8 + bit
                if y < 16 and pixels[col, y]:
                    b |= (1 << bit)
            data[page * 16 + col] = b
    return bytes(data)

# ── 去除重复并排序 ──
seen = set()
unique_chars = []
for c in CHARS.replace(" ", "").replace("\n", ""):
    if c not in seen:
        seen.add(c)
        unique_chars.append(c)

print(f"Generating {len(unique_chars)} unique characters...")

# ── 写入 OLED_Font.h ──
with open("src/ui/OLED_Font.h", "w", encoding="utf-8") as f:
    f.write("""#ifndef __OLED_DATA_H
#define __OLED_DATA_H

#include <stdint.h>

#define OLED_CHN_CHAR_WIDTH 3  /* UTF-8 encoding */

typedef struct
{
    char Index[OLED_CHN_CHAR_WIDTH + 1];
    uint8_t Data[32];
} ChineseCell_t;

/* ASCII 8x16 (95 chars, space to ~) */
const uint8_t OLED_F8x16[][16] =
{
""")

    # ASCII 8x16 字体 (用 PIL 生成)
    ascii_font = ImageFont.truetype(FONT_PATHS[0], 16) if os.path.exists(FONT_PATHS[0]) else font
    for ch_code in range(32, 127):
        ch = chr(ch_code)
        img = Image.new('1', (8, 16), 0)
        draw = ImageDraw.Draw(img)
        draw.text((0, 0), ch, font=ascii_font, fill=1)
        pixels = img.load()
        data = bytearray(16)
        for col in range(8):
            for page in range(2):
                b = 0
                for bit in range(8):
                    y = page * 8 + bit
                    if y < 16 and pixels[col, y]:
                        b |= (1 << bit)
                data[page * 8 + col] = b
        hex_bytes = ",".join(f"0x{b:02X}" for b in data)
        f.write(f"    {hex_bytes},  /* '{ch}' {ch_code - 32} */\n")

    f.write("""};

/* Chinese 16x16 font */
const ChineseCell_t OLED_CF16x16[] = {
""")

    for i, ch in enumerate(unique_chars):
        data = render_char(ch)
        hex_str = ",".join(f"0x{b:02X}" for b in data)
        f.write(f'    "{ch}",\n')
        f.write(f"    {hex_str},  /* {ch} {i} */\n")

    # Sentinel
    f.write('    "",\n')
    f.write('    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},\n')
    f.write("};\n\n")

    f.write("#endif\n")

print(f"Done! {len(unique_chars)} Chinese characters written to src/ui/OLED_Font.h")

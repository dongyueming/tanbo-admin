import qrcode
from PIL import Image, ImageDraw, ImageFont

LOCAL = "http://192.168.110.216:8080"
PUBLIC = "https://foam-introduction-fellowship-deer.trycloudflare.com"


def make(url, title, sub, out):
    qr = qrcode.QRCode(box_size=10, border=2, error_correction=qrcode.constants.ERROR_CORRECT_M)
    qr.add_data(url)
    qr.make(fit=True)
    img = qr.make_image(fill_color="black", back_color="white").convert("RGB")
    w, h = img.size
    pad = 60
    top = 110
    canvas = Image.new("RGB", (w + pad * 2, h + top + pad), "white")
    canvas.paste(img, (pad, top))
    d = ImageDraw.Draw(canvas)
    try:
        font_b = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 34)
        font_s = ImageFont.truetype("C:/Windows/Fonts/msyh.ttc", 22)
    except Exception:
        font_b = ImageFont.load_default()
        font_s = ImageFont.load_default()
    d.text((pad, 25), title, fill="#2f7d4f", font=font_b)
    d.text((pad, 70), sub, fill="#666", font=font_s)
    d.text((pad, h + top + 18), url, fill="#333", font=font_s)
    canvas.save(out)
    print("saved:", out, canvas.size)


make(LOCAL, "小摊串串经营台账", "手机连同一 WiFi 即可访问", "手机扫码访问.png")
make(PUBLIC, "小摊串串经营台账 · 公网", "外地也能打开，地址重启后会变", "公网访问二维码.png")

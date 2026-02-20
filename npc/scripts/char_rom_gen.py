from PIL import Image, ImageFont, ImageDraw

def generate_font_hex(output_file="char_rom.hex"):
    char_width = 8
    char_height = 16
    
    # 尝试加载一个清晰的等宽字体，如果没有则使用系统默认
    try:
        # 在 Linux/Mac/Windows 常见的等宽字体路径
        font = ImageFont.truetype("cour.ttf", 14) # Courier New
    except:
        font = ImageFont.load_default()

    with open(output_file, 'w') as f:
        # 遍历 ASCII 0 到 255
        for i in range(256):
            char = chr(i)
            # 创建一个 8x16 的黑白画布 (Mode '1' 是单色位图)
            img = Image.new('1', (char_width, char_height), color=0)
            draw = ImageDraw.Draw(img)
            
            # 将字符画在画布上
            # 注意：某些不可见字符会显示为空白
            draw.text((0, 0), char, font=font, fill=1)
            
            # 遍历每一行，转为 hex
            for y in range(char_height):
                row_val = 0
                for x in range(char_width):
                    pixel = img.getpixel((x, y))
                    # 按照 Verilog 逻辑：高位在左 (line_tex[7] 是第一个像素)
                    if pixel:
                        row_val |= (1 << (7 - x))
                
                f.write(f"{row_val:02X}\n")

    print(f"Successfully generated {output_file} (4096 lines)")

if __name__ == "__main__":
    generate_font_hex()
    
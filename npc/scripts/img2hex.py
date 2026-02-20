from PIL import Image

def generate_hex(input_img_path, output_hex_path):
    try:
        # 打开图片
        img = Image.open(input_img_path)
        
        # 强制转换为 RGB 模式
        img = img.convert("RGB")
        
        # 调整大小至 640x480
        img = img.resize((640, 480))
        
        width, height = img.size
        print(f"Processing image: {width}x{height}")

        # with open(output_hex_path, 'w') as f:
        #     for y in range(height):       # 0 to 479
        #         for x in range(width):    # 0 to 639
        #             r, g, b = img.getpixel((x, y))
        #             # 生成 24位 hex: RRGGBB
        #             hex_val = f"{r:02X}{g:02X}{b:02X}"
        #             f.write(hex_val + '\n')
        with open(output_hex_path, 'w') as f:
            # 必须以 X 为外循环，因为 Verilog 里的拼接高位是 h_addr
            for x in range(1024): 
                for y in range(512): # 对应 v_addr 的 9 位空间
                    if x < width and y < height:
                        r, g, b = img.getpixel((x, y))
                        hex_val = f"{r:02X}{g:02X}{b:02X}"
                        f.write(hex_val + '\n')
                    else:
                        # 填充空洞（Padding）
                        # 这里的空洞是为了填满 1024*512 的地址空间
                        f.write("000000\n")
        print(f"Successfully generated {output_hex_path}")

    except Exception as e:
        print(f"Error: {e}")

# 使用示例
if __name__ == "__main__":
    # 请确保当前目录下有一张名为 genshin_icon.jpg 或 .png 的图片
    # 如果没有，请先去找一张
    input_image = "genshin_icon.png" 
    output_hex = "genshin_icon.hex"
    
    # 简单的创建一个纯色测试图，防止你也找不到图片报错
    try:
        Image.open(input_image)
    except:
        print(f"Image {input_image} not found. Creating a dummy placeholder image.")
        img = Image.new('RGB', (640, 480), color = 'red')
        img.save(input_image)
        
    generate_hex(input_image, output_hex)

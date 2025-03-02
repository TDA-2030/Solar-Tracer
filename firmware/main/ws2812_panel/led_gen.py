import cv2
import numpy as np
import time
import imageio
from pathlib import Path
import colorsys
import random

ROOT = Path(__file__).parent

def check_led_color(color, led_color_range):
    """
    判断一个颜色是否符合 LED 的颜色特征
    这里的 LED 色彩范围可以调整
    """
    lower_bound, upper_bound = led_color_range
    return np.all(lower_bound <= color) and np.all(color <= upper_bound)

def divide_image_into_grid(image, N, M):
    """
    将图像划分为 N x M 的方格
    """
    h, w, _ = image.shape
    grid_height = h // N
    grid_width = w // M
    grids = []

    for i in range(N):
        for j in range(M):
            y1 = i * grid_height
            y2 = (i + 1) * grid_height
            x1 = j * grid_width
            x2 = (j + 1) * grid_width
            grid = image[y1:y2, x1:x2]
            grids.append((i, j, grid, x1, y1, grid_width, grid_height))  # 额外保存方格的位置和大小

    return grids

def get_average_color(grid):
    """
    计算给定方格的平均颜色
    """
    return np.mean(grid, axis=(0, 1))  # 对每个通道（B, G, R）求平均值

def cartesian_to_polar(x, y, width, height):
    # 中心化
    x -= width / 2
    y -= height / 2
    # 计算极坐标
    r = np.sqrt(x**2 + y**2)
    theta = np.arctan2(y, x)
    return r, theta

def polar_to_cartesian(r, theta, width, height):
    # 转回笛卡尔坐标
    x = r * np.cos(theta) + width / 2
    y = r * np.sin(theta) + height / 2
    return round(x), round(y)


def main(image_path, N, M, led_color_range):
    # 读取图像
    image = cv2.imread(image_path)
    
    # 将图像划分为 N x M 的方格
    grids = divide_image_into_grid(image, N, M)
    
    # 结果保存列表
    grid_results = []

    # 在原图上绘制检测结果
    output_image = image.copy()

    for (i, j, grid, x1, y1, grid_width, grid_height) in grids:
        # 获取方格的平均颜色
        avg_color = get_average_color(grid)
        
        # 判断该方格是否存在 LED
        if check_led_color(avg_color, led_color_range):
            # 在包含 LED 的方格中心绘制绿色实心圆
            center_x = x1 + grid_width // 2
            center_y = y1 + grid_height // 2
            cv2.circle(output_image, (center_x, center_y), min(grid_width, grid_height) // 4, (0, 255, 0), -1)  # 绿色圆圈
            grid_results.append((i, j, True))  # 方格中有 LED
        else:
            grid_results.append((i, j, False))  # 方格中没有 LED
    
    return grid_results, output_image


def my_main():
    # LED颜色的范围，例如红色的范围
    led_color_range = (np.array([0, 0, 150]), np.array([100, 100, 255]))  # 红色的范围

    # 输入图片路径和划分的 NxM
    image_path = 'img.png'
    N = int(790/10)  # 纵向方格数量
    M = int(790/10)  # 横向方格数量
    results, output_image = main(image_path, N, M, led_color_range)


    # 输出每个方格的LED状态
    for (i, j, has_led) in results:
        if has_led:
            print(f"LED detected in Grid ({i}, {j})")
        # print(f"Grid ({i}, {j}): {'LED detected' if has_led else 'No LED'}")

    # 显示并保存结果图像
    # cv2.imshow("Detected LEDs", output_image)  # 显示检测结果图像
    # cv2.imwrite('output_with_leds.jpg', output_image)  # 保存结果图像
    # cv2.waitKey(0)
    # cv2.destroyAllWindows()

######################################################################################

def generate_gradient(color, steps=9):
    """生成从指定颜色到黑色的渐变色数组，共9级"""
    gradient = []
    for i in range(steps):
        # 计算当前步长的颜色值
        r = int(color[0] * ((steps-i)/steps))
        g = int(color[1] * ((steps-i)/steps))
        b = int(color[2] * ((steps-i)/steps))
        gradient.append((b, g, r))
    return gradient

# 定义一些颜色
colors = [
    (255, 0, 0),  # 红色
    (0, 255, 0),  # 绿色
    (0, 0, 255),  # 蓝色
    (255, 255, 0),  # 黄色
    (0, 255, 255),  # 青色
    (255, 0, 255),  # 紫色
    (128, 0, 128),  # 深紫色
    (255, 165, 0),  # 橙色
    (255, 192, 203)  # 粉红色
]




class LED_PANEL1:
    def __init__(self):
        self.W = 40
        self.H = 40
    def calcu_grid(self):
        
        vis_img = np.zeros((H, W, 3), dtype=np.uint8)
        led_map = np.zeros((W, H), dtype=np.int8)
        for i in range(16):
            x, y = polar_to_cartesian(W/2-1, 2*np.pi*i/16, W, H)
            vis_img[y, x] = (0, 255, 0) # h,w,c
            led_map[x, y] = i+1
        cv2.imwrite('output_with_leds.jpg', vis_img)  # 保存结果图像
        
        
        return led_map

    def set_pixel(self, img:np.ndarray, idx, color):
        h, w = img.shape[:2]
        x, y = polar_to_cartesian(W/2-1, 2*np.pi*idx/16, w, h)
        img[y, x] = color

    def refresh_led(self, img:np.ndarray, led_map):
        for _x in range(W):
            for _y in range(H):
                idx = led_map[_x, _y]
                if idx > 0:
                    self.set_pixel(LED_IMG, idx, img[_y, _x])

class LED_PANEL2:
    def __init__(self):
        self.W = 100
        self.H = 100
        self.led_map = np.zeros((self.H, self.W), dtype=np.uint8)
        self.led_line_map = np.zeros((16, 10), dtype=np.uint8)
        self.led_circle2_map = np.zeros((32), dtype=np.uint8)
        self.led_circle1_map = np.zeros((8), dtype=np.uint8)
        self.i2xy = []
        self.LED_IMG = np.zeros((self.H, self.W, 3), dtype=np.uint8)
        self.calcu_grid()
        cv2.namedWindow('Image Window', cv2.WINDOW_NORMAL)

    def save_to_file(self):
        array = self.led_map
        h, w = array.shape[:2]
        array_name = 'led_loc_map'
        c_array = ""
        c_array += f"#ifndef {array_name.upper()}_H\n"
        c_array += f"#define {array_name.upper()}_H\n\n"

        c_array += "#include <stdint.h>\n"
        c_array += f"#define LED_WIDTH {w}\n"
        c_array += f"#define LED_HEIGHT {h}\n"

        
        c_array += f"uint8_t const led_loc_map[LED_WIDTH][LED_HEIGHT]={{\n"
        for row in array:
            c_array += "    {"
            c_array += ", ".join(map(str, row))
            c_array += "},\n"
        c_array = c_array.rstrip(",\n") + "\n"
        c_array += "};\n\n"

        c_array += f"uint8_t const led_line_map[{self.led_line_map.shape[0]}][{self.led_line_map.shape[1]}]={{\n"
        for row in self.led_line_map:
            c_array += "    {"
            c_array += ", ".join(map(str, row))
            c_array += "},\n"
        c_array = c_array.rstrip(",\n") + "\n"
        c_array += "};\n\n"

        c_array += f"uint8_t const led_circle2_map[{self.led_circle2_map.shape[0]}]={{\n"
        c_array += ", ".join(map(str, self.led_circle2_map))
        c_array = c_array.rstrip(",\n") + "\n"
        c_array += "};\n\n"

        c_array += f"uint8_t const led_circle1_map[{self.led_circle1_map.shape[0]}]={{\n"
        c_array += ", ".join(map(str, self.led_circle1_map))
        c_array = c_array.rstrip(",\n") + "\n"
        c_array += "};\n\n"


        c_array += f"#endif // {array_name.upper()}_H\n"

        with open(f"{array_name}.h", "w") as file:
            file.write(c_array)

        print(f"The C array has been saved to {array_name}.h")

    def calcu_grid(self):
        h, w = self.H, self.W
        factor = self.W/160
        vis_img = np.zeros((h, w, 3), dtype=np.uint8)
        _idx = 1
        _dir = 0

        def add_point(r, t):
            nonlocal _idx
            x, y = polar_to_cartesian(r, t, w, h)
            vis_img[y, x] = (0, 255, 0) # h,w,c
            self.led_map[x, y] = _idx
            self.i2xy.append((x, y))
            _idx += 1
            print(f"x,y:{x},{y}")
            # cv2.imwrite('output_with_leds.jpg', vis_img)  # 保存结果图像
            # time.sleep(0.06)

        for i in range(16):
            for __i in range(10):
                if _dir: # 向内
                    _r = factor*(18+(9-__i)*4.5)
                else: # 向外
                    _r = factor*(18+__i*4.5)
                
                add_point(_r, 2*np.pi*i/16)
                self.led_line_map[i, (9-__i) if _dir else __i] = _idx-1
                
            if not _dir:
                add_point(_r, 2*np.pi*(i/16 + 1/32))

            _dir = not _dir

        print("=====\n")
        for i in range(8):
            add_point(factor*(18+9*4.5), 2*np.pi*(i*1/8 + 3/32))
            self.led_circle1_map[i] = _idx - 1

        print("-----\n")
        for i in range(32):
            add_point(factor*(75), 2*np.pi*(i*1/32 - 1/32))
            self.led_circle2_map[i] = _idx - 1

        print(self.led_circle1_map)

        _idx -= 1
        print(f"Done idx:{_idx}")
        self.save_to_file()
        # cv2.imwrite('Image.jpg', vis_img)  # 保存结果图像
        return self.led_map

    def _write_line(self, angle, r, color):
        """
        设置一条射线上的一个像素
        angle: 0~15
        r: 0~9

        """
        if r > 9 or r < 0:
            return
        if angle < 0:
            angle += 16
        angle = angle % 16
        self._write_pixel(self.led_line_map[angle, r], color)

    def _write_circle1(self, angle, color):
        if angle < 0:
            angle += 8
        angle = angle % 8
        self._write_pixel(self.led_circle1_map[angle], color)

    def _write_circle2(self, angle, color):
        if angle < 0:
            angle += 32
        angle = angle % 32
        self._write_pixel(self.led_circle2_map[angle], color)

    def draw_horizontal_line(self, x, y, line_width, colored):
        """
        Draws a horizontal line.
        :param self: A drawable canvas or function to draw pixels
        :param x: X-coordinate of the start of the line
        :param y: Y-coordinate of the line
        :param line_width: Width of the line
        :param colored: Color to draw (could be a boolean or a color value)
        """
        for i in range(x, x + line_width):
            self._write_pixel_xy(i, y, colored)

    def draw_circle(self, x, y, radius, colored):
        """
        Draws a circle using the Bresenham algorithm.
        :param self: A drawable canvas or function to draw pixels
        :param x: X-coordinate of the circle center
        :param y: Y-coordinate of the circle center
        :param radius: Radius of the circle
        :param colored: Color to draw (could be a boolean or a color value)
        """
        x_pos = -radius
        y_pos = 0
        err = 2 - 2 * radius
        while x_pos <= 0:
            self._write_pixel_xy(x - x_pos, y + y_pos, colored)
            self._write_pixel_xy(x + x_pos, y + y_pos, colored)
            self._write_pixel_xy(x + x_pos, y - y_pos, colored)
            self._write_pixel_xy(x - x_pos, y - y_pos, colored)
            e2 = err
            if e2 <= y_pos:
                y_pos += 1
                err += y_pos * 2 + 1
                if -x_pos == y_pos and e2 <= x_pos:
                    e2 = 0
            if e2 > x_pos:
                x_pos += 1
                err += x_pos * 2 + 1


    def draw_filled_circle(self, x, y, radius, colored):
        """
        Draws a filled circle using the Bresenham algorithm.
        :param self: A drawable canvas or function to draw pixels
        :param x: X-coordinate of the circle center
        :param y: Y-coordinate of the circle center
        :param radius: Radius of the circle
        :param colored: Color to draw (could be a boolean or a color value)
        """
        x_pos = -radius
        y_pos = 0
        err = 2 - 2 * radius
        while x_pos <= 0:
            self._write_pixel_xy(x - x_pos, y + y_pos, colored)
            self._write_pixel_xy(x + x_pos, y + y_pos, colored)
            self._write_pixel_xy(x + x_pos, y - y_pos, colored)
            self._write_pixel_xy(x - x_pos, y - y_pos, colored)
            self.draw_horizontal_line(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored)
            self.draw_horizontal_line(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored)
            e2 = err
            if e2 <= y_pos:
                y_pos += 1
                err += y_pos * 2 + 1
                if -x_pos == y_pos and e2 <= x_pos:
                    e2 = 0
            if e2 > x_pos:
                x_pos += 1
                err += x_pos * 2 + 1



    def _write_pixel(self, idx, color):
        x, y  = self.i2xy[idx-1]
        self.LED_IMG[y, x] = color

    def _write_pixel_xy(self, x, y, color):
        idx = self.led_map[x, y]
        if idx > 0:
            self._write_pixel(idx, color)

    def _write_img(self, img:np.ndarray):
        h, w = img.shape[:2]
        for _x in range(w):
            for _y in range(h):
                idx = self.led_map[_x, _y]
                if idx > 0:
                    self._write_pixel(idx, img[_y, _x])

        # cv2.imwrite('output_with_leds.bmp', self.LED_IMG)
        cv2.imshow('Image Window', cv2.resize(self.LED_IMG, None, fx=1, fy=1, interpolation=cv2.INTER_NEAREST))
        cv2.waitKey(1)


    def pretty_effect1(self):
        circle = 0
        # 生成渐变色数组
        color_gradients = generate_gradient(colors[6])
        a = 0
        rat = 0
        # line_pos = np.linspace(0, 15, 16, dtype=np.uint8)
        line_pos = np.zeros(16, dtype=np.float32)
        line_bits = np.zeros(16, dtype=np.uint16)
        while True:
            self._write_circle2(a-9, (0, 0, 0))
            for i in range(9):
                self._write_circle2(a-i, color_gradients[i])

            if 0 == a%2:
                _ang = a//2
                line_bits[_ang] |= 1 << (9+2)
                for i in range(16):
                    line_pos[i] += 0.4
                    if line_pos[i] > 1.0:
                        line_bits[i] >>= 1
                        line_pos[i] -= 1.0
                        for _b in range(12):
                            if 1<<_b & line_bits[i]:
                                self._write_line(i, _b, (0, 0, 0))
                                self._write_line(i, _b-1, color_gradients[1])
                                self._write_line(i, _b-2, color_gradients[0])

            cv2.imshow('Image Window', cv2.resize(self.LED_IMG, None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST))
            time.sleep(0.03)
            cv2.waitKey(1)

            a += 1
            if a >= 32:
                a = 0

    @staticmethod
    def generate_distinct_color(b, g, r):
        # # 将十六进制颜色转换为 RGB
        # b = (hex_color >> 16) & 0xFF
        # g = (hex_color >> 8) & 0xFF
        # r = hex_color & 0xFF

        # 将 RGB 转换为 HSV
        h, s, v = colorsys.rgb_to_hsv(r / 255.0, g / 255.0, b / 255.0)

        # 生成一个新的色相值，确保与原色相差明显
        new_h = (h + random.uniform(0.5, 1.0)) % 1.0  # 确保色相相差较大

        # 将新的 HSV 转换回 RGB
        new_r, new_g, new_b = colorsys.hsv_to_rgb(new_h, s, v)
    

        return (new_b*255, new_g*255, new_r*255)
        
    def pretty_effect2(self):
        color = (0, 255, 0)
        while True:

            for i in range(int(self.H/2)-1):
                self.draw_filled_circle(self.W//2, self.H//2, i, color)

                cv2.imshow('Image Window', cv2.resize(self.LED_IMG, None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST))
                time.sleep(0.01)
                cv2.waitKey(1)
            color = self.generate_distinct_color(*color)

    def pretty_effect3(self):
        color = (0, 255, 0)
        color_gradients = generate_gradient(colors[3], steps=6)
        color_gradients += color_gradients[::-1]
        cnt=0
        while True:

            for i in range(self.H//12, self.H//2-10):
                print(i)
                self.draw_circle(self.W//2, self.H//2, i, color_gradients[(cnt+i)%12])
            print(f"cnt={cnt}")
            cnt+=1
            if cnt >= 11:
                cnt = 0

            cv2.imshow('Image Window', cv2.resize(self.LED_IMG, None, fx=4, fy=4, interpolation=cv2.INTER_NEAREST))
            time.sleep(0.2)
            cv2.waitKey(1)
            color = self.generate_distinct_color(*color)    



if __name__ == "__main__":
    
    panel = LED_PANEL2()
    panel.pretty_effect3()
    exit()

    gif_path="22.gif"
    # 读取 GIF 文件
    gif = imageio.mimread(gif_path)
    print(f"GIF 共有 {len(gif)} 帧")

    # 获取 GIF 的帧率
    gif_meta = imageio.get_reader(gif_path).get_meta_data()
    fps = gif_meta.get('duration', 100)  # duration 是每帧持续时间，单位是ms
    print(f"GIF duration= {fps} ms")

    for i in range(10):
        # 将 GIF 的每一帧转换为 OpenCV 格式并显示
        for frame in gif:
            # 转换为 OpenCV 的 BGR 格式
            frame_bgr = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
            # frame_bgr = cv2.resize(frame_bgr, (panel.W, panel.H))
            # frame_bgr = cv2.resize(frame_bgr[33:149, 405:537], (panel.W, panel.H))
            frame_bgr = cv2.resize(frame_bgr[428:530, 322:430], (panel.W, panel.H))

            # 将BGR图像转换为HSV图像
            frame_hsv = cv2.cvtColor(frame_bgr, cv2.COLOR_BGR2HSV)

            # 定义白色的HSV阈值范围
            # 注意：这些值可能需要根据你的图像进行调整
            lower_white = np.array([0, 0, 168])
            upper_white = np.array([180, 30, 255])
            # print(frame_hsv)

            # 创建掩码，用于识别接近白色的像素
            mask = cv2.inRange(frame_hsv, lower_white, upper_white)

            # 将掩码反转，因为我们想要的是接近白色的像素
            mask = cv2.bitwise_not(mask)

            # 使用掩码将接近白色的像素设置为黑色
            frame_bgr[mask > 0] = [0, 0, 255]
            frame_bgr[mask == 0] = [0, 0, 0]

            # 显示帧
            panel._write_img(frame_bgr)

            # 等待帧时间
            time.sleep(0.2)  # 转换为毫秒



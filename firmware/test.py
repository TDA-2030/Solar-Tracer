import subprocess
import os
from datetime import datetime, timedelta

def compile_program():
    """编译C++程序"""
    try:
        subprocess.run(['gcc', 'main/gimbal/sun_pos.cpp', '-o', 'sun_pos', '-lstdc++', '-lm'], check=True)
        print("编译成功!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"编译失败: {e}")
        return False

def generate_test_cases():
    """生成测试数据"""
    test_cases = []
    
    # 生成一天中不同时间的测试数据
    base_date = datetime(2025, 3, 28)  # 使用固定日期
    for hour in range(0, 24, 1):  # 每隔2小时取一个时间点
        test_time = base_date + timedelta(hours=hour-8)
        test_cases.append((
            test_time.year,
            test_time.month,
            test_time.day,
            test_time.hour,
            test_time.minute
        ))
    return test_cases

def run_test(test_case):
    """运行单个测试用例"""
    year, month, day, hour, minute = test_case
    try:
        result = subprocess.run(
            ['./sun_pos', str(year), str(month), str(day), str(hour), str(minute)],
            capture_output=True,
            text=True,
            check=True
        )
        return result.stdout
    except subprocess.CalledProcessError as e:
        return f"测试执行错误: {e}"

def main():
    # 1. 编译程序
    if not compile_program():
        return

    # 2. 生成测试用例
    test_cases = generate_test_cases()
    
    # 3. 运行测试
    print("\n=== 开始测试 ===")
    print("时间\t\t\t方位角\t\t天顶角\t\t高度角")
    print("-" * 50)
    
    for test_case in test_cases:
        year, month, day, hour, minute = test_case
        result = run_test(test_case)
        print(f"{year}-{month:02d}-{day:02d} {hour+8:02d}:{minute:02d}\t{result}", end='')

if __name__ == "__main__":
    main()
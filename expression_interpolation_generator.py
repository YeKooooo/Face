#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能用药提醒机器人表情插值生成器 - 基于线条的表情系统
功能：使用几何线条绘制表情，通过线条端点插值生成中间表情
"""

import os
import json
import math
from PIL import Image, ImageDraw
from typing import Dict, List, Tuple, NamedTuple

class LineSegment(NamedTuple):
    """线段定义"""
    start: Tuple[float, float]  # 起点坐标
    end: Tuple[float, float]    # 终点坐标
    width: int                  # 线条宽度
    color: Tuple[int, int, int] # RGB颜色

class ExpressionParams(NamedTuple):
    """基于线条的表情参数结构"""
    background_color: Tuple[int, int, int]  # RGB背景色
    face_lines: List[LineSegment]           # 面部线条列表
    eyes: List[LineSegment]                 # 眼睛线条
    eyebrows: List[LineSegment]             # 眉毛线条
    mouth: List[LineSegment]                # 嘴巴线条
    description: str                        # 描述

class ExpressionInterpolator:
    """基于线条的表情插值生成器"""
    
    def __init__(self):
        # 图像尺寸和中心点
        self.image_size = (200, 200)
        self.center_x, self.center_y = 100, 100
        
        # 定义7种基础表情的线条坐标
        self.expressions = {
            'Happy': self._create_happy_expression(),
            'Caring': self._create_caring_expression(),
            'Concerned': self._create_concerned_expression(),
            'Encouraging': self._create_encouraging_expression(),
            'Alert': self._create_alert_expression(),
            'Sad': self._create_sad_expression(),
            'Neutral': self._create_neutral_expression()
        }
        
        self.output_dir = 'expression_interpolations'
        self.interpolation_steps = 10  # 每对表情之间生成10个中间状态
    
    def _create_neutral_expression(self) -> ExpressionParams:
        """创建中性表情"""
        face_lines = [LineSegment((50, 50), (150, 50), 2, (100, 100, 100)),  # 上边框
                     LineSegment((150, 50), (150, 150), 2, (100, 100, 100)), # 右边框
                     LineSegment((150, 150), (50, 150), 2, (100, 100, 100)), # 下边框
                     LineSegment((50, 150), (50, 50), 2, (100, 100, 100))]   # 左边框
        
        eyes = [LineSegment((70, 80), (80, 80), 3, (0, 0, 0)),    # 左眼
               LineSegment((120, 80), (130, 80), 3, (0, 0, 0))]   # 右眼
        
        eyebrows = [LineSegment((65, 70), (85, 70), 2, (80, 80, 80)),  # 左眉毛
                   LineSegment((115, 70), (135, 70), 2, (80, 80, 80))] # 右眉毛
        
        mouth = [LineSegment((85, 120), (115, 120), 2, (60, 60, 60))]  # 直线嘴巴
        
        return ExpressionParams((240, 240, 240), face_lines, eyes, eyebrows, mouth, '中性')
    
    def _create_happy_expression(self) -> ExpressionParams:
        """创建开心表情"""
        face_lines = [LineSegment((50, 50), (150, 50), 2, (0, 150, 0)),
                     LineSegment((150, 50), (150, 150), 2, (0, 150, 0)),
                     LineSegment((150, 150), (50, 150), 2, (0, 150, 0)),
                     LineSegment((50, 150), (50, 50), 2, (0, 150, 0))]
        
        eyes = [LineSegment((70, 75), (80, 85), 3, (0, 0, 0)),    # 左眼（弯曲向上）
               LineSegment((120, 85), (130, 75), 3, (0, 0, 0))]   # 右眼（弯曲向上）
        
        eyebrows = [LineSegment((65, 65), (85, 60), 2, (0, 100, 0)),  # 左眉毛（上扬）
                   LineSegment((115, 60), (135, 65), 2, (0, 100, 0))] # 右眉毛（上扬）
        
        mouth = [LineSegment((80, 115), (100, 125), 3, (0, 80, 0)),   # 嘴巴左半部分（向上弯曲）
                LineSegment((100, 125), (120, 115), 3, (0, 80, 0))]  # 嘴巴右半部分（向上弯曲）
        
        return ExpressionParams((200, 255, 200), face_lines, eyes, eyebrows, mouth, '开心')
    
    def _create_sad_expression(self) -> ExpressionParams:
        """创建悲伤表情"""
        face_lines = [LineSegment((50, 50), (150, 50), 2, (120, 120, 120)),
                     LineSegment((150, 50), (150, 150), 2, (120, 120, 120)),
                     LineSegment((150, 150), (50, 150), 2, (120, 120, 120)),
                     LineSegment((50, 150), (50, 50), 2, (120, 120, 120))]
        
        eyes = [LineSegment((70, 85), (80, 75), 3, (0, 0, 0)),    # 左眼（向下）
               LineSegment((120, 75), (130, 85), 3, (0, 0, 0))]   # 右眼（向下）
        
        eyebrows = [LineSegment((65, 75), (85, 70), 2, (100, 100, 100)),  # 左眉毛（下垂）
                   LineSegment((115, 70), (135, 75), 2, (100, 100, 100))] # 右眉毛（下垂）
        
        mouth = [LineSegment((80, 125), (100, 115), 3, (80, 80, 80)),   # 嘴巴左半部分（向下弯曲）
                LineSegment((100, 115), (120, 125), 3, (80, 80, 80))]  # 嘴巴右半部分（向下弯曲）
        
        return ExpressionParams((200, 200, 200), face_lines, eyes, eyebrows, mouth, '悲伤')
    
    def _create_caring_expression(self) -> ExpressionParams:
        """创建关怀表情"""
        face_lines = [LineSegment((50, 50), (150, 50), 2, (200, 100, 150)),
                     LineSegment((150, 50), (150, 150), 2, (200, 100, 150)),
                     LineSegment((150, 150), (50, 150), 2, (200, 100, 150)),
                     LineSegment((50, 150), (50, 50), 2, (200, 100, 150))]
        
        eyes = [LineSegment((70, 78), (80, 82), 3, (0, 0, 0)),    # 左眼（温和）
               LineSegment((120, 82), (130, 78), 3, (0, 0, 0))]   # 右眼（温和）
        
        eyebrows = [LineSegment((65, 68), (85, 65), 2, (150, 80, 100)),  # 左眉毛（轻微上扬）
                   LineSegment((115, 65), (135, 68), 2, (150, 80, 100))] # 右眉毛（轻微上扬）
        
        mouth = [LineSegment((85, 118), (100, 122), 3, (120, 60, 80)),   # 嘴巴左半部分（微笑）
                LineSegment((100, 122), (115, 118), 3, (120, 60, 80))]  # 嘴巴右半部分（微笑）
        
        return ExpressionParams((255, 220, 230), face_lines, eyes, eyebrows, mouth, '关怀')
    
    def _create_concerned_expression(self) -> ExpressionParams:
        """创建担忧表情"""
        face_lines = [LineSegment((50, 50), (150, 50), 2, (200, 150, 50)),
                     LineSegment((150, 50), (150, 150), 2, (200, 150, 50)),
                     LineSegment((150, 150), (50, 150), 2, (200, 150, 50)),
                     LineSegment((50, 150), (50, 50), 2, (200, 150, 50))]
        
        eyes = [LineSegment((70, 80), (80, 80), 3, (0, 0, 0)),    # 左眼（正常）
               LineSegment((120, 80), (130, 80), 3, (0, 0, 0))]   # 右眼（正常）
        
        eyebrows = [LineSegment((65, 72), (85, 68), 2, (150, 100, 0)),   # 左眉毛（皱眉）
                   LineSegment((115, 68), (135, 72), 2, (150, 100, 0))]  # 右眉毛（皱眉）
        
        mouth = [LineSegment((85, 120), (100, 120), 2, (100, 80, 0)),   # 嘴巴左半部分（紧闭）
                LineSegment((100, 120), (115, 120), 2, (100, 80, 0))]  # 嘴巴右半部分（紧闭）
        
        return ExpressionParams((255, 230, 150), face_lines, eyes, eyebrows, mouth, '担忧')
    
    def _create_encouraging_expression(self) -> ExpressionParams:
        """创建鼓励表情"""
        face_lines = [LineSegment((50, 50), (150, 50), 2, (50, 150, 200)),
                     LineSegment((150, 50), (150, 150), 2, (50, 150, 200)),
                     LineSegment((150, 150), (50, 150), 2, (50, 150, 200)),
                     LineSegment((50, 150), (50, 50), 2, (50, 150, 200))]
        
        eyes = [LineSegment((70, 75), (80, 85), 4, (0, 0, 0)),    # 左眼（有力）
               LineSegment((120, 85), (130, 75), 4, (0, 0, 0))]   # 右眼（有力）
        
        eyebrows = [LineSegment((65, 62), (85, 58), 3, (0, 80, 150)),   # 左眉毛（坚定上扬）
                   LineSegment((115, 58), (135, 62), 3, (0, 80, 150))]  # 右眉毛（坚定上扬）
        
        mouth = [LineSegment((80, 112), (100, 128), 4, (0, 60, 120)),   # 嘴巴左半部分（大笑）
                LineSegment((100, 128), (120, 112), 4, (0, 60, 120))]  # 嘴巴右半部分（大笑）
        
        return ExpressionParams((200, 230, 255), face_lines, eyes, eyebrows, mouth, '鼓励')
    
    def _create_alert_expression(self) -> ExpressionParams:
        """创建警示表情"""
        face_lines = [LineSegment((50, 50), (150, 50), 3, (200, 50, 50)),
                     LineSegment((150, 50), (150, 150), 3, (200, 50, 50)),
                     LineSegment((150, 150), (50, 150), 3, (200, 50, 50)),
                     LineSegment((50, 150), (50, 50), 3, (200, 50, 50))]
        
        eyes = [LineSegment((68, 78), (82, 78), 4, (0, 0, 0)),    # 左眼（瞪大）
               LineSegment((118, 78), (132, 78), 4, (0, 0, 0))]   # 右眼（瞪大）
        
        eyebrows = [LineSegment((65, 65), (85, 62), 3, (150, 0, 0)),   # 左眉毛（紧皱）
                   LineSegment((115, 62), (135, 65), 3, (150, 0, 0))]  # 右眉毛（紧皱）
        
        mouth = [LineSegment((85, 125), (100, 125), 3, (120, 0, 0)),   # 嘴巴左半部分（严肃）
                LineSegment((100, 125), (115, 125), 3, (120, 0, 0))]  # 嘴巴右半部分（严肃）
        
        return ExpressionParams((255, 200, 200), face_lines, eyes, eyebrows, mouth, '警示')
        
    def smooth_interpolation(self, t: float) -> float:
        """平滑插值函数 (ease-in-out)"""
        return t * t * (3.0 - 2.0 * t)
    
    def interpolate_color(self, color1: Tuple[int, int, int], 
                         color2: Tuple[int, int, int], t: float) -> Tuple[int, int, int]:
        """颜色插值"""
        return (
            int(color1[0] + t * (color2[0] - color1[0])),
            int(color1[1] + t * (color2[1] - color1[1])),
            int(color1[2] + t * (color2[2] - color1[2]))
        )
    
    def interpolate_point(self, point1: Tuple[float, float], 
                         point2: Tuple[float, float], t: float) -> Tuple[float, float]:
        """点坐标插值"""
        return (
            point1[0] + t * (point2[0] - point1[0]),
            point1[1] + t * (point2[1] - point1[1])
        )
    
    def interpolate_line_segment(self, line1: LineSegment, 
                               line2: LineSegment, t: float) -> LineSegment:
        """线段插值"""
        start = self.interpolate_point(line1.start, line2.start, t)
        end = self.interpolate_point(line1.end, line2.end, t)
        width = int(line1.width + t * (line2.width - line1.width))
        color = self.interpolate_color(line1.color, line2.color, t)
        return LineSegment(start, end, width, color)
    
    def interpolate_line_list(self, lines1: List[LineSegment], 
                            lines2: List[LineSegment], t: float) -> List[LineSegment]:
        """线条列表插值"""
        # 确保两个列表长度相同，如果不同则进行补齐处理
        max_len = max(len(lines1), len(lines2))
        
        # 补齐较短的列表
        extended_lines1 = list(lines1)
        extended_lines2 = list(lines2)
        
        # 如果lines1较短，用最后一个线段补齐
        while len(extended_lines1) < max_len:
            if extended_lines1:
                extended_lines1.append(extended_lines1[-1])
            else:
                # 如果为空，创建一个默认的透明线段
                extended_lines1.append(LineSegment((100, 100), (100, 100), 1, (128, 128, 128)))
        
        # 如果lines2较短，用最后一个线段补齐
        while len(extended_lines2) < max_len:
            if extended_lines2:
                extended_lines2.append(extended_lines2[-1])
            else:
                # 如果为空，创建一个默认的透明线段
                extended_lines2.append(LineSegment((100, 100), (100, 100), 1, (128, 128, 128)))
        
        result = []
        for i in range(max_len):
            result.append(self.interpolate_line_segment(extended_lines1[i], extended_lines2[i], t))
        return result
     
    def interpolate_expressions(self, expr1: ExpressionParams, 
                              expr2: ExpressionParams, t: float) -> ExpressionParams:
        """基于线条的表情参数插值"""
        # 应用平滑插值
        smooth_t = self.smooth_interpolation(max(0.0, min(1.0, t)))
        
        # 插值计算
        bg_color = self.interpolate_color(expr1.background_color, expr2.background_color, smooth_t)
        face_lines = self.interpolate_line_list(expr1.face_lines, expr2.face_lines, smooth_t)
        eyes = self.interpolate_line_list(expr1.eyes, expr2.eyes, smooth_t)
        eyebrows = self.interpolate_line_list(expr1.eyebrows, expr2.eyebrows, smooth_t)
        mouth = self.interpolate_line_list(expr1.mouth, expr2.mouth, smooth_t)
        
        description = f"{expr1.description}→{expr2.description}"
            
        return ExpressionParams(bg_color, face_lines, eyes, eyebrows, mouth, description)
    
    def create_expression_image(self, params: ExpressionParams, 
                              filename: str) -> None:
        """使用线条绘制创建表情图像"""
        # 创建图像
        img = Image.new('RGB', self.image_size, params.background_color)
        draw = ImageDraw.Draw(img)
        
        # 绘制面部边框线条
        for line in params.face_lines:
            draw.line([line.start, line.end], fill=line.color, width=line.width)
        
        # 绘制眼睛线条
        for line in params.eyes:
            draw.line([line.start, line.end], fill=line.color, width=line.width)
        
        # 绘制眉毛线条
        for line in params.eyebrows:
            draw.line([line.start, line.end], fill=line.color, width=line.width)
        
        # 绘制嘴巴线条
        for line in params.mouth:
            draw.line([line.start, line.end], fill=line.color, width=line.width)
        
        # 保存图像
        img.save(filename, 'PNG')
        print(f"✓ 生成线条表情图像: {filename}")
    
    def generate_interpolation_sequence(self, expr1_name: str, expr2_name: str) -> List[str]:
        """生成两个表情之间的插值序列"""
        expr1 = self.expressions[expr1_name]
        expr2 = self.expressions[expr2_name]
        
        # 创建输出目录
        sequence_dir = os.path.join(self.output_dir, f"{expr1_name}_to_{expr2_name}")
        os.makedirs(sequence_dir, exist_ok=True)
        
        generated_files = []
        metadata = []
        
        # 生成插值序列
        for i in range(self.interpolation_steps + 1):
            t = i / self.interpolation_steps
            interpolated = self.interpolate_expressions(expr1, expr2, t)
            
            # 文件名
            filename = f"frame_{i:03d}_t{t:.2f}.png"
            filepath = os.path.join(sequence_dir, filename)
            
            # 生成图像
            self.create_expression_image(interpolated, filepath)
            generated_files.append(filepath)
            
            # 记录元数据
            metadata.append({
                'frame': i,
                't': t,
                'filename': filename,
                'description': interpolated.description,
                'params': {
                    'background_color': interpolated.background_color,
                    'face_lines_count': len(interpolated.face_lines),
                    'eyes_count': len(interpolated.eyes),
                    'eyebrows_count': len(interpolated.eyebrows),
                    'mouth_count': len(interpolated.mouth)
                }
            })
        
        # 保存元数据
        metadata_file = os.path.join(sequence_dir, 'metadata.json')
        with open(metadata_file, 'w', encoding='utf-8') as f:
            json.dump(metadata, f, ensure_ascii=False, indent=2)
        
        print(f"✓ 生成 {expr1_name} → {expr2_name}: {len(generated_files)} 帧")
        return generated_files
    
    def generate_all_interpolations(self) -> Dict[str, List[str]]:
        """生成所有表情对之间的插值"""
        print("开始生成表情插值序列...")
        print(f"输出目录: {os.path.abspath(self.output_dir)}")
        print(f"每个序列帧数: {self.interpolation_steps + 1}")
        print("-" * 50)
        
        # 创建主输出目录
        os.makedirs(self.output_dir, exist_ok=True)
        
        all_sequences = {}
        expression_names = list(self.expressions.keys())
        total_pairs = len(expression_names) * (len(expression_names) - 1)
        current_pair = 0
        
        # 生成所有表情对的插值
        for expr1_name in expression_names:
            for expr2_name in expression_names:
                if expr1_name != expr2_name:
                    current_pair += 1
                    print(f"[{current_pair}/{total_pairs}] 处理: {expr1_name} → {expr2_name}")
                    
                    sequence_key = f"{expr1_name}_to_{expr2_name}"
                    files = self.generate_interpolation_sequence(expr1_name, expr2_name)
                    all_sequences[sequence_key] = files
        
        # 生成总体统计信息
        self.generate_summary_report(all_sequences)
        
        print("-" * 50)
        print(f"✅ 完成！共生成 {len(all_sequences)} 个插值序列")
        print(f"总帧数: {sum(len(files) for files in all_sequences.values())}")
        
        return all_sequences
    
    def generate_summary_report(self, all_sequences: Dict[str, List[str]]) -> None:
        """生成总结报告"""
        report = {
            'generation_info': {
                'total_sequences': len(all_sequences),
                'interpolation_steps': self.interpolation_steps,
                'image_size': self.image_size,
                'total_frames': sum(len(files) for files in all_sequences.values())
            },
            'expressions': {name: {
                'params': {
                    'background_color': params.background_color,
                    'face_lines_count': len(params.face_lines),
                    'eyes_count': len(params.eyes),
                    'eyebrows_count': len(params.eyebrows),
                    'mouth_count': len(params.mouth),
                    'description': params.description
                }
            } for name, params in self.expressions.items()},
            'sequences': {key: len(files) for key, files in all_sequences.items()}
        }
        
        report_file = os.path.join(self.output_dir, 'generation_report.json')
        with open(report_file, 'w', encoding='utf-8') as f:
            json.dump(report, f, ensure_ascii=False, indent=2)
        
        print(f"📊 生成报告已保存: {report_file}")

def main():
    """主函数"""
    print("=" * 60)
    print("智能用药提醒机器人表情插值生成器")
    print("=" * 60)
    
    # 创建插值生成器
    interpolator = ExpressionInterpolator()
    
    # 显示表情列表
    print("\n📋 支持的表情类型:")
    for i, (name, params) in enumerate(interpolator.expressions.items(), 1):
        print(f"  {i}. {name} ({params.description})")
    
    print(f"\n🎯 将生成 {len(interpolator.expressions)} × {len(interpolator.expressions)-1} = {len(interpolator.expressions) * (len(interpolator.expressions)-1)} 个插值序列")
    
    # 自动开始生成
    print("\n🚀 开始自动生成图像序列...")
    
    # 开始生成
    try:
        all_sequences = interpolator.generate_all_interpolations()
        
        print("\n🎉 生成完成！")
        print(f"📁 输出目录: {os.path.abspath(interpolator.output_dir)}")
        print("\n📖 使用说明:")
        print("  - 每个子文件夹包含一对表情的插值序列")
        print("  - frame_000.png 是起始表情，frame_010.png 是目标表情")
        print("  - metadata.json 包含每帧的详细参数信息")
        print("  - generation_report.json 包含整体生成统计")
        
    except KeyboardInterrupt:
        print("\n⚠️  生成被用户中断。")
    except Exception as e:
        print(f"\n❌ 生成过程中出现错误: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
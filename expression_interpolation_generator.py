#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能用药提醒机器人表情插值生成器
功能：生成所有表情相互切换的中间插值表情并保存到对应文件夹
作者：QT_ROS_Dev
"""

import os
import json
import math
from PIL import Image, ImageDraw, ImageFont
from typing import Dict, List, Tuple, NamedTuple

class ExpressionParams(NamedTuple):
    """表情参数结构"""
    background_color: Tuple[int, int, int]  # RGB背景色
    text_color: Tuple[int, int, int]        # RGB文字色
    scale: float                            # 缩放比例
    opacity: float                          # 透明度
    emoji: str                              # 表情符号
    description: str                        # 描述

class ExpressionInterpolator:
    """表情插值生成器"""
    
    def __init__(self):
        self.expressions = {
            'Happy': ExpressionParams((144, 238, 144), (0, 100, 0), 1.2, 1.0, '😊', '开心'),
            'Caring': ExpressionParams((255, 182, 193), (139, 69, 19), 1.1, 0.9, '🤗', '关怀'),
            'Concerned': ExpressionParams((255, 165, 0), (139, 69, 19), 0.9, 0.8, '😟', '担忧'),
            'Encouraging': ExpressionParams((173, 216, 230), (25, 25, 112), 1.3, 1.0, '💪', '鼓励'),
            'Alert': ExpressionParams((255, 99, 71), (139, 0, 0), 1.1, 1.0, '⚠️', '警示'),
            'Sad': ExpressionParams((169, 169, 169), (105, 105, 105), 0.8, 0.7, '😢', '悲伤'),
            'Neutral': ExpressionParams((211, 211, 211), (105, 105, 105), 1.0, 0.8, '😐', '中性')
        }
        
        self.output_dir = 'expression_interpolations'
        self.image_size = (200, 200)
        self.interpolation_steps = 10  # 每对表情之间生成10个中间状态
        
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
    
    def interpolate_expressions(self, expr1: ExpressionParams, 
                              expr2: ExpressionParams, t: float) -> ExpressionParams:
        """表情参数插值"""
        # 应用平滑插值
        smooth_t = self.smooth_interpolation(max(0.0, min(1.0, t)))
        
        # 插值计算
        bg_color = self.interpolate_color(expr1.background_color, expr2.background_color, smooth_t)
        text_color = self.interpolate_color(expr1.text_color, expr2.text_color, smooth_t)
        scale = expr1.scale + smooth_t * (expr2.scale - expr1.scale)
        opacity = expr1.opacity + smooth_t * (expr2.opacity - expr1.opacity)
        
        # 表情符号在中点切换
        if smooth_t < 0.5:
            emoji = expr1.emoji
            description = f"{expr1.description}→{expr2.description}"
        else:
            emoji = expr2.emoji
            description = f"{expr1.description}→{expr2.description}"
            
        return ExpressionParams(bg_color, text_color, scale, opacity, emoji, description)
    
    def create_expression_image(self, params: ExpressionParams, 
                              filename: str) -> None:
        """创建表情图像"""
        # 创建图像
        img = Image.new('RGBA', self.image_size, (*params.background_color, int(255 * params.opacity)))
        draw = ImageDraw.Draw(img)
        
        # 绘制边框
        border_color = tuple(max(0, c - 50) for c in params.text_color)
        draw.rectangle([5, 5, self.image_size[0]-5, self.image_size[1]-5], 
                      outline=border_color, width=3)
        
        # 计算字体大小
        base_font_size = 48
        font_size = int(base_font_size * params.scale)
        
        try:
            # 尝试使用系统字体
            font = ImageFont.truetype("arial.ttf", font_size)
        except:
            try:
                # Windows系统字体
                font = ImageFont.truetype("C:/Windows/Fonts/arial.ttf", font_size)
            except:
                # 使用默认字体
                font = ImageFont.load_default()
        
        # 绘制表情符号
        text = params.emoji
        bbox = draw.textbbox((0, 0), text, font=font)
        text_width = bbox[2] - bbox[0]
        text_height = bbox[3] - bbox[1]
        
        x = (self.image_size[0] - text_width) // 2
        y = (self.image_size[1] - text_height) // 2
        
        draw.text((x, y), text, fill=params.text_color, font=font)
        
        # 保存图像
        img.save(filename, 'PNG')
    
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
                    'text_color': interpolated.text_color,
                    'scale': interpolated.scale,
                    'opacity': interpolated.opacity,
                    'emoji': interpolated.emoji
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
                    'text_color': params.text_color,
                    'scale': params.scale,
                    'opacity': params.opacity,
                    'emoji': params.emoji,
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
        print(f"  {i}. {name} ({params.description}) {params.emoji}")
    
    print(f"\n🎯 将生成 {len(interpolator.expressions)} × {len(interpolator.expressions)-1} = {len(interpolator.expressions) * (len(interpolator.expressions)-1)} 个插值序列")
    
    # 用户确认
    try:
        response = input("\n是否开始生成？(y/N): ").strip().lower()
        if response not in ['y', 'yes', '是']:
            print("已取消生成。")
            return
    except KeyboardInterrupt:
        print("\n已取消生成。")
        return
    
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
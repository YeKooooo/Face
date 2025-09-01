#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能用药提醒机器人表情系统 - Python Socket客户端测试脚本

功能:
1. 连接到Qt应用程序的Socket服务器
2. 发送emotion_output JSON数据
3. 测试不同表情类型的切换
4. 支持交互式和自动测试模式

使用方法:
    python python_socket_client_test.py

作者: QT_ROS_Dev
版本: 1.0
"""

import socket
import json
import time
import sys
from typing import Dict, Any

class EmotionOutputClient:
    """表情输出Socket客户端"""
    
    def __init__(self, host: str = "127.0.0.1", port: int = 8888):
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False
        
    def connect(self) -> bool:
        """连接到服务器"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.connected = True
            print(f"✅ 成功连接到服务器 {self.host}:{self.port}")
            return True
        except Exception as e:
            print(f"❌ 连接失败: {e}")
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.socket:
            self.socket.close()
            self.connected = False
            print("🔌 已断开连接")
    
    def send_emotion_output(self, expression_type: str, duration_ms: int = 3000, 
                           trigger_reason: str = "测试触发") -> bool:
        """发送表情输出数据"""
        if not self.connected:
            print("❌ 未连接到服务器")
            return False
        
        emotion_data = {
            "emotion_output": {
                "expression_type": expression_type,
                "duration_ms": duration_ms,
                "trigger_reason": trigger_reason
            }
        }
        
        try:
            json_str = json.dumps(emotion_data, ensure_ascii=False)
            # 追加换行作为消息分隔，便于Qt端按行解析
            self.socket.send((json_str + "\n").encode('utf-8'))
            print(f"📤 发送数据: {json_str}")
            return True
        except Exception as e:
            print(f"❌ 发送失败: {e}")
            return False

    def send_asr(self, text: str, is_final: bool = True) -> bool:
        """发送ASR识别结果到Qt端（type=asr）"""
        if not self.connected:
            print("❌ 未连接到服务器")
            return False
        payload = {"type": "asr", "text": text, "isFinal": bool(is_final)}
        try:
            json_str = json.dumps(payload, ensure_ascii=False)
            self.socket.send((json_str + "\n").encode('utf-8'))
            print(f"🎙️ 发送ASR: {json_str}")
            return True
        except Exception as e:
            print(f"❌ 发送ASR失败: {e}")
            return False

    def send_llm_stream(self, full_text: str, tokens_per_chunk: int = 3, interval_ms: int = 40) -> bool:
        """模拟LLM逐字流发送（type=llm_stream）"""
        if not self.connected:
            print("❌ 未连接到服务器")
            return False
        try:
            i = 0
            while i < len(full_text):
                chunk = full_text[i:i+tokens_per_chunk]
                msg = {"type": "llm_stream", "text": chunk, "isFinal": False}
                self.socket.send((json.dumps(msg, ensure_ascii=False) + "\n").encode("utf-8"))
                print(f"🧠 LLM分片: {chunk}")
                i += tokens_per_chunk
                time.sleep(interval_ms / 1000.0)
            # 发送完成标记
            final_msg = {"type": "llm_stream", "text": "", "isFinal": True}
            self.socket.send((json.dumps(final_msg, ensure_ascii=False) + "\n").encode("utf-8"))
            print("✅ 发送LLM完成标记")
            return True
        except Exception as e:
            print(f"❌ 发送LLM失败: {e}")
            return False

    def run_interactive_test(self):
        """运行交互式测试"""
        print("\n🎭 交互式表情测试模式")
        print("支持的表情类型:")
        expressions = [
            ("happy", "开心", "😊"),
            ("caring", "关怀", "🤗"),
            ("concerned", "担忧", "😟"),
            ("encouraging", "鼓励", "💪"),
            ("alert", "警觉", "⚠️"),
            ("sad", "悲伤", "😢"),
            ("neutral", "中性", "😐")
        ]
        
        for i, (eng, chn, emoji) in enumerate(expressions, 1):
            print(f"  {i}. {eng} ({chn}) {emoji}")
        
        print("\n输入命令:")
        print("  数字1-7: 发送对应表情")
        print("  'asr': 发送一条ASR文本")
        print("  'llm': 模拟LLM流式回复")
        print("  'mix': 混合场景（ASR->LLM->表情）")
        print("  'auto': 运行自动测试")
        print("  'quit': 退出程序")
        
        while True:
            try:
                cmd = input("\n请输入命令: ").strip().lower()
                
                if cmd == 'quit':
                    break
                elif cmd == 'auto':
                    self.run_auto_test()
                elif cmd == 'asr':
                    self.run_asr_demo()
                elif cmd == 'llm':
                    self.run_llm_demo()
                elif cmd == 'mix':
                    self.run_mixed_demo()
                elif cmd.isdigit() and 1 <= int(cmd) <= 7:
                    idx = int(cmd) - 1
                    expr_type, expr_name, emoji = expressions[idx]
                    duration = int(input(f"持续时间(毫秒,默认3000): ") or "3000")
                    reason = input(f"触发原因(默认'手动测试'): ") or "手动测试"
                    
                    print(f"🎭 切换到{expr_name}表情 {emoji}")
                    self.send_emotion_output(expr_type, duration, reason)
                else:
                    print("❌ 无效命令，请重新输入")
                    
            except KeyboardInterrupt:
                print("\n👋 用户中断")
                break
            except Exception as e:
                print(f"❌ 输入错误: {e}")

    def run_auto_test(self):
        """运行自动测试序列"""
        print("\n🤖 自动测试模式启动")
        
        test_cases = [
            ("caring", 2500, "用户询问用药时间"),
            ("alert", 4000, "检测到用药延迟"),
            ("encouraging", 3000, "用户完成用药"),
            ("happy", 2000, "用药记录更新成功"),
            ("concerned", 3500, "检测到副作用症状"),
            ("neutral", 2000, "系统待机状态")
        ]
        
        for i, (expr_type, duration, reason) in enumerate(test_cases, 1):
            print(f"\n📋 测试 {i}/{len(test_cases)}: {reason}")
            self.send_emotion_output(expr_type, duration, reason)
            
            # 等待表情持续时间 + 额外间隔
            wait_time = (duration / 1000) + 1
            print(f"⏳ 等待 {wait_time:.1f} 秒...")
            time.sleep(wait_time)
        
        print("\n✅ 自动测试完成")

    def run_stress_test(self, count: int = 10):
        """运行压力测试"""
        print(f"\n⚡ 压力测试模式 - 发送 {count} 条消息")
        
        expressions = ["happy", "caring", "concerned", "encouraging", "alert", "sad", "neutral"]
        
        for i in range(count):
            expr_type = expressions[i % len(expressions)]
            duration = 1000 + (i * 100)  # 递增持续时间
            reason = f"压力测试消息 {i+1}/{count}"
            
            print(f"📤 [{i+1:2d}/{count}] {expr_type} ({duration}ms)")
            self.send_emotion_output(expr_type, duration, reason)
            time.sleep(0.5)  # 短间隔
        
        print("\n✅ 压力测试完成")

    def run_asr_demo(self):
        """交互式：发送一条ASR识别文本"""
        print("\n🎙️ ASR 文本模拟")
        text = input("ASR文本(默认: 请提醒我按时吃药): ").strip() or "请提醒我按时吃药"
        final_str = input("是否final(true/false, 默认true): ").strip().lower() or "true"
        is_final = final_str in ("1", "true", "t", "y", "yes")
        self.send_asr(text, is_final)

    def run_llm_demo(self):
        """交互式：模拟LLM逐字流式输出"""
        print("\n🧠 LLM 流式输出模拟")
        answer = input("LLM回复文本(默认: 好的，我会按时提醒您按医嘱用药。): ").strip() or "好的，我会按时提醒您按医嘱用药。"
        tpc_str = input("每片字符数(默认3): ").strip() or "3"
        itv_str = input("分片间隔毫秒(默认40): ").strip() or "40"
        try:
            tpc = int(tpc_str)
        except ValueError:
            tpc = 3
        try:
            itv = int(itv_str)
        except ValueError:
            itv = 40
        self.send_llm_stream(answer, tokens_per_chunk=tpc, interval_ms=itv)

    def run_mixed_demo(self):
        """一键混合场景：ASR -> Caring -> LLM -> Happy"""
        print("\n🔀 混合场景：ASR -> 表情Caring -> LLM流 -> 表情Happy")
        self.send_asr("请问我今天的降压药什么时候吃？", True)
        time.sleep(0.2)
        self.send_emotion_output("caring", 3000, "用户询问用药时间")
        time.sleep(0.5)
        answer = "建议在早餐后30分钟服用，并配合温水。若出现不适，请及时就医。"
        self.send_llm_stream(answer, tokens_per_chunk=3, interval_ms=40)
        time.sleep(0.2)
        self.send_emotion_output("happy", 2000, "已给出建议")
        print("✅ 混合场景完成")

def main():
    """主函数"""
    print("🎭 智能用药提醒机器人表情系统 - Socket客户端测试")
    print("=" * 50)
    
    # 获取服务器地址
    host = input("服务器地址 (默认 127.0.0.1): ").strip() or "127.0.0.1"
    port_str = input("服务器端口 (默认 8888): ").strip() or "8888"
    
    try:
        port = int(port_str)
    except ValueError:
        print("❌ 端口号无效，使用默认端口 8888")
        port = 8888
    
    # 创建客户端并连接
    client = EmotionOutputClient(host, port)
    
    if not client.connect():
        print("\n💡 请确保:")
        print("  1. Qt应用程序正在运行")
        print("  2. Socket服务器已启动")
        print("  3. 防火墙允许连接")
        return
    
    try:
        # 选择测试模式
        print("\n选择测试模式:")
        print("  1. 交互式测试 (手动控制)")
        print("  2. 自动测试 (预设序列)")
        print("  3. 压力测试 (批量发送)")
        print("  4. ASR/LLM 模拟 (流式+ASR)")
        print("  5. 混合场景 (ASR->LLM->表情)")
        
        mode = input("\n请选择模式 (1-5, 默认1): ").strip() or "1"
        
        if mode == "1":
            client.run_interactive_test()
        elif mode == "2":
            client.run_auto_test()
        elif mode == "3":
            count_str = input("压力测试消息数量 (默认10): ").strip() or "10"
            count = int(count_str) if count_str.isdigit() else 10
            client.run_stress_test(count)
        elif mode == "4":
            client.run_asr_demo()
            client.run_llm_demo()
        elif mode == "5":
            client.run_mixed_demo()
        else:
            print("❌ 无效模式，使用交互式测试")
            client.run_interactive_test()
            
    except KeyboardInterrupt:
        print("\n👋 程序被用户中断")
    except Exception as e:
        print(f"\n❌ 程序异常: {e}")
    finally:
        client.disconnect()
        print("\n👋 测试结束")

if __name__ == "__main__":
    main()
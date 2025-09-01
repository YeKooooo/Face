# 智能用药提醒机器人表情系统

## 程序简介

本程序是一个基于Qt 5.13的跨平台GUI应用，专为智能用药提醒机器人设计的表情显示系统。支持多种表情状态切换和平滑过渡动画。

## 主要功能

### 表情系统
- **支持6种基础表情**：开心(Happy)、关怀(Caring)、担忧(Concerned)、鼓励(Encouraging)、警示(Alert)、悲伤(Sad)、中性(Neutral)
- **平滑过渡动画**：任意两种表情间的插值动画，共42种过渡序列
- **双重渲染模式**：
  - 图像序列模式：使用预生成的PNG动画帧
  - 参数插值模式：实时计算表情参数变化

### 网络通信
- **TCP Socket服务器**：监听端口8080
- **JSON API接口**：接收表情切换指令
- **实时状态反馈**：显示连接状态和接收到的数据

## 使用方法

### 启动程序
1. 双击 `faceshiftDemo.exe` 启动程序
2. 程序会自动启动TCP服务器监听8080端口
3. 界面显示当前表情状态和网络连接信息

### 表情控制

#### 方式1：界面按钮控制
- 点击界面上的表情按钮直接切换表情
- 支持的表情：Happy、Caring、Concerned、Encouraging、Alert、Sad、Neutral

#### 方式2：网络API控制
发送JSON格式数据到TCP端口8080：

```json
{
    "emotion": "Happy",
    "intensity": 0.8,
    "duration": 2.0
}
```

参数说明：
- `emotion`: 目标表情名称
- `intensity`: 表情强度 (0.0-1.0)
- `duration`: 过渡时间 (秒)

### 测试客户端

可以使用Python测试客户端发送指令：

```python
import socket
import json

def send_emotion(emotion, intensity=0.8, duration=2.0):
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect(('localhost', 8080))
    
    data = {
        "emotion": emotion,
        "intensity": intensity,
        "duration": duration
    }
    
    client.send(json.dumps(data).encode())
    client.close()

# 示例使用
send_emotion("Happy")
```

## 技术特性

### 架构设计
- **Qt 5.13框架**：跨平台GUI支持
- **多线程安全**：网络通信与UI渲染分离
- **模块化设计**：表情渲染、网络通信、动画控制独立模块

### 动画系统
- **预生成动画**：505个PNG帧文件，覆盖42种表情过渡
- **元数据驱动**：每个动画序列包含完整的参数信息
- **自适应回退**：图像序列不可用时自动切换到参数插值

### 性能优化
- **图像缓存**：预加载常用动画序列到内存
- **异步渲染**：非阻塞的动画播放机制
- **资源管理**：智能的内存和文件资源管理

## 文件结构

```
deploy/
├── faceshiftDemo.exe          # 主程序
├── *.dll                      # Qt运行库
├── expression_interpolations/ # 动画资源目录
│   ├── Happy_to_Sad/         # 表情过渡序列
│   ├── Alert_to_Caring/      # ...
│   └── generation_report.json # 动画生成报告
├── platforms/                # Qt平台插件
├── imageformats/            # 图像格式支持
├── translations/            # 多语言支持
└── README.md               # 本文档
```

## 系统要求

- **操作系统**：Windows 7/8/10/11 (64位)
- **内存**：最低512MB，推荐1GB以上
- **存储空间**：约100MB
- **网络**：支持TCP/IP协议栈

## 故障排除

### 常见问题

1. **程序无法启动**
   - 检查是否安装了Visual C++ Redistributable
   - 确认系统支持Qt 5.13运行环境

2. **网络连接失败**
   - 检查端口8080是否被其他程序占用
   - 确认防火墙设置允许程序网络访问

3. **动画显示异常**
   - 确认expression_interpolations目录完整
   - 检查PNG文件是否损坏

### 日志信息
程序运行时会在控制台输出详细的状态信息，包括：
- 网络连接状态
- 表情切换事件
- 动画加载状态
- 错误和警告信息

## 开发信息

- **开发框架**：Qt 5.13.2 + MinGW 7.3
- **编程语言**：C++ 17
- **构建工具**：qmake + mingw32-make
- **版本控制**：支持Git版本管理

---
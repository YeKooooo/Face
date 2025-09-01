# EmotionOutput API 使用说明

## 概述

本API实现了基于 `emotion_output` 字段的智能用药提醒机器人表情控制接口，支持JSON格式的表情切换指令和持续时间控制。

## 核心功能

### 1. 数据结构

```cpp
struct EmotionOutput {
    QString expression_type;    // 表情类型
    int duration_ms;           // 持续时间(毫秒)
    QString trigger_reason;    // 触发原因
};
```

### 2. 公共接口

```cpp
// JSON字符串接口
void processEmotionOutput(const QString& jsonString);

// 直接结构体接口
void processEmotionOutput(const EmotionOutput& emotionData);
```

## 支持的表情类型

| 英文名称 | 中文名称 | 应用场景 |
|---------|---------|----------|
| `happy` | 开心 | 成功完成用药、健康状况良好 |
| `caring` | 关怀 | 用药提醒、健康关怀、温暖陪伴 |
| `concerned` | 担忧 | 用户延迟用药、健康指标异常 |
| `encouraging` | 鼓励 | 激励用户坚持治疗、克服困难 |
| `alert` | 警觉 | 紧急情况、重要提醒 |
| `sad` | 难过 | 用户身体不适、治疗效果不佳 |
| `neutral` | 中性 | 默认状态、日常交互 |

## 使用示例

### 示例1: JSON格式调用

```cpp
// 用户身份询问场景
QString jsonData = R"({
    "transcription": "你知道我是谁吗？",
    "speaker": {
        "id": "USR1755675801",
        "name": "张三",
        "status": "registered",
        "confidence": 0.75
    },
    "emotion_output": {
        "expression_type": "caring",
        "duration_ms": 3000,
        "trigger_reason": "user_identity_inquiry"
    }
})";

widget->processEmotionOutput(jsonData);
```

### 示例2: 直接结构体调用

```cpp
// 用药完成场景
EmotionOutput emotionData("happy", 2500, "medication_completed");
widget->processEmotionOutput(emotionData);
```

### 示例3: 紧急情况

```cpp
QString emergencyJson = R"({
    "emotion_output": {
        "expression_type": "alert",
        "duration_ms": 6000,
        "trigger_reason": "health_emergency"
    }
})";

widget->processEmotionOutput(emergencyJson);
```

## 持续时间控制

- **正数值**: 表情持续指定毫秒数后自动恢复到上一个表情或中性表情
- **-1**: 永久切换，不自动恢复
- **0或未指定**: 使用默认值3000毫秒

## 特性说明

### 1. 线程安全
- 使用Qt的信号槽机制确保UI线程安全
- 支持从任意线程调用接口

### 2. 表情切换逻辑
- 自动停止当前表情的持续时间定时器
- 记录上一个表情状态用于恢复
- 支持图像序列和参数插值两种动画模式

### 3. 错误处理
- JSON解析错误时输出调试信息
- 未知表情类型自动回退到中性表情
- 完整的日志记录系统

### 4. 调试支持
```cpp
// 启用调试输出查看表情切换日志
qDebug() << "[表情切换] 触发原因:" << reason << "目标表情:" << typeStr;
```

## 集成指南

### 1. 在现有项目中使用

```cpp
#include "widget.h"

// 创建Widget实例
Widget* emotionWidget = new Widget();

// 处理语音识别结果
void onSpeechRecognitionResult(const QString& jsonResult) {
    emotionWidget->processEmotionOutput(jsonResult);
}
```

### 2. 自定义表情映射

如需添加新的表情类型，修改 `stringToExpressionType()` 方法：

```cpp
ExpressionType Widget::stringToExpressionType(const QString& typeString)
{
    QString lowerType = typeString.toLower();
    
    // 添加新的表情类型映射
    if (lowerType == "custom_emotion") {
        return ExpressionType::CustomEmotion;
    }
    
    // ... 现有映射逻辑
}
```

## 性能优化

- JSON解析采用Qt原生解析器，性能优异
- 表情切换使用硬件加速动画
- 图像序列预加载机制减少切换延迟
- 单次定时器避免资源泄漏

## 注意事项

1. **JSON格式**: 必须包含 `emotion_output` 字段
2. **表情类型**: 支持英文和中文名称（不区分大小写）
3. **持续时间**: 建议范围1000-10000毫秒
4. **触发原因**: 用于日志记录和调试分析
5. **线程调用**: 可从任意线程安全调用

## 错误码说明

- JSON解析失败: 输出解析错误信息到调试控制台
- 表情类型未知: 自动使用中性表情并输出警告
- 持续时间无效: 使用默认3000毫秒
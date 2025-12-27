# UML 设计文档

本目录包含银行分行模拟器的UML设计图表，帮助理解系统架构和业务流程。

## 📊 图表文件

### 类图 (Class Diagrams)
- **bank_classes.puml** - PlantUML源文件
- **bank_classes.png** - 生成的类图图片

### 序列图 (Sequence Diagrams)
- **bank_sequence_start_day.puml** - 银行营业日开始流程源文件
- **bank_sequence_start_day.png** - 银行营业日开始流程图片
- **bank_sequence_personal_appeal.puml** - 客户办理业务流程源文件
- **bank_sequence_personal_appeal.png** - 客户办理业务流程图片（需生成）

## 🎨 图表说明

### 系统架构类图

![系统架构类图](bank_classes.png)

**核心组件：**
- **BankSystem**: 银行系统核心类，管理所有业务逻辑
- **Domain Model**: 领域模型包，包含所有业务实体
- **Account**: 账户实体，支持多种货币和类型
- **Client**: 客户实体，支持个人和法人客户
- **Deposit/Loan**: 存款和贷款产品
- **ClientAccount**: 客户与账户的关联关系

**设计特点：**
- 清晰的包结构分离
- 完整的实体关系映射
- 支持枚举类型的业务规则
- 丰富的系统状态管理

### 银行营业日开始序列图

![银行营业日开始](bank_sequence_start_day.png)

**流程说明：**
1. 操作员触发银行营业日开始事件
2. 系统标准化开业时间为8:00
3. 重置所有工作岗位状态
4. 返回确认信息

### 客户办理业务序列图

**流程说明：**
1. 客户提交Personal Appeal事件
2. 系统解析客户信息和操作列表
3. 验证客户身份（新客户自动注册）
4. 逐项处理客户请求：
   - 权限检查
   - 业务规则验证
   - 手续费计算
   - 账户创建
   - 交易日志记录

## 🛠️ 生成图表

### 方式一：使用脚本（推荐）
```batch
.\generate_uml.bat
```

### 方式二：手动生成
```batch
# 需要先安装PlantUML
plantuml docs/bank_classes.puml
plantuml docs/bank_sequence_start_day.puml
plantuml docs/bank_sequence_personal_appeal.puml
```

### 方式三：在线生成
访问 [PlantUML在线编辑器](http://www.plantuml.com/plantuml/)，复制.puml文件内容进行在线生成。

## 📝 修改图表

1. 编辑对应的.puml源文件
2. 运行生成脚本或PlantUML命令
3. 检查生成的.png文件
4. 更新相关文档中的图表引用

## 🔗 相关文档

- **../README.md** - 项目主文档
- **../新手指导.md** - 开发指导
- **../第二题.md** - 完整题目要求

---

**这些UML图表是理解银行分行模拟器系统设计的重要工具，建议结合代码一起学习。**
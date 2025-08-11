# 不同的模型需要不同的输出，MNN框架是怎么处理这样的不同的输入喂给大模型的

● MNN框架通过以下机制处理不同模型的输入适配：

1. 统一Tokenizer架构

- 工厂模式：Tokenizer::createTokenizer()根据模型类型动态创建对应tokenizer
- 四种实现：Sentencepiece、Tiktoken、BERT、Huggingface，覆盖主流LLM需求

2. 模型特定适配器

- 配置驱动：通过config.json定义模型专属的token映射和特殊token
- 模板系统：Jinja2风格的prompt模板，支持Baichuan、ChatGLM、DeepSeek等不同格式
- 角色格式：System/User/Assistant三段式对话格式自动适配

3. 输入处理流水线

- 多轮对话：自动处理聊天历史，维护上下文token序列
- 特殊token管理：统一处理起始、结束、未知、填充等token
- 流式处理：支持实时token化，适合移动端聊天应用

4. 跨平台支持

- C++核心：高性能原生实现
- Python封装：PyMNN提供tokenizer_encode/decode接口
- 移动端集成：Android/iOS原生应用通过JNI/Swift桥接

这种架构使MNN能够无缝支持不同架构的大模型，只需加载对应配置文件即可适配新模型。
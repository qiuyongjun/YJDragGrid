# QtDragGrid 贡献指南

感谢你对 QtDragGrid 的关注。本文说明如何提交 Issue 和 Pull Request。

## 提交 Issue

提交前请先搜索已有 Issue，避免重复反馈。

报告缺陷时请尽量提供：

- Qt 版本和平台（Windows、macOS、Linux）
- CMake 版本
- 最小复现步骤
- 期望行为和实际行为
- 相关代码片段或错误信息

## 提交 Pull Request

1. Fork 仓库，并从 `master` 创建功能分支。
2. 修改代码时保持现有代码风格。
3. 如涉及行为变化，请同步更新测试和文档。
4. 确认项目能在本地构建。
5. 使用 `ctest --test-dir build --output-on-failure` 运行测试。
6. 新增行为请在 `tests/Test<Component>.cpp` 中补充单元测试。
7. 提交信息应清楚说明变更内容。
8. 创建 Pull Request，并按模板填写必要信息。

## 代码风格

- C++17
- 遵循现有缩进和命名习惯。
- 公共 API 保持简洁，并补充必要说明。
- 只为复杂逻辑、业务规则和边界条件添加注释。

## License / 许可证

提交贡献即表示你同意贡献内容采用 [MIT License](LICENSE) 授权。

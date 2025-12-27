@echo off
echo 正在构建项目...
cmake -S . -B build
cmake --build build --config Release

echo.
echo 正在运行单元测试...
ctest --test-dir build -C Release --output-on-failure

echo.
echo 正在运行银行模拟器演示...
build\Release\bank_sim.exe < demo_input.txt > demo_output.txt 2> demo_log.txt

echo.
echo 演示完成！查看以下文件：
echo - demo_output.txt: 业务处理结果
echo - demo_log.txt: 交易日志
echo.

echo 输出结果：
echo ============
type demo_output.txt
echo.
echo 交易日志：
echo ============
type demo_log.txt

pause
@echo off
echo 正在构建项目...
cmake -S . -B build
cmake --build build --config Release

echo.
echo 正在运行复杂银行模拟器演示...
build\Release\bank_sim.exe < complex_demo.txt > complex_output.txt 2> complex_log.txt

echo.
echo 复杂演示完成！查看以下文件：
echo - complex_output.txt: 业务处理结果
echo - complex_log.txt: 交易日志
echo.

echo 输出结果：
echo ============
type complex_output.txt
echo.
echo 交易日志：
echo ============
type complex_log.txt

echo.
echo 演示说明：
echo - 展示了多种客户类型（个人、VIP个人、法人、VIP法人）
echo - 展示了多种货币（RUB、USD、EUR）
echo - 展示了不同的手续费规则
echo - 展示了新客户注册限制

pause
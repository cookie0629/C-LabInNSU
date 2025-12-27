@echo off
echo 正在生成UML图表...

echo.
echo 检查PlantUML是否可用...
where plantuml >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo 错误：未找到PlantUML命令
    echo 请先安装PlantUML：
    echo 1. 下载plantuml.jar
    echo 2. 安装Java
    echo 3. 创建plantuml.bat批处理文件
    echo 或者使用在线版本：http://www.plantuml.com/plantuml/
    pause
    exit /b 1
)

echo.
echo 生成类图...
plantuml docs/bank_classes.puml
if %ERRORLEVEL% EQU 0 (
    echo 类图生成成功：docs/bank_classes.png
) else (
    echo 类图生成失败
)

echo.
echo 生成序列图...
plantuml docs/bank_sequence_start_day.puml
if %ERRORLEVEL% EQU 0 (
    echo 银行营业日序列图生成成功：docs/bank_sequence_start_day.png
) else (
    echo 银行营业日序列图生成失败
)

plantuml docs/bank_sequence_personal_appeal.puml
if %ERRORLEVEL% EQU 0 (
    echo 客户办理业务序列图生成成功：docs/bank_sequence_personal_appeal.png
) else (
    echo 客户办理业务序列图生成失败
)

echo.
echo UML图表生成完成！
echo 图表文件位于docs/目录下
pause
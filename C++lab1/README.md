# 现代化项目

## 构建命令


PowerShell 不支持反斜杠 `\` 续行，请使用反引号 `` ` `` 续行，或改为单行命令。
```powershell
# 清理 CMake 缓存（切换生成器时需要）
# Очистка кэша CMake (требуется при переключении генераторов)
Remove-Item -Recurse -Force build/Debug/CMakeFiles
Remove-Item -Force build/Debug/CMakeCache.txt
```
```powershell
# 1) 检测配置 + 安装依赖（Debug）
# Определение зависимостей конфигурации и установки (отладка)
conan profile detect --force
conan install . --build=missing -s build_type=Debug -of build/Debug

# 2) 生成（使用 VS 生成器，并指定 x64）
# Сгенерировать (используйте генератор VS и укажите x64)
cmake -B build/Debug `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="build/Debug/build/generators/conan_toolchain.cmake" `
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW


# 3) 编译（多配置生成器需指定 --config）
# Скомпилировать (для генератора нескольких конфигураций необходимо указать --config)
cmake --build build/Debug --config Debug -j

# 4) 运行测试
# Запустите тест
ctest --test-dir build/Debug -C Debug --output-on-failure

# 5) 安装（Windows 无需 sudo；如无管理员权限建议指定前缀）
# Установка
cmake --install build/Debug --config Debug --prefix "F:\C++First\install"

# 6) 运行示例（注意 PowerShell 需使用调用运算符 &）
# Запустите пример
& "F:\C++First\install\bin\compressor.exe" zlib "test_string"
```



### WSL（Windows Subsystem for Linux）
在 WSL 中可直接使用上面的 Bash 命令（无需修改）。安装步骤仍需 `sudo`：
```bash
sudo cmake --install build/Debug
```
### 1. 调试构建：安装依赖 + 配置 + 编译  Linux
```bash
conan profile detect --force
conan install . --build=missing -s build_type=Debug
cmake -B build/Debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
cmake --build build/Debug -j
```

### 2. 运行测试并生成报告
```bash
ctest --test-dir build/Debug --output-on-failure
ls build/Debug/test_results.xml
```

### 3. 生成文档（Doxygen）
```bash
cmake --build build/Debug --target docs
ls build/Debug/docs/html
```

### 4. 代码格式化与静态分析
```bash
cmake --build build/Debug --target format
cmake --build build/Debug --target tidy
```

### 5. 安装产物
```bash
sudo cmake --install build/Debug
which compressor
/usr/local/bin/compressor zlib "test_string"
/usr/local/bin/compressor bzip "test_string"
ls -l /usr/local/lib/liblibcompressor.a
ls -l /usr/local/include/libcompressor/libcompressor.hpp
```

#!/bin/bash
set -e  # エラー時にスクリプトを停止

export CC="gcc"
export CXX="g++"

#arm windows
if [ "$(uname -m)" = "aarch64" ]; then
export CC="x86_64-linux-gnu-gcc"	
export CXX="x86_64-linux-gnu-g++"
fi

echo "Compiling Lambda Server..."

${CXX} -o lambda_server src/main/simple_test_server.cpp src/main/mysql_dynamic_loading.cpp \
    -pthread -ldl -Wl,-rpath,. --std=c++2a \
    -I src/main -I src/headeronly -I src/mysql_8_4

${CXX} -o lambda-adapter src/main/lamnda_web_adapter.cpp \
    -pthread -ldl -Wl,-rpath,. --std=c++2a \
    -I src/main -I src/headeronly -I src/mysql_8_4

chmod a+x lambda_server
chmod a+x lambda-adapter
chmod a+x bootstrap

echo "Copyging files..."

rm -r -f image
mkdir image
cd image
mkdir lib
#cp -r ../html .

cp ../lambda_server .
#cp ../lib*.so lib/
cp ../bootstrap .
cp ../lambda-adapter .
cp ../images.jpg .
cp ../libmysqlclient.so .

# zipファイルを作成（上位ディレクトリに）
echo "Creating Lambda deployment package..."
zip -r ../build_image.zip * 
echo "Build completed: build_image.zip created in impl/ directory"

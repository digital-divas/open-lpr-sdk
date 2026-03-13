BUILD_DIR=build

echo "Building"

rm -rf $BUILD_DIR
mkdir $BUILD_DIR
cd $BUILD_DIR

cmake ..

cmake --build . --config Release
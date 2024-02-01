FLAGS=$(cat compile_flags.txt)

mkdir -p build
# compile the shaders
clang -o build/engine src/*.c $FILES $FLAGS 
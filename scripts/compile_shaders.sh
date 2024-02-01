mkdir -p ./build/shaders
glslc src/shaders/shader.vert -o build/shaders/vertex.spv
glslc src/shaders/shader.frag -o build/shaders/fragment.spv


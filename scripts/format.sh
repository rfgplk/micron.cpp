echo "Formatting all .hpp, .cpp., .h, and .c files."
find src/ -type f  -iname '*.hpp' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.c' | xargs clang-format -i

echo "Formatting all .hpp, .cpp., .h, and .c files."
find  -iname '*.hpp' -o -iname '*.cpp' -o -iname '*.h' -o -iname '*.c' | xargs clang-format -i

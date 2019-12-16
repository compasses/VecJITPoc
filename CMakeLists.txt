cmake_minimum_required(VERSION 3.12)
project(homework_cpp)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-O2")

add_executable(homework_cpp main.cpp ASTNode.h RowData.h Evaluator.h dummy/DummyEvaluator.h dummy/ColumnVecEvaluator.h)

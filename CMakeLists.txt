cmake_minimum_required(VERSION 3.13.0)

project(chipSmith VERSION 0.9)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


set(SOURCES
  # chipSmith
  src/chipsmith/main.cpp
  src/chipsmith/toolFlow.cpp
  src/chipsmith/genFunc.cpp
  src/chipsmith/ParserLef.cpp
  src/chipsmith/ParserDef.cpp
  src/chipsmith/chipFill.cpp

  # GDScpp library
  src/gdscpp/gdsCpp.cpp
  src/gdscpp/gdsParser.cpp
  src/gdscpp/gdsForge.cpp
  src/gdscpp/gdsImport.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Ensures that the header files of the project is included
target_include_directories(${PROJECT_NAME} PRIVATE
  ${PROJECT_SOURCE_DIR}/include
)
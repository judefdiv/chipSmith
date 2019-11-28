cmake_minimum_required(VERSION 2.6)
project(chipSmith)

set(SOURCES
  # chipSmith
  src/chipsmith/main.cpp
  src/chipsmith/toolFlow.cpp
  src/chipsmith/genFunc.cpp
  src/chipsmith/ParserLef.cpp

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


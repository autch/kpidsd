cmake_minimum_required(VERSION 3.21)
project(kpidsd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 壊れたファイル・細工されたファイルの検証用 (MSVC のみ)。すべてを AddressSanitizer 付きでビルドし、
# libFuzzer の fuzz-decoder を作る。配布用のビルドでは使わないこと (別のビルドディレクトリで使う)
option(KPIDSD_FUZZ "Build everything with AddressSanitizer and the fuzz-decoder (MSVC only)" OFF)
if(KPIDSD_FUZZ)
  if(NOT MSVC)
    message(FATAL_ERROR "KPIDSD_FUZZ is supported only with MSVC")
  endif()
  add_compile_options(/fsanitize=address /Zi)
  add_link_options(/INCREMENTAL:NO /DEBUG)
endif()
# MSVC では、KbMedia Player 本体付属のプラグインと同じく VC++ ランタイムを動的リンクする (既定の /MD)。
# MinGW (開発中の確認用) では libstdc++ などの DLL に依存しないよう静的リンクする

function(kpidsd_warnings target)
  target_compile_definitions(${target} PRIVATE UNICODE _UNICODE NOMINMAX)
  if(MSVC)
    target_compile_options(${target} PRIVATE /utf-8 /W3)
  else()
    # SDK ヘッダの __declspec(uuid()) は kpi_mingw_uuid.h で補うので、gcc の警告は抑える
    target_compile_options(${target} PRIVATE -Wall $<$<CXX_COMPILER_ID:GNU>:-Wno-attributes>
      $<$<CXX_COMPILER_ID:Clang>:-fms-extensions>)
  endif()
endfunction()

# DSF / DSDIFF / WSD のパーサー
add_library(libdsd STATIC
  libdsd/src/CDFFFile.cpp
  libdsd/src/CDSFFile.cpp
  libdsd/src/CLargeFile.cpp
  libdsd/src/CWSDFile.cpp
  libdsd/include/CAbstractFile.h
  libdsd/include/CDFFFile.h
  libdsd/include/CDSFFile.h
  libdsd/include/CLargeFile.h
  libdsd/include/CWSDFile.h
  libdsd/include/dff_types.h
  libdsd/include/dsf_types.h
  libdsd/include/wsd_types.h
  libdsd/stdafx.h
)
target_include_directories(libdsd PRIVATE libdsd PUBLIC libdsd/include)
kpidsd_warnings(libdsd)

# 両方のプラグインで使う、IKpiFile から DSD を読む部分とタグ
add_library(kpidsd_common STATIC
  common/DsdSource.cpp
  common/DsdSource.h
  common/KpiFileAdapter.h
  common/KpiPartialFile.h
  common/kpi_mingw_uuid.h
  common/stdafx.h
)
target_include_directories(kpidsd_common PUBLIC common kpisdk)
target_link_libraries(kpidsd_common PUBLIC libdsd)
kpidsd_warnings(kpidsd_common)

function(kpidsd_plugin name)
  add_library(${name} SHARED ${ARGN})
  target_include_directories(${name} PRIVATE ${name})
  target_link_libraries(${name} PRIVATE kpidsd_common)
  kpidsd_warnings(${name})
  if(NOT MSVC)
    target_link_options(${name} PRIVATE -static $<$<STREQUAL:${CMAKE_SIZEOF_VOID_P},4>:-Wl,--kill-at>)
  endif()
  set_target_properties(${name} PROPERTIES
    OUTPUT_NAME ${name}
    PREFIX ""
    SUFFIX ".kpi"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endfunction()

kpidsd_plugin(kpidop
  kpidop/kpidop.cpp
  kpidop/kpidop.def
  kpidop/KpiDopDecoder.cpp
  kpidop/KpiDopDecoder.h
  kpidop/KpiDopDecoderModule.cpp
  kpidop/KpiDopDecoderModule.h
)

kpidsd_plugin(kpid2p
  kpid2p/kpid2p.cpp
  kpid2p/kpid2p.def
  kpid2p/DsdDecimator.cpp
  kpid2p/DsdDecimator.h
  kpid2p/KpiD2pDecoder.cpp
  kpid2p/KpiD2pDecoder.h
  kpid2p/KpiD2pDecoderModule.cpp
  kpid2p/KpiD2pDecoderModule.h
)

if(KPIDSD_FUZZ)
  add_executable(fuzz-decoder
    tools/fuzz-decoder.cpp
    kpidop/KpiDopDecoder.cpp
    kpid2p/KpiD2pDecoder.cpp
    kpid2p/DsdDecimator.cpp
  )
  target_include_directories(fuzz-decoder PRIVATE kpidop kpid2p)
  target_link_libraries(fuzz-decoder PRIVATE kpidsd_common)
  kpidsd_warnings(fuzz-decoder)
  target_compile_options(fuzz-decoder PRIVATE /fsanitize=fuzzer)
  set_target_properties(fuzz-decoder PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# 検証用ホストとテスト
option(KPIDSD_BUILD_TOOLS "Build kpi-render and the tests" ON)
# 手元の DSF/DSDIFF/WSD を置いたディレクトリ。指定すると、その中のファイルでも seek-check を行う
set(KPIDSD_TEST_DATA "" CACHE PATH "Directory of DSF/DSDIFF/WSD files for seek-check")

if(KPIDSD_BUILD_TOOLS)
  add_executable(kpi-render tools/kpi-render.cpp)
  target_include_directories(kpi-render PRIVATE common kpisdk)
  kpidsd_warnings(kpi-render)
  target_compile_definitions(kpi-render PRIVATE _CRT_SECURE_NO_WARNINGS)
  if(NOT MSVC)
    target_link_options(kpi-render PRIVATE -municode -static)
  endif()
  # kpi-render は同じディレクトリの .kpi を使うので、出力先をそろえる
  set_target_properties(kpi-render PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  add_dependencies(kpi-render kpidop kpid2p)

  # クロスビルドでは、CMAKE_CROSSCOMPILING_EMULATOR (wine など) があるときだけテストを登録する
  enable_testing()
  find_package(Python3 COMPONENTS Interpreter)
  if(NOT CMAKE_CROSSCOMPILING OR CMAKE_CROSSCOMPILING_EMULATOR)
    if(Python3_FOUND)
      set(fixtures_dir ${CMAKE_BINARY_DIR}/fixtures)
      set(fixtures_py ${CMAKE_CURRENT_SOURCE_DIR}/tools/fixtures.py)
      set(cli_args --cli $<TARGET_FILE:kpi-render>)
      if(CMAKE_CROSSCOMPILING_EMULATOR)
        list(APPEND cli_args --emulator ${CMAKE_CROSSCOMPILING_EMULATOR})
      endif()
      # 合成した DSF / DSDIFF / WSD を、tools/fixtures.py の参照実装の結果と照合する
      add_test(NAME make-fixtures COMMAND ${Python3_EXECUTABLE} ${fixtures_py} generate ${fixtures_dir})
      set_tests_properties(make-fixtures PROPERTIES FIXTURES_SETUP fixtures)
      # fuzz-smoke: 壊したファイルで落ちたり止まったりしないか
      foreach(check check-dop check-tags seek-check fuzz-smoke)
        add_test(NAME ${check} COMMAND ${Python3_EXECUTABLE} ${fixtures_py} ${check} ${fixtures_dir} ${cli_args})
        set_tests_properties(${check} PROPERTIES FIXTURES_REQUIRED fixtures)
      endforeach()
      # D2P の周波数特性と歪みを測る (numpy が要る。なければ飛ばす)
      add_test(NAME check-d2p COMMAND ${Python3_EXECUTABLE} ${fixtures_py} check-d2p ${fixtures_dir} ${cli_args})
      set_tests_properties(check-d2p PROPERTIES FIXTURES_REQUIRED fixtures SKIP_RETURN_CODE 77)
      if(KPIDSD_TEST_DATA)
        add_test(NAME seek-check-data
          COMMAND ${Python3_EXECUTABLE} ${fixtures_py} seek-check-data ${KPIDSD_TEST_DATA} ${cli_args})
      endif()
    endif()
  endif()
endif()

# Native inference only. KataGo's search and engine code are not linked.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
set(GOGAME_KATAGO_SOURCE_DIR "" CACHE PATH "Extracted KataGo v1.18.1 source (offline override)")
set(GOGAME_KATAGO_BACKEND "CUDA" CACHE STRING "CUDA or EIGEN")
set_property(CACHE GOGAME_KATAGO_BACKEND PROPERTY STRINGS CUDA EIGEN)
if(NOT GOGAME_KATAGO_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(gogame_katago_source
        URL https://codeload.github.com/lightvector/KataGo/tar.gz/refs/tags/v1.18.1
        URL_HASH SHA256=011a205c60c22f83008ba92d31f91196640f620edbfc2f09af79ef3f3311750c)
    FetchContent_MakeAvailable(gogame_katago_source)
    set(GOGAME_KATAGO_SOURCE_DIR "${gogame_katago_source_SOURCE_DIR}")
endif()
set(kg "${GOGAME_KATAGO_SOURCE_DIR}/cpp")
if(NOT EXISTS "${kg}/neuralnet/nneval.cpp")
    message(FATAL_ERROR "GOGAME_KATAGO_SOURCE_DIR must contain cpp/neuralnet/nneval.cpp")
endif()
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
set(kg_files
    core/global.cpp core/base64.cpp core/bsearch.cpp core/config_parser.cpp
    core/datetime.cpp core/fancymath.cpp core/fileutils.cpp core/hash.cpp core/logger.cpp
    core/makedir.cpp core/md5.cpp core/multithread.cpp core/rand.cpp core/rand_helpers.cpp
    core/sha2.cpp core/test.cpp core/threadsafequeue.cpp core/timer.cpp
    game/board.cpp game/rules.cpp game/boardhistory.cpp dataio/homedata.cpp
    neuralnet/nninputs.cpp neuralnet/sgfmetadata.cpp neuralnet/modelversion.cpp
    neuralnet/desc.cpp neuralnet/debugprint.cpp search/mutexpool.cpp)
list(TRANSFORM kg_files PREPEND "${kg}/")
add_library(gogameKataGoNative STATIC ${kg_files} "${PROJECT_SOURCE_DIR}/src/AI/KataGoNative.cpp")
set_target_properties(gogameKataGoNative PROPERTIES AUTOMOC OFF)
target_include_directories(gogameKataGoNative PRIVATE "${kg}" "${PROJECT_SOURCE_DIR}/src/AI")
target_include_directories(gogameKataGoNative SYSTEM PRIVATE "${kg}/external"
    "${kg}/external/filesystem-1.5.8/include" "${kg}/external/half-2.2.0/include")
target_compile_definitions(gogameKataGoNative PRIVATE NO_LIBZIP NO_GIT_REVISION)
target_link_libraries(gogameKataGoNative PRIVATE ZLIB::ZLIB Threads::Threads)
# Pace actual batches centrally, so multiple MCTS workers cannot multiply the duty.
set(kg_paced_dir "${CMAKE_CURRENT_BINARY_DIR}/katago-paced/neuralnet")
file(MAKE_DIRECTORY "${kg_paced_dir}")
file(READ "${kg}/neuralnet/nneval.cpp" kg_eval_source)
string(REPLACE "NeuralNet::getOutput(" "gogamePacedGetOutput(this, " kg_eval_source "${kg_eval_source}")
set(kg_eval_hook "
#include <thread>
#include <utility>
namespace GoKata { double inferenceDuty(const void*); }
template<typename... Args>
static void gogamePacedGetOutput(const void* evaluator, Args&&... args) {
    const auto started = std::chrono::steady_clock::now();
    NeuralNet::getOutput(std::forward<Args>(args)...);
    const auto active = std::chrono::steady_clock::now() - started;
    const double duty = GoKata::inferenceDuty(evaluator);
    if(duty < 1. && duty > 0.) std::this_thread::sleep_for(active * (1. / duty - 1.));
}
")
string(REPLACE "using namespace std;" "${kg_eval_hook}\nusing namespace std;" kg_eval_source "${kg_eval_source}")
set(kg_eval_existing "")
if(EXISTS "${kg_paced_dir}/nneval.cpp")
    file(READ "${kg_paced_dir}/nneval.cpp" kg_eval_existing)
endif()
if(NOT kg_eval_existing STREQUAL kg_eval_source)
    file(WRITE "${kg_paced_dir}/nneval.cpp" "${kg_eval_source}")
endif()
target_include_directories(gogameKataGoNative PRIVATE "${kg}/neuralnet")
target_sources(gogameKataGoNative PRIVATE "${kg_paced_dir}/nneval.cpp")
if(GOGAME_KATAGO_BACKEND STREQUAL "CUDA")
    enable_language(CUDA)
    find_package(CUDAToolkit REQUIRED)
    find_path(GOGAME_CUDNN_INCLUDE cudnn.h HINTS ${CUDAToolkit_INCLUDE_DIRS} ENV CUDNN_ROOT PATH_SUFFIXES include REQUIRED)
    find_library(GOGAME_CUDNN_LIBRARY NAMES cudnn libcudnn.so.9 HINTS ENV CUDNN_ROOT PATH_SUFFIXES lib lib64 REQUIRED)
    # This cuDNN rejects the forced FP16 PRECOMP_GEMM algorithm for the
    # model's input convolution. Ask cuDNN for a supported algorithm instead.
    # Patch a private build copy; the downloaded source stays unchanged.
    set(kg_cuda "${CMAKE_CURRENT_BINARY_DIR}/katago-cuda/neuralnet")
    file(MAKE_DIRECTORY "${kg_cuda}")
    file(COPY "${kg}/neuralnet/" DESTINATION "${kg_cuda}" PATTERN "cudaandrocmbackend.inc" EXCLUDE)
    file(READ "${kg}/neuralnet/cudaandrocmbackend.inc" kg_cuda_original)
    string(REPLACE "if(useFP16 && dilationX <= 1 && dilationY <= 1) {"
        "if(false && useFP16 && dilationX <= 1 && dilationY <= 1) {" kg_cuda_patched "${kg_cuda_original}")
    string(REPLACE "    CUDNN_ERR(name.c_str(),cudnnGetConvolutionForwardWorkspaceSize("
        "    CUDNN_ERR(name.c_str(),cudnnSetConvolutionMathType(convolutionDescriptor, (*convolutionAlgorithms)[batchSize].mathType));\n    CUDNN_ERR(name.c_str(),cudnnGetConvolutionForwardWorkspaceSize("
        kg_cuda_patched "${kg_cuda_patched}")
    string(REPLACE "    CUDNN_ERR(name.c_str(),cudnnConvolutionForward("
        "    CUDNN_ERR(name.c_str(),cudnnSetConvolutionMathType(convolutionDescriptor, (*convolutionAlgorithms)[batchSize].mathType));\n    CUDNN_ERR(name.c_str(),cudnnConvolutionForward("
        kg_cuda_patched "${kg_cuda_patched}")
    if(kg_cuda_patched STREQUAL kg_cuda_original)
        message(FATAL_ERROR "KataGo CUDA compatibility patch no longer matches source")
    endif()
    set(kg_cuda_existing "")
    if(EXISTS "${kg_cuda}/cudaandrocmbackend.inc")
        file(READ "${kg_cuda}/cudaandrocmbackend.inc" kg_cuda_existing)
    endif()
    if(NOT kg_cuda_existing STREQUAL kg_cuda_patched)
        file(WRITE "${kg_cuda}/cudaandrocmbackend.inc" "${kg_cuda_patched}")
    endif()
    target_include_directories(gogameKataGoNative PRIVATE "${kg}/neuralnet")
    target_sources(gogameKataGoNative PRIVATE "${kg_cuda}/cudabackend.cpp" "${kg_cuda}/cudautils.cpp" "${kg_cuda}/cudahelpers.cu")
    target_compile_definitions(gogameKataGoNative PRIVATE USE_CUDA_BACKEND CUDA_TARGET_VERSION=${CMAKE_CUDA_COMPILER_VERSION})
    target_include_directories(gogameKataGoNative SYSTEM PRIVATE "${GOGAME_CUDNN_INCLUDE}" "${kg}/external/cudnn-frontend/include")
    target_link_libraries(gogameKataGoNative PRIVATE CUDA::cudart CUDA::cublas CUDA::nvrtc "${GOGAME_CUDNN_LIBRARY}")
    set_target_properties(gogameKataGoNative PROPERTIES CUDA_STANDARD 17 CUDA_STANDARD_REQUIRED ON)
elseif(GOGAME_KATAGO_BACKEND STREQUAL "EIGEN")
    set(GOGAME_EIGEN_INCLUDE_DIR "" CACHE PATH "Optional extracted Eigen headers (contains Eigen/)")
    if(GOGAME_EIGEN_INCLUDE_DIR)
        if(NOT EXISTS "${GOGAME_EIGEN_INCLUDE_DIR}/Eigen/Dense"
            OR NOT EXISTS "${GOGAME_EIGEN_INCLUDE_DIR}/unsupported/Eigen/CXX11/Tensor")
            message(FATAL_ERROR "GOGAME_EIGEN_INCLUDE_DIR must contain Eigen/Dense and unsupported/Eigen/CXX11/Tensor: ${GOGAME_EIGEN_INCLUDE_DIR}. Clear this override with -UGOGAME_EIGEN_INCLUDE_DIR to use automatic Eigen discovery/download.")
        endif()
        target_include_directories(gogameKataGoNative SYSTEM PRIVATE "${GOGAME_EIGEN_INCLUDE_DIR}")
    else()
        find_package(Eigen3 3.4 QUIET NO_MODULE)
        if(Eigen3_FOUND)
            target_link_libraries(gogameKataGoNative PRIVATE Eigen3::Eigen)
        else()
            include(FetchContent)
            FetchContent_Declare(gogame_eigen
                URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
                URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
                SOURCE_SUBDIR headers-only)
            FetchContent_MakeAvailable(gogame_eigen)
            target_include_directories(gogameKataGoNative SYSTEM PRIVATE "${gogame_eigen_SOURCE_DIR}")
        endif()
    endif()
    target_sources(gogameKataGoNative PRIVATE "${kg}/neuralnet/eigenbackend.cpp")
    target_compile_definitions(gogameKataGoNative PRIVATE USE_EIGEN_BACKEND)
else()
    message(FATAL_ERROR "GOGAME_KATAGO_BACKEND must be CUDA or EIGEN")
endif()

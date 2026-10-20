#pragma once
// No Qt types cross this boundary into KataGo's native inference code.
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace GoKata {
struct Move { int player, point; };
struct Request {
    std::array<int, 361> initial{}, current{};
    int initialPlayer = 0, initialKo = -1, initialPasses = 0, initialTurn = 0;
    std::array<int, 2> initialCaptured{};
    int player = 0, ko = -1;
    double komi = 6.5;
    std::vector<Move> moves;
};
struct Result { std::array<float, 362> policy{}; float value = 0; };
struct Statistics { std::uint64_t rows = 0, batches = 0, hits = 0; };
class Native {
public:
    Native(const std::string& model, int batchSize, int cachePower, int gpu, int cpuThreads, bool fp16, double duty);
    ~Native();
    Result evaluate(const Request& request);
    Statistics statistics() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}

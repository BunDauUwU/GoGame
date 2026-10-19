#pragma once
#include "../Rules/GoMatch.h"
#include <atomic>
#include <memory>

#include "SearchTypes.h"

class EasyAI
{
public:
    explicit EasyAI(SearchOptions options = {});
    const SearchOptions &options() const { return m_options; }
    SearchResult chooseMove(const GoMatch &cur,
                            const std::shared_ptr<std::atomic_bool> &cancel = {}) const;

private:
    class Search;
    SearchOptions m_options;
    double evaluate(const GoMatch &game) const;
};

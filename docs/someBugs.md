# Development Issues and Fixes

**Project:** GoGame — C++17 / Qt 6  
**Updated:** 21 October 2026

These notes rewrite two development experiences reported by Hoang Minh Vu. They distinguish the reported symptoms from the implementation visible in the current source.

## 1. High Memory Usage During DSU Board Development

### Reported Symptom

While implementing the board using a Disjoint Set Union (DSU), I noticed high memory usage. It appeared acceptable on my computer, but I was concerned about weaker machines and wanted to reduce unnecessary allocations and copies.

This was a memory-usage concern, not a confirmed memory-limit-exceeded (MLE) failure; no memory limit or measured peak was recorded.

### Approach Used

GoMatch stores group data through a shared cache:

```cpp
mutable std::shared_ptr<const Groups> m_groups;
```

`buildGroups()` creates a writable group object with `std::make_shared<Groups>()`, fills its parent, size, and liberty data, and returns it as `std::shared_ptr<const Groups>`.

The useful effects are:

- `std::make_shared<T>()` typically allocates the object and its shared-ownership control block together, reducing allocation overhead compared with separately allocating the object and constructing a shared pointer.
- Copies of an unchanged GoMatch can share the same group cache instead of duplicating its DSU arrays and liberty bitsets.
- The shared group data is read-only after construction. A changed position builds its own cache rather than modifying a cache still used by another position.

Combining the object and control block does not put all DSU data into one small memory slot or eliminate its storage cost. Distinct cached positions, retained game history, and search structures still consume memory. Sharing unchanged caches avoids redundant copies; it does not impose a total-memory cap.

### Current Status

The shared-cache implementation is present in [GoMatch.h](../src/Rules/GoMatch.h) and [GoMatch.cpp](../src/Rules/GoMatch.cpp). Its structure was checked for this documentation update. The original memory reduction has not been quantified; a before/after benchmark would be needed to measure it on this machine or a weaker machine.

## 2. Const-Correctness Errors While Refactoring the Group Cache

### Reported Symptom

During the shared-pointer refactor, several GoMatch functions stopped compiling. I spent approximately three hours working through the errors. Adding const qualification reduced some errors, but the cache still needed initialization when accessed through a const function. Attempts to make it writable again either reintroduced the earlier problems or prevented the expected update.

### Cause

`groups()` is a const accessor:

```cpp
const GoMatch::Groups &GoMatch::groups() const
```

It may need to initialize the group cache on the first call. A const member function cannot normally assign to an ordinary data member, even when that assignment only caches a result derived from an unchanged board.

There are two separate const-related concerns:

| Declaration element             | Meaning                                                                                                       |
| ------------------------------- | ------------------------------------------------------------------------------------------------------------- |
| `std::shared_ptr<const Groups>` | Group data cannot be modified through this pointer. The pointer itself can normally be assigned a new target. |
| `mutable` on `m_groups`         | This pointer member may be assigned even inside a const GoMatch member function.                              |

### Resolution in the Current Code

The cache pointer is declared mutable, while its pointed-to data remains const:

```cpp
mutable std::shared_ptr<const Groups> m_groups;
```

The accessor initializes it lazily:

```cpp
const GoMatch::Groups &GoMatch::groups() const
{
    if (!m_groups)
        m_groups = buildGroups(board);
    return *m_groups;
}
```

`buildGroups()` fills a mutable `Groups` object before exposing it through the read-only shared pointer. `groups()` then updates only the cached pointer; it does not change the board or mutate an existing shared Groups object.

### Current Status

The original notes report that this resolved the compilation problem, and the current declaration/accessor match that explanation. The historical compiler errors are unavailable; no new build or reproduction was performed for this documentation-only update.

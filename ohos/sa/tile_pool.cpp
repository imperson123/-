#include "tile_pool.h"

#include <atomic>
#include <thread>
#include <vector>

namespace tmac_sa {

std::vector<std::pair<int, int>> SplitMTiles(int m_logical_total, int n_threads) {
  std::vector<std::pair<int, int>> out;
  if (m_logical_total <= 0) {
    return out;
  }
  const int nt = n_threads > 0 ? n_threads : 1;
  const int base = m_logical_total / nt;
  int rem = m_logical_total % nt;
  int begin = 0;
  for (int t = 0; t < nt; ++t) {
    int rows = base + (rem > 0 ? 1 : 0);
    if (rem > 0) {
      --rem;
    }
    if (rows > 0) {
      out.emplace_back(begin, rows);
      begin += rows;
    }
  }
  return out;
}

bool RunParallelTiles(int n_threads, const std::function<bool(int, int, int)>& fn) {
  if (!fn) {
    return false;
  }
  const int nt = n_threads > 0 ? n_threads : 1;
  // Caller supplies begin/rows via SplitMTiles externally; here we only fan out by tid.
  // For generic parallel-over-jobs use RunTiledQGeMMParallel.
  std::atomic<bool> ok{true};
  std::vector<std::thread> threads;
  threads.reserve(static_cast<size_t>(nt));
  for (int t = 0; t < nt; ++t) {
    threads.emplace_back([&, t]() {
      if (!fn(t, 0, 0)) {
        ok.store(false);
      }
    });
  }
  for (auto& th : threads) {
    th.join();
  }
  return ok.load();
}

int RunTiledQGeMMParallel(const TileJob* jobs, size_t n_jobs, int n_threads) {
  if (!jobs && n_jobs > 0) {
    return -1;
  }
  if (n_jobs == 0) {
    return 0;
  }
  const int nt = n_threads > 1 ? n_threads : 1;
  if (nt == 1 || n_jobs == 1) {
    return RunTiledQGeMM(jobs, n_jobs);
  }

  std::atomic<int> first_err{0};
  std::vector<std::thread> threads;
  threads.reserve(static_cast<size_t>(nt));

  auto worker = [&](size_t begin, size_t end) {
    for (size_t i = begin; i < end; ++i) {
      int r = RunTiledQGeMM(jobs + i, 1);
      if (r != 0) {
        int expected = 0;
        first_err.compare_exchange_strong(expected, r);
        return;
      }
    }
  };

  const size_t chunk = (n_jobs + static_cast<size_t>(nt) - 1) / static_cast<size_t>(nt);
  for (int t = 0; t < nt; ++t) {
    size_t b = static_cast<size_t>(t) * chunk;
    if (b >= n_jobs) {
      break;
    }
    size_t e = b + chunk;
    if (e > n_jobs) {
      e = n_jobs;
    }
    threads.emplace_back(worker, b, e);
  }
  for (auto& th : threads) {
    th.join();
  }
  return first_err.load();
}

}  // namespace tmac_sa

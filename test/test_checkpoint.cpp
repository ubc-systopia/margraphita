#include <sys/stat.h>
#include <unistd.h>
#include <wiredtiger.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Simple atomic counters to simulate local_nnodes and local_nedges
static std::atomic<uint64_t> record_count(0);
static std::atomic<uint64_t> total_inserts(0);

// Checkpoint metadata storage
struct CheckpointMetadata
{
  uint64_t record_count_snapshot;
  uint64_t total_inserts_snapshot;
  std::string checkpoint_name;
};

std::vector<CheckpointMetadata> checkpoint_history;
std::mutex checkpoint_mutex;

class CheckpointTester
{
 private:
  WT_CONNECTION *conn = nullptr;
  std::string db_dir;

 public:
  CheckpointTester(const std::string &dir) : db_dir(dir) {}

  ~CheckpointTester()
  {
    if (conn)
    {
      conn->close(conn, nullptr);
    }
  }

  int init()
  {
    // Create directory
    mkdir(db_dir.c_str(), 0755);

    // Open connection
    const char *config = "create,cache_size=100MB,log=(enabled=true)";
    int ret = wiredtiger_open(db_dir.c_str(), nullptr, config, &conn);
    if (ret != 0)
    {
      std::cerr << "Failed to open WiredTiger connection: "
                << wiredtiger_strerror(ret) << std::endl;
      return ret;
    }

    // Create table and metadata table
    WT_SESSION *session;
    ret = conn->open_session(conn, nullptr, nullptr, &session);
    if (ret != 0) return ret;

    // Create data table
    ret = session->create(
        session, "table:test_data", "key_format=Q,value_format=QS");
    if (ret != 0)
    {
      std::cerr << "Failed to create data table: " << wiredtiger_strerror(ret)
                << std::endl;
      return ret;
    }

    // Create metadata table for checkpoint info
    ret = session->create(
        session, "table:checkpoint_metadata", "key_format=S,value_format=QQS");
    if (ret != 0)
    {
      std::cerr << "Failed to create metadata table: "
                << wiredtiger_strerror(ret) << std::endl;
      return ret;
    }

    session->close(session, nullptr);
    std::cout << "Initialized WiredTiger database at: " << db_dir << std::endl;
    return 0;
  }

  void insert_metadata(WT_CURSOR *cursor,
                       const std::string &key,
                       uint64_t record_count,
                       uint64_t total_inserts,
                       const std::string &checkpoint_name)
  {
    cursor->set_key(cursor, key.c_str());
    cursor->set_value(
        cursor, record_count, total_inserts, checkpoint_name.c_str());
    int ret = cursor->insert(cursor);
    if (ret != 0)
    {
      std::cerr << "Failed to insert metadata: " << wiredtiger_strerror(ret)
                << std::endl;
    }
  }

  std::string make_checkpoint_atomic()
  {
    WT_SESSION *session;
    int ret = conn->open_session(conn, nullptr, nullptr, &session);
    if (ret != 0)
    {
      std::cerr << "Failed to open session for checkpoint" << std::endl;
      return "";
    }

    try
    {
      // Start transaction for atomic capture
      ret = session->begin_transaction(session, "isolation=snapshot");
      if (ret != 0)
      {
        std::cerr << "Failed to begin transaction: " << wiredtiger_strerror(ret)
                  << std::endl;
        session->close(session, nullptr);
        return "";
      }

      // *** CRITICAL SECTION: Capture counts atomically within transaction ***
      uint64_t snapshot_records = record_count.load(std::memory_order_acquire);
      uint64_t snapshot_inserts = total_inserts.load(std::memory_order_acquire);

      // Generate checkpoint name
      auto now = std::chrono::system_clock::to_time_t(
          std::chrono::system_clock::now());
      std::tm localTime = *std::localtime(&now);
      char cpt_name[32];
      std::strftime(
          cpt_name, sizeof(cpt_name), "cpt_%Y%m%d_%H%M%S", &localTime);

      // Add microseconds to make it unique
      auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count() %
                1000000;
      std::snprintf(cpt_name + strlen(cpt_name),
                    sizeof(cpt_name) - strlen(cpt_name),
                    "_%06ld",
                    us);

      std::cout << "[CHECKPOINT] Captured state - Records: " << snapshot_records
                << ", Inserts: " << snapshot_inserts
                << " for checkpoint: " << cpt_name << std::endl;

      // Store metadata within the same transaction
      WT_CURSOR *meta_cursor;
      ret = session->open_cursor(session,
                                 "table:checkpoint_metadata",
                                 nullptr,
                                 "overwrite=true",
                                 &meta_cursor);
      if (ret != 0)
      {
        session->rollback_transaction(session, nullptr);
        session->close(session, nullptr);
        return "";
      }

      insert_metadata(meta_cursor,
                      std::string(cpt_name),
                      snapshot_records,
                      snapshot_inserts,
                      std::string(cpt_name));
      meta_cursor->close(meta_cursor);

      // Commit the metadata transaction
      ret = session->commit_transaction(session, nullptr);
      if (ret != 0)
      {
        std::cerr << "Failed to commit metadata transaction: "
                  << wiredtiger_strerror(ret) << std::endl;
        session->close(session, nullptr);
        return "";
      }

      // Now create the actual WiredTiger checkpoint
      char checkpoint_config[128];
      std::snprintf(
          checkpoint_config, sizeof(checkpoint_config), "name=%s", cpt_name);

      ret = session->checkpoint(session, checkpoint_config);
      if (ret != 0)
      {
        std::cerr << "Failed to create checkpoint: " << wiredtiger_strerror(ret)
                  << std::endl;
        session->close(session, nullptr);
        return "";
      }

      // Store in our history for verification
      {
        std::lock_guard<std::mutex> lock(checkpoint_mutex);
        checkpoint_history.push_back(
            {snapshot_records, snapshot_inserts, std::string(cpt_name)});
      }

      session->close(session, nullptr);
      std::cout << "[CHECKPOINT] Successfully created checkpoint: " << cpt_name
                << std::endl;
      return std::string(cpt_name);
    }
    catch (...)
    {
      session->rollback_transaction(session, nullptr);
      session->close(session, nullptr);
      throw;
    }
  }

  void worker_thread(int thread_id, int num_operations)
  {
    WT_SESSION *session;
    int ret = conn->open_session(conn, nullptr, nullptr, &session);
    if (ret != 0)
    {
      std::cerr << "Worker " << thread_id << ": Failed to open session"
                << std::endl;
      return;
    }

    WT_CURSOR *cursor;
    ret = session->open_cursor(
        session, "table:test_data", nullptr, nullptr, &cursor);
    if (ret != 0)
    {
      std::cerr << "Worker " << thread_id << ": Failed to open cursor"
                << std::endl;
      session->close(session, nullptr);
      return;
    }

    for (int i = 0; i < num_operations; i++)
    {
      uint64_t key = thread_id * 100000 + i;
      std::string value = "thread_" + std::to_string(thread_id) + "_record_" +
                          std::to_string(i);

      cursor->set_key(cursor, key);
      cursor->set_value(cursor, key, value.c_str());

      ret = cursor->insert(cursor);
      if (ret != 0 && ret != WT_DUPLICATE_KEY)
      {
        std::cerr << "Worker " << thread_id
                  << ": Insert failed: " << wiredtiger_strerror(ret)
                  << std::endl;
      }
      else if (ret != WT_DUPLICATE_KEY)
      {
        // Only increment if it was a successful new insert
        record_count.fetch_add(1);
      }
      total_inserts.fetch_add(1);

      // Small delay to simulate work (reduced for more concurrency)
      std::this_thread::sleep_for(std::chrono::microseconds(5));
    }

    cursor->close(cursor);
    session->close(session, nullptr);

    std::cout << "Worker " << thread_id << " completed " << num_operations
              << " operations" << std::endl;
  }

  void checkpoint_thread(int num_checkpoints, int interval_ms)
  {
    for (int i = 0; i < num_checkpoints; i++)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

      std::string checkpoint_name = make_checkpoint_atomic();
      if (checkpoint_name.empty())
      {
        std::cerr << "Failed to create checkpoint " << i << std::endl;
      }
    }
    std::cout << "Checkpoint thread completed " << num_checkpoints
              << " checkpoints" << std::endl;
  }

  bool verify_checkpoint_consistency()
  {
    std::cout << "\n=== Verifying Checkpoint Consistency ===" << std::endl;

    bool all_good = true;

    for (const auto &checkpoint : checkpoint_history)
    {
      // Open a session at this checkpoint
      WT_SESSION *session;
      int ret = conn->open_session(conn, nullptr, nullptr, &session);
      if (ret != 0) continue;

      std::string checkpoint_config =
          "checkpoint=" + checkpoint.checkpoint_name;

      // First, read the metadata stored in the checkpoint
      uint64_t stored_records = 0, stored_inserts = 0;
      bool metadata_found = false;

      WT_CURSOR *meta_cursor;
      ret = session->open_cursor(session,
                                 "table:checkpoint_metadata",
                                 nullptr,
                                 checkpoint_config.c_str(),
                                 &meta_cursor);

      if (ret == 0)
      {
        meta_cursor->set_key(meta_cursor, checkpoint.checkpoint_name.c_str());
        ret = meta_cursor->search(meta_cursor);

        if (ret == 0)
        {
          const char *stored_name;
          meta_cursor->get_value(
              meta_cursor, &stored_records, &stored_inserts, &stored_name);
          metadata_found = true;
        }
        meta_cursor->close(meta_cursor);
      }

      if (!metadata_found)
      {
        std::cout << "Checkpoint " << checkpoint.checkpoint_name
                  << ": ✗ METADATA NOT FOUND" << std::endl;
        all_good = false;
        session->close(session, nullptr);
        continue;
      }

      // Now, open the data table at the checkpoint and count actual records
      WT_CURSOR *data_cursor;
      ret = session->open_cursor(session,
                                 "table:test_data",
                                 nullptr,
                                 checkpoint_config.c_str(),
                                 &data_cursor);

      if (ret != 0)
      {
        std::cout << "Checkpoint " << checkpoint.checkpoint_name
                  << ": ✗ FAILED TO OPEN DATA TABLE" << std::endl;
        all_good = false;
        session->close(session, nullptr);
        continue;
      }

      // Tally the actual number of records in the checkpoint
      uint64_t actual_record_count = 0;
      ret = data_cursor->reset(data_cursor);
      while ((ret = data_cursor->next(data_cursor)) == 0)
      {
        actual_record_count++;
      }
      data_cursor->close(data_cursor);

      // Compare actual count against metadata
      bool records_match = (actual_record_count == stored_records);

      std::cout << "Checkpoint " << checkpoint.checkpoint_name << ": ";
      if (records_match)
      {
        std::cout << "✓ CONSISTENT (actual: " << actual_record_count
                  << ", metadata: " << stored_records << ")" << std::endl;
      }
      else
      {
        std::cout << "✗ INCONSISTENT" << std::endl;
        std::cout << "  Actual records in checkpoint: " << actual_record_count
                  << std::endl;
        std::cout << "  Metadata stored records: " << stored_records
                  << std::endl;
        std::cout << "  Metadata stored inserts: " << stored_inserts
                  << std::endl;
        all_good = false;
      }

      session->close(session, nullptr);
    }

    return all_good;
  }

  void print_final_stats()
  {
    std::cout << "\n=== Final Statistics ===" << std::endl;
    std::cout << "Total records in DB: " << record_count.load() << std::endl;
    std::cout << "Total insert operations: " << total_inserts.load()
              << std::endl;
    std::cout << "Number of checkpoints created: " << checkpoint_history.size()
              << std::endl;
  }
};

int main()
{
  std::cout << "=== Testing Atomic Checkpoint Strategy ===" << std::endl;

  // Clean up any existing test database
  system("rm -rf /tmp/test_checkpoint_db");

  CheckpointTester tester("/tmp/test_checkpoint_db");

  if (tester.init() != 0)
  {
    std::cerr << "Failed to initialize tester" << std::endl;
    return 1;
  }

  // Test parameters
  const int num_workers = 8;
  const int operations_per_worker = 100000;
  const int num_checkpoints = 5;
  const int checkpoint_interval_ms = 10;

  std::cout << "Starting test with " << num_workers << " worker threads, "
            << operations_per_worker << " operations each" << std::endl;
  std::cout << "Creating " << num_checkpoints << " checkpoints every "
            << checkpoint_interval_ms << "ms" << std::endl;

  // Launch worker threads
  std::vector<std::thread> workers;
  for (int i = 0; i < num_workers; i++)
  {
    workers.emplace_back(
        &CheckpointTester::worker_thread, &tester, i, operations_per_worker);
  }

  // Launch checkpoint thread
  std::thread checkpoint_worker(&CheckpointTester::checkpoint_thread,
                                &tester,
                                num_checkpoints,
                                checkpoint_interval_ms);

  // Wait for all threads to complete
  for (auto &worker : workers)
  {
    worker.join();
  }
  checkpoint_worker.join();

  // Small delay to ensure all operations are flushed
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Print statistics and verify consistency
  tester.print_final_stats();
  bool consistent = tester.verify_checkpoint_consistency();

  std::cout << "\n=== Test Result ===" << std::endl;
  if (consistent)
  {
    std::cout << "✓ SUCCESS: All checkpoints are consistent!" << std::endl;
    return 0;
  }
  else
  {
    std::cout << "✗ FAILURE: Some checkpoints are inconsistent!" << std::endl;
    return 1;
  }
}
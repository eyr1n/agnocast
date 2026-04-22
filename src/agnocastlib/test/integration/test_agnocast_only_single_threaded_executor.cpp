#include <agnocast/agnocast.hpp>
#include <agnocast/node/agnocast_only_executor.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

class AgnocastOnlyDummyNode : public agnocast::Node
{
public:
  AgnocastOnlyDummyNode() : agnocast::Node("agnocast_only_dummy_node")
  {
    timer_ = this->create_timer(100ms, [this]() { count++; });
  }

  int count = 0;

private:
  agnocast::TimerBase::SharedPtr timer_;
};

class AgnocastOnlySingleThreadedExecutorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    agnocast::init(0, nullptr);
    executor_ = std::make_shared<agnocast::AgnocastOnlySingleThreadedExecutor>();
    node_ = std::make_shared<AgnocastOnlyDummyNode>();
    executor_->add_node(node_);
  }

  void TearDown() override { agnocast::shutdown(); }

  std::shared_ptr<agnocast::AgnocastOnlySingleThreadedExecutor> executor_;
  std::shared_ptr<AgnocastOnlyDummyNode> node_;
};

TEST_F(AgnocastOnlySingleThreadedExecutorTest, test_is_spinning)
{
  EXPECT_FALSE(executor_->is_spinning());

  std::thread spin_thread([this]() { this->executor_->spin(); });

  auto deadline = std::chrono::steady_clock::now() + 1s;
  while (std::chrono::steady_clock::now() < deadline) {
    if (executor_->is_spinning()) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  EXPECT_TRUE(executor_->is_spinning());

  executor_->cancel();
  spin_thread.join();

  EXPECT_FALSE(executor_->is_spinning());
}

TEST_F(AgnocastOnlySingleThreadedExecutorTest, test_spin_once)
{
  EXPECT_EQ(node_->count, 0);

  executor_->spin_once();

  EXPECT_EQ(node_->count, 1);

  executor_->spin_once();

  EXPECT_EQ(node_->count, 2);
}

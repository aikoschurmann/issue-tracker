#include <gtest/gtest.h>
#include "tracker/parser.hpp"
#include <optional>

TEST(ParserTest, ParsesValidTask) {
    std::string_view content = 
R"(---
status: CLOSED
priority: 42
tags: [bug, ui]
---
# Fix the rendering bug
This is the body of the bug.)";

    std::optional<tracker::Task> task_opt = tracker::Parser::parse_task(content);
    ASSERT_TRUE(task_opt.has_value());
    
    tracker::Task task = task_opt.value();
    EXPECT_EQ(task.status, tracker::Status::Closed);
    EXPECT_EQ(task.priority, 42);
    ASSERT_EQ(task.tags.size(), 2);
    EXPECT_EQ(task.tags[0], "bug");
    EXPECT_EQ(task.tags[1], "ui");
    EXPECT_EQ(task.title, "Fix the rendering bug");
    EXPECT_EQ(task.body, "This is the body of the bug.");
}

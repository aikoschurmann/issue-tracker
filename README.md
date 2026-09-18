# IssueTracker (C++ CLI)

A blisteringly fast, git-native, terminal-based task and issue tracker. Built purely in C++20, it parses Markdown files into a Directed Acyclic Graph (DAG) to help you manage complex task dependencies without leaving your terminal.

## Philosophy

The tracker operates as a **thin, transparent Markdown layer** directly over your actual Git project repository. It does not use shadow repositories, background tracking databases, or proprietary APIs. Your tasks live natively inside your project in a `tasks/` directory as standard Markdown files. "Closing" a task is simply an edit to a file that gets organically committed alongside your code.

## Features

- **Git-Native Workflow:** Use `issue-tracker start <id>` to instantly checkout a `task/<id>` branch. Use `issue-tracker finish` to commit your work and close the issue automatically.
- **Implicit Context Awareness:** Once you are branched into a task, commands like `edit`, `close`, `ls`, and `status` automatically understand your context. The `ls` command highlights your active task in green.
- **DAG Engine:** Tasks are nodes. Dependencies are edges. Kahn's Algorithm ensures you never create a cycle and automatically determines the topologically sorted optimal order of execution.
- **Markdown Native:** Tasks are just `.md` files with YAML frontmatter. 
- **Prefix Matching:** Git-style prefix matching. You don't need to type `issue-tracker edit implement-directory-discovery-023a` — just `issue-tracker edit impl` and it instantly resolves.

## Installation

We provide a fully automated script that compiles the binary via CMake, installs it to `/usr/local/bin`, and natively configures ZSH autocompletion.

```bash
# Clone the repository
git clone https://github.com/aikoschurmann/issue-tracker.git
cd issue-tracker

# Run the automated installer (requires sudo for /usr/local/bin)
./install.sh

# Reload your shell for autocompletion
source ~/.zshrc
```

## Quick Start & Usage

Initialize the tracker inside any existing Git repository:

```bash
cd ~/my-project
issue-tracker init
```
*This creates a `tasks/` directory and `.trackerconfig`, and adds them to your `.gitignore`.*

### Creating and Linking Tasks

```bash
# Create a new epic (Priority 500)
issue-tracker new "Build Authentication System" -p 500

# Create a dependency task
issue-tracker new "Setup OAuth API" -p 200

# Link them (Authentication depends on OAuth)
issue-tracker link build-auth setup-oauth
```

### Git-Integrated Workflow

When you are ready to start coding, the tracker automatically orchestrates your git branches:

```bash
# Checkout a new branch: task/setup-oauth-api-1234
issue-tracker start setup-oauth

# Look at your tasks. The active task is highlighted!
issue-tracker ls

# Edit the current task's markdown file in your editor (Implicit context)
issue-tracker edit

# Code... code... code...

# Close the task, stage the code, and launch $EDITOR with a pre-filled git commit message!
issue-tracker finish
```

### Dependency Management & Visualizations

```bash
# View your dependency tree (hoists shared dependencies cleanly)
issue-tracker tree

# View exactly what you should work on right now (topological sort)
issue-tracker plan

# Quick-edit metadata without opening an editor
issue-tracker set setup-oauth -t "backend,api" -p 250

# View project health dashboard
issue-tracker status
```

## Configuration

IssueTracker can be configured globally (`~/.trackerconfig`) or locally (`.trackerconfig` in your project root).

Use the CLI to configure settings:
```bash
# Set your author name for Markdown stamping
issue-tracker config user.name "Aiko Schurmann"

# Set your preferred CLI text editor (defaults to $EDITOR then code)
issue-tracker config core.editor "vim"
```

## Documentation

Run `issue-tracker help` to see all available commands and detailed usage arguments.

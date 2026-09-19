# IssueTracker

A lightweight, terminal-native project management tool embedded directly within your Git repository.

IssueTracker maintains project state using standard Markdown files and a Directed Acyclic Graph (DAG) for dependency resolution. It provides deep, opt-in Git integration without enforcing rigid workflows.

## Core Philosophy

1. **Data is Local**: Tasks are stored in a `tasks/` directory as Markdown files. Your project state travels with your repository.
2. **Strict Dependencies**: Tasks are organized as a DAG. You define dependencies, and the engine calculates the exact topological order of execution and critical paths.
3. **Decoupled Workspaces**: Git branching is strictly opt-in. You can set active task contexts while remaining on your primary branch, or explicitly request isolated feature branches.
4. **Explicit Staging**: The tool never touches your Git staging area without explicit flags or configuration.

## Quick Start

### 1. Initialization
Initialize the tracker at the root of your repository:
```bash
issue-tracker init
```
*This creates the `tasks/` directory and updates your `.gitignore`.*

### 2. Task Creation & Dependency Graph
Create tasks and establish blocking dependencies:
```bash
issue-tracker new "Implement authentication API" -p 200
issue-tracker new "Design login interface" -p 100

# Block the interface task until the API is completed
issue-tracker link design-login-interface implement-authentication-api
```

### 3. Execution Planning
Evaluate project state and determine the next actionable task:
```bash
# View the dependency tree
issue-tracker tree

# Output a strictly ordered topological execution plan
issue-tracker plan
```

### 4. Development Context
Set your development context to an unblocked task.
```bash
# Set context (remains on current branch)
issue-tracker start implement-authentication-api

# Alternative: Create and checkout an isolated feature branch
issue-tracker start implement-authentication-api -b
```

### 5. Atomic Closure
Once development is complete, close the task. 
```bash
# Close task, stage all changes, and commit
issue-tracker finish -a -m "Implemented JWT authentication"

# Close task, stage changes, commit, and push as a Pull Request
issue-tracker finish -a --pr
```
*Note: If `--pr` is invoked on the main branch, the tool will dynamically isolate your staged changes to a new feature branch, push to origin, and restore your previous working tree.*

---

## Command Reference

### Workflow & Git
| Command | Description |
|---|---|
| `start <id> [-b]` | Set active task context. Use `-b` to checkout a new branch (`task/<id>`). |
| `finish [id] [flags]` | Mark task as CLOSED and create a Git commit. |
| `submit [id]` | Alias for `finish --pr`. |
| `close [id]` | Mark a task CLOSED without generating a Git commit. |
| `open [id]` | Mark a task OPEN. |
| `link <t> <d>` | Add dependency `<d>` to target task `<t>`. |
| `unlink <t> <d>` | Remove dependency `<d>` from target `<t>`. |

**Finish Flags:**
- `-a, --stage-all` : Stage all repository changes before committing.
- `--stage-tasks` : Stage only the `tasks/` directory metadata.
- `-m <msg>` : Provide an inline commit message.
- `-e, --edit` : Open `$EDITOR` to write the commit message.
- `--pr` : Push the resulting commit to origin.

### Task Management
| Command | Description |
|---|---|
| `new <title> [flags]`| Create a new task. |
| `edit [id]` | Open the Markdown file in `$EDITOR`. |
| `set <id> [flags]` | Modify task metadata without editing the file. |
| `rm <id> [-r]` | Permanently delete a task. Use `-r` to recursively delete subtasks. |

**Creation/Modification Flags:**
- `-d <desc>` : Description.
- `-p <pri>` : Priority integer (0-1000).
- `-t <tags>` : Comma-separated list of tags.
- `--deps <ids>` : Comma-separated list of blocking dependencies.

### Views & Analysis
| Command | Description |
|---|---|
| `ls [-a] [-c]` | List tasks. Differentiates between `[ACTIVE CONTEXT]` and `[ACTIVE BRANCH]`. |
| `tree [-a] [-d <n>]` | Visualize the dependency DAG. |
| `plan` | Print a topological sort of immediately actionable tasks. |
| `bottleneck` | Critical Path Analysis identifying the most blocking tasks. |
| `requires <id>` | Visualize the specific dependency sub-tree for a task. |
| `burndown` | ASCII velocity chart plotting closures over the last 14 days. |
| `status` | High-level project completion dashboard. |

### Configuration & Setup
| Command | Description |
|---|---|
| `init` | Initialize tracker infrastructure. |
| `config [<k> <v>]` | View or modify tracker settings. |
| `log [-n <num>]` | Output Git log filtered to the `tasks/` directory. |

## Configuration
Executing `issue-tracker config` without arguments outputs a table of all available variables and their current values.

Notable configuration targets:
- `git.autostage_tasks` (Default: `false`) : Automatically stages metadata when running `finish`.
- `git.autostage_code` (Default: `false`) : Automatically stages all code (`git add -A`) when running `finish`.
- `core.editor` (Default: `$EDITOR`) : The executable invoked by `edit`.

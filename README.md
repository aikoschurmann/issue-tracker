# IssueTracker

A simple, fast, and terminal-native issue tracker that lives right inside your Git repository. 

Instead of switching to a browser to manage your tasks, IssueTracker lets you create, link, and close tasks directly from your terminal. Tasks are saved as standard Markdown files in your project, meaning your code and your issues always stay perfectly in sync.

## How It Works (Step-by-Step)

Imagine you are starting a new feature. Here is what your workflow looks like with IssueTracker:

**1. Initialize the tracker in your project**
```bash
cd my-project
issue-tracker init
```
*This creates a `tasks/` folder where your issues will live.*

**2. Create a new task**
```bash
issue-tracker new "Build login page" -p 100 -t frontend
```
*This creates a markdown file. It gives it an ID, like `build-login-page-abcd`.*

**3. Break it down into subtasks**
```bash
issue-tracker new "Design login UI" -p 200
issue-tracker new "Write authentication API" -p 200

# Tell the tracker that the login page depends on the UI and API tasks
issue-tracker link build-login-page design-login
issue-tracker link build-login-page write-auth
```

**4. See what you need to do**
```bash
# View your tasks in a clear, tree-like structure
issue-tracker tree

# Or ask the tracker for a step-by-step Execution Plan!
# It will tell you exactly which tasks are ready to be worked on right now.
issue-tracker plan
```

**5. Start working!**
```bash
# This automatically creates and checks out a git branch for your task!
issue-tracker start design-login
```

**6. Finish the task**
Once you're done coding, just type:
```bash
issue-tracker finish
```
*This will automatically mark the task as CLOSED, stage your code and task files, and open your editor so you can save the git commit!*

---

## Command Reference

Here is a complete list of all available commands in IssueTracker:

### Task Creation & Editing
- `new <title> [-p priority] [-t tags] [-e]` : Create a new task. Use `-e` to immediately open it in your editor.
- `edit [<id>]` : Open the task's Markdown file in your text editor. If you are already working on a task branch, you can just type `edit` without an ID.
- `set <id> [-p priority] [-t tags]` : Quickly update a task's priority or tags without opening an editor.
- `rm <id>` : Permanently delete a task and its Markdown file.

### Linking Dependencies
- `link <target_id> <dependency_id>` : Make the target task depend on the dependency task. The target task will be BLOCKED until the dependency is CLOSED.
- `unlink <target_id> <dependency_id>` : Remove a dependency link.

### Workflow & Git
- `start <id>` : Start working on a task. This creates and switches to a Git branch named `task/<id>`.
- `finish` : Closes the task
- `submit` : Pushes the current task branch to origin for a Pull Request you are currently working on, stages the changes, and prompts you for a git commit.
- `close [<id>]` : Manually mark a task as CLOSED.
- `open [<id>]` : Manually mark a task as OPEN.

### Views & Dashboards
- `ls` : List all open and blocked tasks. Highlights the task you are currently working on in green.
- `ls --all` : List all tasks, including closed ones.
- `tree` : Visually display your tasks and their dependencies as a branching forest.
- `plan` : Print a strictly ordered execution plan, showing you exactly which tasks are unblocked and ready to be worked on.
- `status` : Print a high-level project dashboard showing task completion percentages.

### Project Setup
- `init` : Initialize IssueTracker in the current directory (creates the `tasks/` folder).
- `config <key> <value>` : Configure tracker settings (e.g., `issue-tracker config user.name "Alice"`, or `core.editor "vim"`).

## Installation

```bash
git clone https://github.com/aikoschurmann/issue-tracker.git
cd issue-tracker
./install.sh
source ~/.zshrc
```

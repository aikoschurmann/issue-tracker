#!/bin/bash
set -e

echo "==> Building Issue Tracker..."
mkdir -p build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu 2>/dev/null || nproc)

echo "==> Installing binary to /usr/local/bin/issue-tracker (may prompt for sudo)..."
sudo make install
cd ..

echo "==> Setting up ZSH autocompletion..."
mkdir -p ~/.zsh/completions

cat << 'ZSH_COMPLETION' > ~/.zsh/completions/_issue-tracker
#compdef issue-tracker

__issue_tracker_tasks() {
    local bin_path="issue-tracker"
    if ! command -v issue-tracker &>/dev/null; then
        if [[ -x "./build/issue-tracker" ]]; then
            bin_path="./build/issue-tracker"
        else
            return
        fi
    fi

    local -a tasks
    tasks=("${(@f)$($bin_path _autocomplete 2>/dev/null)}")
    compadd -a tasks
}

_issue_tracker() {
    local cmd=$words[2]

    if (( CURRENT == 2 )); then
        local -a cmds=(
            'new:Create task'
            'edit:Open a task'
            'set:Set metadata'
            'open:Mark a task as OPEN'
            'close:Mark a task as CLOSED'
            'rm:Delete a task'
            'link:Make a task depend on another'
            'unlink:Remove dependency'
            'ls:List open tasks'
            'tree:Visualize the entire project DAG'
            'requires:Visualize dependencies for a task'
            'status:View project health dashboard'
            'bottleneck:Critical path analysis'
            'burndown:ASCII velocity chart'
            'plan:Topological sort of exactly what to do next'
            'config:Get or set config'
            'log:View history of background actions'
            'undo:Undo the last action'
            'reset:Hard reset the task database'
            'help:Show help message'
        )
        _describe -t commands "issue-tracker command" cmds
        return
    fi

    if [[ "$words[$CURRENT]" == -* ]]; then
        case $cmd in
            new)
                local -a flags=('-d:Description' '--desc:Description' '-p:Priority' '--priority:Priority' '-t:Tags' '--tags:Tags' '--deps:Dependencies' '-e:Open editor' '--edit:Open editor')
                _describe -t flags "new flags" flags
                return
                ;;
            tree)
                local -a flags=('-a:Include closed' '--all:Include closed' '-c:Include closed' '--closed:Include closed' '-d:Max depth' '--depth:Max depth')
                _describe -t flags "tree flags" flags
                return
                ;;
            ls)
                local -a flags=('-a:Include closed' '--all:Include closed' '-c:Include closed' '--closed:Include closed')
                _describe -t flags "ls flags" flags
                return
                ;;
            rm)
                local -a flags=('-r:Recursive delete' '--recursive:Recursive delete')
                _describe -t flags "rm flags" flags
                return
                ;;
            set)
                local -a flags=('-p:Priority' '--priority:Priority' '-t:Tags' '--tags:Tags')
                _describe -t flags "set flags" flags
                return
                ;;
            log)
                local -a flags=('-n:Number of commits' '--all:Show all commits')
                _describe -t flags "log flags" flags
                return
                ;;
        esac
    fi

    case $cmd in
        edit|open|close|rm|requires|set)
            __issue_tracker_tasks
            ;;
        link|unlink)
            __issue_tracker_tasks
            ;;
        config)
            if (( CURRENT == 3 )); then
                local -a keys=('user.name' 'core.editor' 'ui.labels.unblocked')
                compadd -a keys
            fi
            ;;
    esac
}

compdef _issue_tracker issue-tracker
ZSH_COMPLETION

ZSHRC="$HOME/.zshrc"
touch "$ZSHRC"
if ! grep -q "fpath=(~/.zsh/completions" "$ZSHRC" 2>/dev/null; then
    echo "==> Adding autocomplete fpath to ~/.zshrc..."
    echo -e "\n# Issue Tracker Autocomplete" >> "$ZSHRC"
    echo 'fpath=(~/.zsh/completions $fpath)' >> "$ZSHRC"
    echo "autoload -Uz compinit && compinit" >> "$ZSHRC"
else
    echo "==> ZSH fpath already configured in ~/.zshrc."
fi

echo "==> Installation complete! Please restart your terminal or run: source ~/.zshrc"

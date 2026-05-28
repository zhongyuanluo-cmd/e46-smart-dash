import sqlite3
import json
import os
import glob
from datetime import datetime

db_path = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\state.vscdb')
conn = sqlite3.connect(db_path)

# Check agent-related keys
print("=== Agent session related keys ===")
for row in conn.execute("SELECT key, length(cast(value as blob)) FROM ItemTable WHERE key LIKE '%agent%' OR key LIKE '%Agent%'"):
    print(f"  {row[0]}: {row[1]} bytes")

# Check agentSessions data  
data = conn.execute("SELECT value FROM ItemTable WHERE key='agentSessions.filterExcludes.previousUserFilter'").fetchone()
if data:
    print(f"\n  agentSessions.filterExcludes: {data[0][:300]}")

# Check workbench.agentsession
data = conn.execute("SELECT value FROM ItemTable WHERE key='workbench.agentsession.welcomeComplete'").fetchone()
if data:
    print(f"\n  agentSession.welcomeComplete: {data[0]}")

# Check claude-sessions sidebar
data = conn.execute("SELECT value FROM ItemTable WHERE key='workbench.view.extension.claude-sessions-sidebar.state.hidden'").fetchone()
if data:
    print(f"\n  claude-sessions-sidebar: {data[0][:500]}")

# Check GitHub.copilot-chat data
data = conn.execute("SELECT value FROM ItemTable WHERE key='GitHub.copilot-chat'").fetchone()
if data:
    val = data[0]
    print(f"\n  GitHub.copilot-chat: {val[:1000]}")

# Check __GitHub.copilot-chat-zhongyuanluo-cmd
data = conn.execute("SELECT value FROM ItemTable WHERE key='__GitHub.copilot-chat-zhongyuanluo-cmd'").fetchone()
if data:
    print(f"\n  __GitHub.copilot-chat-zhongyuanluo-cmd: {data[0][:1000]}")

# Check agentSessions.sorting
data = conn.execute("SELECT value FROM ItemTable WHERE key='agentSessions.sorting'").fetchone()
if data:
    print(f"\n  agentSessions.sorting: {data[0]}")

conn.close()

# Check agentSessions folder
print("\n=== Agent sessions folder ===")
agent_dir = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\agentSessions')
if os.path.isdir(agent_dir):
    for item in os.listdir(agent_dir):
        print(f"  {item}")
else:
    print("  (does not exist)")

# Check for any sessions with "车机" or related content
print("\n=== Searching for '车机' in recent session files ===")
workspace_dir = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\ec830f41e101e57c7aa08a79e62c561b\chatSessions')
for f in sorted(glob.glob(os.path.join(workspace_dir, '*.jsonl')), key=os.path.getmtime, reverse=True):
    fname = os.path.basename(f)
    mtime = datetime.fromtimestamp(os.path.getmtime(f))
    try:
        with open(f, 'r', encoding='utf-8') as fh:
            content = fh.read()
            if '车机' in content or '助手' in content:
                print(f"  [{mtime}] {fname} - contains 车机/助手")
                # Show first 200 chars
                for line in content.split('\n')[:3]:
                    if '车机' in line or '助手' in line:
                        print(f"    {line[:200]}")
    except:
        pass

# Also search global sessions
global_dir = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\emptyWindowChatSessions')
for f in sorted(glob.glob(os.path.join(global_dir, '*.jsonl')), key=os.path.getmtime, reverse=True)[:15]:
    fname = os.path.basename(f)
    mtime = datetime.fromtimestamp(os.path.getmtime(f))
    try:
        with open(f, 'r', encoding='utf-8') as fh:
            content = fh.read()
            if '车机' in content or '助手' in content:
                print(f"  [{mtime}] {fname} - contains 车机/助手")
                for line in content.split('\n')[:3]:
                    if '车机' in line or '助手' in line:
                        print(f"    {line[:200]}")
    except:
        pass

print("\nDone.")

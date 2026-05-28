import sqlite3
import json
import os
from datetime import datetime

db_path = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\state.vscdb')
conn = sqlite3.connect(db_path)

# Read the full index
data = conn.execute("SELECT value FROM ItemTable WHERE key='chat.ChatSessionStore.index'").fetchone()
index = json.loads(data[0])
entries = index.get('entries', {})

print(f"Total entries in index: {len(entries)}")

# List workspace session files
ws_dir = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\ec830f41e101e57c7aa08a79e62c561b\chatSessions')
ws_sessions = {}
if os.path.isdir(ws_dir):
    for f in os.listdir(ws_dir):
        if f.endswith('.jsonl'):
            sid = f.replace('.jsonl', '')
            fp = os.path.join(ws_dir, f)
            ws_sessions[sid] = {
                'mtime': datetime.fromtimestamp(os.path.getmtime(fp)),
                'size': os.path.getsize(fp)
            }

# Check which workspace sessions are in the index
print("\n=== Workspace sessions in index? ===")
for sid, info in sorted(ws_sessions.items(), key=lambda x: x[1]['mtime'], reverse=True):
    in_index = sid in entries
    print(f"  [{info['mtime']}] {sid[:20]}... | size={info['size']:>10} | in_index={in_index}")

# Also check: how many sessions in index are from global vs workspace
global_sessions = set()
for f in os.listdir(os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\emptyWindowChatSessions')):
    if f.endswith('.jsonl'):
        global_sessions.add(f.replace('.jsonl', ''))

indexed_global = [s for s in entries if s in global_sessions]
indexed_ws = [s for s in entries if s in ws_sessions]
indexed_other = [s for s in entries if s not in global_sessions and s not in ws_sessions]

print(f"\n=== Index composition ===")
print(f"  From global storage: {len(indexed_global)}")
print(f"  From current workspace: {len(indexed_ws)}")
print(f"  From other workspaces: {len(indexed_other)} (e.g., {indexed_other[:3]})")

# Check the "对话内容消失问题" session metadata
if 'a1081c9c-4107-44e1-82f9-0d1977dcddea' in entries:
    e = entries['a1081c9c-4107-44e1-82f9-0d1977dcddea']
    print(f"\n=== '对话内容消失问题' session ===")
    print(f"  title: {e.get('title')}")
    print(f"  lastMessageDate: {datetime.fromtimestamp(e['lastMessageDate']/1000)}")
    print(f"  hasPendingEdits: {e.get('hasPendingEdits')}")
    print(f"  lastResponseState: {e.get('lastResponseState')}")
    print(f"  isEmpty: {e.get('isEmpty')}")

conn.close()

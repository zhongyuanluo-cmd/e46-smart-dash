import sqlite3
import json
import os
import glob
from datetime import datetime

DB_PATH = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\state.vscdb')

conn = sqlite3.connect(DB_PATH)
data = conn.execute("SELECT value FROM ItemTable WHERE key='chat.ChatSessionStore.index'").fetchone()
index = json.loads(data[0])
entries = index.get('entries', {})

# Check current workspace sessions
ws_dir = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\ec830f41e101e57c7aa08a79e62c561b\chatSessions')
ws_on_disk = set()
if os.path.isdir(ws_dir):
    for f in os.listdir(ws_dir):
        if f.endswith('.jsonl'):
            ws_on_disk.add(f.replace('.jsonl', ''))

print("=== Post-fix: Current workspace sessions ===")
for sid in sorted(ws_on_disk):
    info = entries.get(sid, None)
    if info:
        ts = datetime.fromtimestamp(info['lastMessageDate']/1000)
        print(f"  ✅ {sid[:20]}... | {ts} | {info.get('title', '?')[:50]}")
    else:
        print(f"  ❌ {sid[:20]}... | NOT IN INDEX!")

# Count index coverage
indexed_ws = ws_on_disk & set(entries.keys())
print(f"\n=== Index coverage ===")
print(f"  Workspace sessions on disk: {len(ws_on_disk)}")
print(f"  Workspace sessions in index: {len(indexed_ws)}")
print(f"  Coverage: {len(indexed_ws)}/{len(ws_on_disk)}")

# Check: has this current session been re-indexed after reload?
current_sid = "369e5379-2990-4c0c-a41e-f847a9681214"
if current_sid in entries:
    e = entries[current_sid]
    print(f"\n=== Current session ({current_sid[:20]}...) ===")
    print(f"  title: {e.get('title')}")
    print(f"  lastMessageDate: {datetime.fromtimestamp(e['lastMessageDate']/1000)}")
    print(f"  hasPendingEdits: {e.get('hasPendingEdits')}")
    print(f"  lastResponseState: {e.get('lastResponseState')}")

# Check for any hasPendingEdits=True sessions (this was a sync issue symptom)
pending = [(sid, e) for sid, e in entries.items() if e.get('hasPendingEdits')]
print(f"\n=== Sessions with hasPendingEdits=True: {len(pending)} ===")
for sid, e in pending:
    print(f"  ⚠️  {sid[:20]}... | {e.get('title', '?')[:50]}")

conn.close()

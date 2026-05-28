import sqlite3
import json
import os
import glob
from datetime import datetime

# Check state.vscdb
db_path = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\state.vscdb')
print("=== state.vscdb ===")
try:
    conn = sqlite3.connect(db_path)
    cursor = conn.execute("SELECT key, length(cast(value as blob)) FROM ItemTable WHERE key LIKE '%chat%' OR key LIKE '%Chat%' OR key LIKE '%session%'")
    rows = cursor.fetchall()
    for k, v in rows:
        print(f"  {k}: {v} bytes")
        if 'ChatSessionStore.index' in k:
            data = conn.execute("SELECT value FROM ItemTable WHERE key=?", (k,)).fetchone()
            if data:
                try:
                    index = json.loads(data[0])
                    print(f"    -> entries: {len(index) if isinstance(index, list) else 'not a list'}")
                    if isinstance(index, list):
                        for entry in index:
                            sid = entry.get('id', entry.get('sessionId', '?'))
                            ts = entry.get('lastMessageTime', entry.get('timestamp', 0))
                            title = entry.get('title', entry.get('name', '?'))
                            ws = entry.get('workspaceFolder', '')
                            if ts:
                                dt = datetime.fromtimestamp(ts/1000 if ts > 1e12 else ts)
                                print(f"    [{sid[:20]}...] {dt} | {title[:60]} | ws={ws[-40:]}")
                            else:
                                print(f"    [{sid[:20]}...] (no timestamp) | {title[:60]}")
                except Exception as e:
                    print(f"    Error parsing: {e}")
    conn.close()
except Exception as e:
    print(f"  Error: {e}")

# Also list all recent JSONL files
print("\n=== All session files (by modified time, most recent 30) ===")
all_files = []
for root in [
    os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\emptyWindowChatSessions'),
    os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage'),
]:
    if 'workspaceStorage' in root:
        for ws_dir in glob.glob(os.path.join(root, '*')):
            cs_dir = os.path.join(ws_dir, 'chatSessions')
            if os.path.isdir(cs_dir):
                for f in glob.glob(os.path.join(cs_dir, '*.jsonl')):
                    all_files.append(f)
    else:
        for f in glob.glob(os.path.join(root, '*.jsonl')):
            all_files.append(f)

all_files.sort(key=lambda f: os.path.getmtime(f), reverse=True)
for f in all_files[:30]:
    mtime = datetime.fromtimestamp(os.path.getmtime(f))
    size = os.path.getsize(f)
    fname = os.path.basename(f)
    parent = os.path.basename(os.path.dirname(f))
    grandparent = os.path.basename(os.path.dirname(os.path.dirname(f)))
    path_info = f"{grandparent}/{parent}" if grandparent != 'Code' else parent
    print(f"  {mtime} | {size:>10} | {path_info}/{fname}")

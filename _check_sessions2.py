import sqlite3
import json
import os
import glob
from datetime import datetime

db_path = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\state.vscdb')

print("=== ChatSessionStore.index structure ===")
conn = sqlite3.connect(db_path)
data = conn.execute("SELECT value FROM ItemTable WHERE key='chat.ChatSessionStore.index'").fetchone()
if data:
    val = data[0]
    print(f"Raw type: {type(val)}, length: {len(val)}")
    print(f"First 500 chars: {val[:500]}")
    try:
        index = json.loads(val)
        print(f"Parsed type: {type(index)}")
        if isinstance(index, dict):
            print(f"Keys: {list(index.keys())[:20]}")
            for k in list(index.keys())[:5]:
                v = index[k]
                print(f"  [{k}]: type={type(v).__name__}")
                if isinstance(v, dict):
                    print(f"    keys: {list(v.keys())}")
                    for sk, sv in v.items():
                        if isinstance(sv, str) and len(sv) > 100:
                            print(f"    {sk}: {sv[:100]}...")
                        else:
                            print(f"    {sk}: {sv}")
        elif isinstance(index, list):
            print(f"List length: {len(index)}")
            for entry in index[:5]:
                print(f"  {json.dumps(entry, ensure_ascii=False)[:200]}")
        else:
            print(f"Value: {str(index)[:500]}")
    except Exception as e:
        print(f"Parse error: {e}")

# Check debug-logs
print("\n=== Debug logs in current workspace ===")
debug_dir = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\ec830f41e101e57c7aa08a79e62c561b\GitHub.copilot-chat\debug-logs')
if os.path.isdir(debug_dir):
    for item in sorted(os.listdir(debug_dir), reverse=True):
        full = os.path.join(debug_dir, item)
        if os.path.isdir(full):
            mtime = datetime.fromtimestamp(os.path.getmtime(full))
            files = len(os.listdir(full))
            print(f"  [{mtime}] DIR {item}/ ({files} files)")
        else:
            mtime = datetime.fromtimestamp(os.path.getmtime(full))
            size = os.path.getsize(full)
            print(f"  [{mtime}] {item} ({size} bytes)")

# Search for any file modified May 27 evening (18:00-23:59)
print("\n=== Files modified May 27 18:00-23:59 ===")
base = os.path.expandvars(r'%APPDATA%\Code\User')
for root, dirs, files in os.walk(base):
    for f in files:
        fp = os.path.join(root, f)
        try:
            mt = os.path.getmtime(fp)
        except:
            continue
        dt = datetime.fromtimestamp(mt)
        if dt.date() == datetime(2026, 5, 27).date() and dt.hour >= 18:
            size = os.path.getsize(fp)
            rel = os.path.relpath(fp, base)
            print(f"  [{dt}] {size:>10} | {rel}")

conn.close()

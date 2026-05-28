import sqlite3
import json
import os
from datetime import datetime

our_ws_db = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\ec830f41e101e57c7aa08a79e62c561b\state.vscdb')
conn = sqlite3.connect(our_ws_db)

# Check ALL keys in workspace-local state.vscdb
print("=== Workspace-local state.vscdb: ALL keys ===")
for row in conn.execute("SELECT key, length(cast(value as blob)) FROM ItemTable ORDER BY key"):
    print(f"  {row[0]:60s} | {row[1]:>8} bytes")

# Specifically check for ChatSessionStore.index
data = conn.execute("SELECT value FROM ItemTable WHERE key='chat.ChatSessionStore.index'").fetchone()
if data:
    val = data[0]
    print(f"\n=== Workspace-local ChatSessionStore.index ===")
    print(f"  Size: {len(val)} bytes")
    try:
        idx = json.loads(val)
        entries = idx.get('entries', {})
        print(f"  Version: {idx.get('version')}")
        print(f"  Entries: {len(entries)}")
        for sid, e in list(entries.items())[:10]:
            ts = datetime.fromtimestamp(e['lastMessageDate']/1000) if 'lastMessageDate' in e else '?'
            print(f"    [{ts}] {sid[:30]}... | {e.get('title', '?')[:40]}")
    except Exception as e:
        print(f"  Parse error: {e}")
        print(f"  Raw: {val[:300]}")
else:
    print("\n=== NO ChatSessionStore.index in workspace-local DB! ===")

conn.close()

# Compare: check other workspaces' local indexes
print("\n=== Other workspaces' local ChatSessionStore.index ===")
ws_base = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage')
for d in sorted(os.listdir(ws_base)):
    full = os.path.join(ws_base, d)
    db_path = os.path.join(full, 'state.vscdb')
    cs_dir = os.path.join(full, 'chatSessions')
    if os.path.exists(db_path):
        conn = sqlite3.connect(db_path)
        data = conn.execute("SELECT value FROM ItemTable WHERE key='chat.ChatSessionStore.index'").fetchone()
        has_cs = os.path.isdir(cs_dir) and len(os.listdir(cs_dir)) > 0
        if data:
            try:
                idx = json.loads(data[0])
                n = len(idx.get('entries', {}))
                print(f"  {d[:20]}... | index_entries={n:>3} | chatSessions={'YES' if has_cs else 'no'}")
            except:
                print(f"  {d[:20]}... | index_entries=??? | (parse error)")
        else:
            print(f"  {d[:20]}... | NO index key | chatSessions={'YES' if has_cs else 'no'}")
        conn.close()

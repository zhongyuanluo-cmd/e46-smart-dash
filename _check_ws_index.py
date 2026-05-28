import sqlite3
import json
import os
from datetime import datetime

# Check workspace -27b63d8e's own index
ws_db = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\-27b63d8e\state.vscdb')
if os.path.exists(ws_db):
    conn = sqlite3.connect(ws_db)
    
    # Check the workspace-local index
    data = conn.execute("SELECT value FROM ItemTable WHERE key='chat.ChatSessionStore.index'").fetchone()
    if data:
        val = data[0]
        print(f"Workspace -27b63d8e ChatSessionStore.index: {len(val)} bytes")
        print(f"Raw: {val[:500]}")
        try:
            idx = json.loads(val)
            print(f"Parsed: version={idx.get('version')}, entries={len(idx.get('entries', {}))}")
        except:
            pass
    
    # Check agentSessions
    for key in ['agentSessions.activeSessionStates', 'agentSessions.state.cache', 'agentSessions.model.cache']:
        data = conn.execute("SELECT value FROM ItemTable WHERE key=?", (key,)).fetchone()
        if data:
            print(f"\n{key}: {data[0][:300]}")
    
    conn.close()

# Also check: is there a state.vscdb in our current workspace? 
our_ws = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\ec830f41e101e57c7aa08a79e62c561b')
print(f"\nOur workspace state.vscdb exists: {os.path.exists(os.path.join(our_ws, 'state.vscdb'))}")
print(f"Our workspace contents:")
for item in sorted(os.listdir(our_ws)):
    full = os.path.join(our_ws, item)
    if os.path.isdir(full):
        print(f"  [DIR] {item}/")
    else:
        print(f"  [FILE] {item}")

# Compare workspace hashes to understand naming
print("\n=== All workspace directories ===")
ws_base = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage')
for d in sorted(os.listdir(ws_base)):
    full = os.path.join(ws_base, d)
    if os.path.isdir(full):
        has_db = os.path.exists(os.path.join(full, 'state.vscdb'))
        has_chat = os.path.exists(os.path.join(full, 'chatSessions'))
        chat_count = len(os.listdir(os.path.join(full, 'chatSessions'))) if has_chat else 0
        print(f"  {d} | state.vscdb={'✅' if has_db else '❌'} | chatSessions={chat_count}")

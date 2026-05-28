"""
Fix ChatSessionStore.index: scan all session files and rebuild missing entries.
Safe approach - only modifies the index key, leaves everything else intact.
"""
import sqlite3
import json
import os
import glob
import shutil
from datetime import datetime

DB_PATH = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\state.vscdb')
BACKUP_PATH = DB_PATH + f'.backup-{datetime.now().strftime("%Y%m%d_%H%M%S")}'

# Step 1: Backup
print(f"[1/4] Backing up state.vscdb -> {os.path.basename(BACKUP_PATH)}")
shutil.copy2(DB_PATH, BACKUP_PATH)
print("       Backup OK.")

# Step 2: Scan all session files
print("[2/4] Scanning all session files...")

session_files = {}  # session_id -> {mtime, size, path, workspace_hash}
workspace_sessions = set()

# Global sessions
global_dir = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\emptyWindowChatSessions')
if os.path.isdir(global_dir):
    for f in glob.glob(os.path.join(global_dir, '*.jsonl')):
        sid = os.path.basename(f).replace('.jsonl', '')
        session_files[sid] = {
            'mtime': os.path.getmtime(f),
            'size': os.path.getsize(f),
            'path': f,
            'source': 'global'
        }

# Workspace sessions
ws_base = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage')
for ws_dir in glob.glob(os.path.join(ws_base, '*')):
    cs_dir = os.path.join(ws_dir, 'chatSessions')
    if os.path.isdir(cs_dir):
        ws_hash = os.path.basename(ws_dir)
        for f in glob.glob(os.path.join(cs_dir, '*.jsonl')):
            sid = os.path.basename(f).replace('.jsonl', '')
            workspace_sessions.add(sid)
            # Use the most recent version if duplicates exist across workspaces
            mtime = os.path.getmtime(f)
            if sid not in session_files or mtime > session_files[sid]['mtime']:
                session_files[sid] = {
                    'mtime': mtime,
                    'size': os.path.getsize(f),
                    'path': f,
                    'source': f'workspace/{ws_hash}'
                }

print(f"       Found {len(session_files)} session files ({len(workspace_sessions)} workspace, {len(session_files) - len(workspace_sessions)} global)")

# Step 3: Read current index
print("[3/4] Reading current index...")
conn = sqlite3.connect(DB_PATH)
data = conn.execute("SELECT value FROM ItemTable WHERE key='chat.ChatSessionStore.index'").fetchone()
current_index = json.loads(data[0])
entries = current_index.get('entries', {})
print(f"       Current entries: {len(entries)}")

# Step 4: Build missing entries
print("[4/4] Adding missing sessions...")
added = 0
for sid, info in session_files.items():
    if sid in entries:
        continue
    
    # Try to read title from the session file
    title = 'New Chat'
    try:
        with open(info['path'], 'r', encoding='utf-8') as fh:
            first_line = fh.readline().strip()
            if first_line:
                data = json.loads(first_line)
                if data.get('kind') == 0:
                    v = data.get('v', {})
                    title = v.get('customTitle', 'New Chat')
                # Also check kind:1 for customTitle
            # Read a few more lines for customTitle
            for _ in range(10):
                line = fh.readline().strip()
                if not line:
                    break
                try:
                    d = json.loads(line)
                    if d.get('kind') == 1 and d.get('k') == ['customTitle']:
                        title = d.get('v', title)
                        break
                except:
                    pass
    except Exception as e:
        print(f"       Warning: could not read title for {sid}: {e}")
    
    mtime_ms = int(info['mtime'] * 1000)
    
    entries[sid] = {
        'sessionId': sid,
        'title': title,
        'lastMessageDate': mtime_ms,
        'timing': {'created': mtime_ms},
        'initialLocation': 'panel',
        'hasPendingEdits': False,
        'isEmpty': info['size'] < 500,
        'isExternal': False,
        'lastResponseState': 1,
        'permissionLevel': 'default'
    }
    added += 1
    print(f"       + {sid[:20]}... | {title[:50]} | {info['source']}")

# Write back
new_index = {'version': current_index.get('version', 1), 'entries': entries}
new_value = json.dumps(new_index, ensure_ascii=False)

conn.execute("UPDATE ItemTable SET value=? WHERE key='chat.ChatSessionStore.index'", (new_value,))
conn.commit()
conn.close()

print(f"\n✅ Done! Added {added} sessions. Index now has {len(entries)} entries.")
print(f"   Backup saved to: {BACKUP_PATH}")
print(f"\n   ⚠️  You need to reload VS Code (Ctrl+Shift+P -> 'Developer: Reload Window')")
print(f"       for the changes to take effect.")

import sqlite3
import json
import os
import glob
from datetime import datetime

DB_PATH = os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\state.vscdb')

conn = sqlite3.connect(DB_PATH)

# Check ALL keys to see if there's a workspace-specific index
print("=== All keys related to chat/session storage ===")
for row in conn.execute("SELECT key, length(cast(value as blob)) FROM ItemTable WHERE key LIKE '%chat%' OR key LIKE '%Chat%' OR key LIKE '%session%' OR key LIKE '%Session%'"):
    print(f"  {row[0]:60s} | {row[1]:>8} bytes")

# Check if any key contains workspace hash
print("\n=== Keys containing workspace hash fragments ===")
for row in conn.execute("SELECT key FROM ItemTable WHERE key LIKE '%ec830f41%' OR key LIKE '%workspace%' OR key LIKE '%Workspace%'"):
    print(f"  {row[0]}")

# Check workspace directory for its own state.vscdb
ws_base = os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage')
for ws_dir in glob.glob(os.path.join(ws_base, '*')):
    state_file = os.path.join(ws_dir, 'state.vscdb')
    if os.path.exists(state_file):
        print(f"\n=== Workspace {os.path.basename(ws_dir)} has its own state.vscdb! ===")
        ws_conn = sqlite3.connect(state_file)
        for row in ws_conn.execute("SELECT key, length(cast(value as blob)) FROM ItemTable WHERE key LIKE '%chat%' OR key LIKE '%Chat%' OR key LIKE '%session%'"):
            print(f"  {row[0]:50s} | {row[1]:>8} bytes")
        ws_conn.close()

conn.close()

# Check if there is a separate agent sessions storage
print("\n=== Agent sessions storage locations ===")
for loc in [
    os.path.expandvars(r'%APPDATA%\Code\User\globalStorage\agentSessions'),
    os.path.expandvars(r'%APPDATA%\Code\User\workspaceStorage\ec830f41e101e57c7aa08a79e62c561b\agentSessions'),
]:
    if os.path.isdir(loc):
        print(f"  EXISTS: {loc}")
        for f in os.listdir(loc):
            print(f"    {f}")
    else:
        print(f"  NOT FOUND: {loc}")

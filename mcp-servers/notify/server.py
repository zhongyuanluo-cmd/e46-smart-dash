"""MCP Server for sending WeChat notifications via Server酱 (sct.ftqq.com).

Uses the official `mcp` SDK for proper JSON-RPC stdio transport.

Requires:
    pip install mcp requests

Usage:
    Set SERVERCHAN_SENDKEY env var, then register in .vscode/mcp.json.
"""

import os

import requests
from mcp.server import Server
from mcp.server.stdio import stdio_server
from mcp.types import TextContent, Tool

# Server酱 API endpoint
SERVERCHAN_URL = "https://sctapi.ftqq.com/{sendkey}.send"

# Create MCP server instance
app = Server("notify-server")


def _get_sendkey() -> str:
    key = os.environ.get("SERVERCHAN_SENDKEY", "")
    if not key:
        raise RuntimeError(
            "SERVERCHAN_SENDKEY 未设置。请在环境变量中设置你的 Server酱 SendKey。\n"
            "获取方式：访问 https://sct.ftqq.com/ 用微信扫码登录，复制 SENDKEY。"
        )
    return key


def _send_notification(title: str, content: str = "", channel: str = "9") -> dict:
    """发送微信通知。

    Args:
        title: 通知标题（必填，最多256字节）
        content: 通知正文，支持 Markdown
        channel: 推送渠道（默认 9=方糖服务号）

    Returns:
        API 响应 dict
    """
    sendkey = _get_sendkey()
    url = SERVERCHAN_URL.format(sendkey=sendkey)

    # 截断标题到 256 字节
    title_bytes = title.encode("utf-8")
    if len(title_bytes) > 256:
        title = title_bytes[:253].decode("utf-8", errors="ignore") + "..."

    resp = requests.post(url, data={
        "title": title,
        "desp": content,
        "channel": channel,
    }, timeout=10)
    resp.raise_for_status()
    return resp.json()


@app.list_tools()
async def list_tools() -> list[Tool]:
    """Return the list of available tools."""
    return [
        Tool(
            name="send_notification",
            description="通过 Server酱 向微信发送通知消息。用于任务完成后通知手机。",
            inputSchema={
                "type": "object",
                "properties": {
                    "title": {
                        "type": "string",
                        "description": "通知标题",
                    },
                    "content": {
                        "type": "string",
                        "description": "通知正文（支持 Markdown）",
                    },
                },
                "required": ["title"],
            },
        )
    ]


@app.call_tool()
async def call_tool(name: str, arguments: dict) -> list[TextContent]:
    """Handle tool calls."""
    if name == "send_notification":
        try:
            result = _send_notification(
                title=arguments.get("title", "无标题"),
                content=arguments.get("content", ""),
            )
            return [
                TextContent(
                    type="text",
                    text=f"✅ 通知已发送\n"
                         f"状态: {result.get('code', 'unknown')}\n"
                         f"消息: {result.get('message', '')}",
                )
            ]
        except Exception as e:
            return [
                TextContent(
                    type="text",
                    text=f"❌ 发送失败: {e}",
                )
            ]
    else:
        raise ValueError(f"Unknown tool: {name}")


async def main():
    """Run the MCP server using stdio transport."""
    async with stdio_server() as (read_stream, write_stream):
        await app.run(read_stream, write_stream, app.create_initialization_options())


if __name__ == "__main__":
    import asyncio
    asyncio.run(main())

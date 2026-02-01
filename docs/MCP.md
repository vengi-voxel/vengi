# VoxEdit MCP Server

MCP (Model Context Protocol) server enabling LLMs to control VoxEdit via its network interface.

## MCP Client Configuration

```json
{
  "mcpServers": {
    "voxedit": {
      "command": "/path/to/vengi-mcp",
      "args": ["--host", "127.0.0.1", "--port", "10001", "--rcon", "changeme", "--password", "changeme"]
    }
  }
}
```

## Command Line Options

| Option | Short | Default | Description |
|--------|-------|---------|-------------|
| `--host` | `-H` | 127.0.0.1 | VoxEdit server host |
| `--port` | `-p` | 10001 | VoxEdit server port |
| `--rcon` | | changeme | RCON password for commands |
| `--password` | | changeme | Connection password |
| `--state-dir` | | /tmp/vengi-mcp | Directory for temp files |

#!/usr/bin/env python3
"""
Eagle Software - NeoSD ROM Packager & Converter GUI
Provides an interactive graphical interface for packing, unpacking,
and inspecting Neo Geo NeoSD (.neo) ROM sets.

Works seamlessly via local Web UI (zero third-party dependencies) or
native desktop Tkinter when available.
"""

import os
import sys
import json
import subprocess
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import parse_qs, urlparse
import threading
import time

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SDK_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
CLI_BINARY = os.path.join(SCRIPT_DIR, "eagle_neosd")
if os.name == "nt":
    CLI_BINARY += ".exe"

HTML_CONTENT = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Eagle Software - NeoSD Converter</title>
    <style>
        :root {
            --bg-color: #0d1117;
            --card-bg: #161b22;
            --border-color: #30363d;
            --accent-color: #58a6ff;
            --accent-hover: #1f6feb;
            --gold: #f2cc60;
            --success: #238636;
            --text-primary: #c9d1d9;
            --text-secondary: #8b949e;
            --font-mono: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, monospace;
        }

        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
            background-color: var(--bg-color);
            color: var(--text-primary);
            padding: 24px;
            display: flex;
            justify-content: center;
        }

        .container {
            width: 100%;
            max-width: 960px;
        }

        header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 16px;
            margin-bottom: 24px;
        }

        .brand {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        .brand-badge {
            background: linear-gradient(135deg, #f2cc60, #d29922);
            color: #000;
            font-weight: 800;
            padding: 6px 12px;
            border-radius: 6px;
            font-size: 14px;
            letter-spacing: 1px;
            text-transform: uppercase;
        }

        .brand h1 {
            font-size: 22px;
            font-weight: 600;
            color: #fff;
        }

        .brand p {
            font-size: 13px;
            color: var(--text-secondary);
        }

        .tabs {
            display: flex;
            gap: 8px;
            margin-bottom: 20px;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 8px;
        }

        .tab-btn {
            background: transparent;
            border: 1px solid transparent;
            color: var(--text-secondary);
            padding: 8px 18px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 14px;
            font-weight: 600;
            transition: all 0.2s;
        }

        .tab-btn:hover {
            color: var(--text-primary);
            background-color: rgba(110, 118, 129, 0.1);
        }

        .tab-btn.active {
            color: var(--accent-color);
            background-color: rgba(88, 166, 255, 0.15);
            border-color: rgba(88, 166, 255, 0.4);
        }

        .panel {
            display: none;
            background-color: var(--card-bg);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 24px;
            margin-bottom: 24px;
        }

        .panel.active {
            display: block;
        }

        .form-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 16px;
            margin-bottom: 20px;
        }

        .form-full {
            grid-column: span 2;
        }

        label {
            display: block;
            font-size: 13px;
            font-weight: 600;
            color: var(--text-secondary);
            margin-bottom: 6px;
        }

        input[type="text"], input[type="number"], select {
            width: 100%;
            background-color: #0d1117;
            border: 1px solid var(--border-color);
            border-radius: 6px;
            color: var(--text-primary);
            padding: 8px 12px;
            font-size: 14px;
            outline: none;
            transition: border-color 0.2s;
        }

        input[type="text"]:focus, select:focus {
            border-color: var(--accent-color);
        }

        .preset-row {
            display: flex;
            gap: 10px;
            margin-bottom: 16px;
            align-items: center;
        }

        .preset-chip {
            background-color: #21262d;
            border: 1px solid var(--border-color);
            color: var(--text-primary);
            padding: 6px 14px;
            border-radius: 20px;
            font-size: 13px;
            cursor: pointer;
            transition: all 0.2s;
        }

        .preset-chip:hover {
            border-color: var(--gold);
            color: var(--gold);
        }

        .btn-primary {
            background-color: var(--success);
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 10px 24px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: filter 0.2s;
            display: inline-flex;
            align-items: center;
            gap: 8px;
        }

        .btn-primary:hover {
            filter: brightness(1.15);
        }

        .btn-secondary {
            background-color: #21262d;
            color: var(--text-primary);
            border: 1px solid var(--border-color);
            border-radius: 6px;
            padding: 10px 20px;
            font-size: 14px;
            cursor: pointer;
            transition: all 0.2s;
        }

        .btn-secondary:hover {
            border-color: var(--accent-color);
            color: var(--accent-color);
        }

        .action-bar {
            display: flex;
            justify-content: flex-end;
            gap: 12px;
            border-top: 1px solid var(--border-color);
            padding-top: 18px;
        }

        .terminal-card {
            background-color: #05070a;
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 16px;
            margin-top: 20px;
        }

        .terminal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.08);
            padding-bottom: 8px;
        }

        .terminal-title {
            font-size: 12px;
            font-weight: 700;
            color: var(--gold);
            text-transform: uppercase;
            letter-spacing: 1px;
        }

        pre#outputTerminal {
            font-family: var(--font-mono);
            font-size: 13px;
            color: #7ee787;
            white-space: pre-wrap;
            word-break: break-all;
            max-height: 320px;
            overflow-y: auto;
            line-height: 1.45;
        }

        .badge-tag {
            display: inline-block;
            font-size: 11px;
            font-weight: 600;
            padding: 2px 6px;
            border-radius: 4px;
            background: #21262d;
            color: var(--accent-color);
            border: 1px solid var(--border-color);
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <div class="brand">
                <div class="brand-badge">Eagle</div>
                <div>
                    <h1>NeoSD ROM Packager & Converter</h1>
                    <p>High performance custom converter for Eagle Software & Neo Geo homebrews</p>
                </div>
            </div>
            <div>
                <span class="badge-tag">v1.0.0</span>
            </div>
        </header>

        <div class="tabs">
            <button class="tab-btn active" onclick="showTab('pack')">Pack ROM to .neo</button>
            <button class="tab-btn" onclick="showTab('info')">Inspect .neo File</button>
            <button class="tab-btn" onclick="showTab('unpack')">Extract / Unpack .neo</button>
        </div>

        <!-- PACK PANEL -->
        <div id="panel-pack" class="panel active">
            <div class="preset-row">
                <span style="font-size: 13px; color: var(--text-secondary); margin-right: 4px;">Quick Presets:</span>
                <button type="button" class="preset-chip" onclick="applyPreset('maiya')">Maiya (Eagle 780)</button>
                <button type="button" class="preset-chip" onclick="applyPreset('demo')">Demo (Eagle 777)</button>
            </div>

            <div class="form-grid">
                <div class="form-full">
                    <label for="packInputDir">Input ROM Directory Path</label>
                    <input type="text" id="packInputDir" placeholder="roms/maiya or absolute path">
                </div>
                <div class="form-full">
                    <label for="packOutputFile">Output .neo File Path</label>
                    <input type="text" id="packOutputFile" placeholder="out/maiya.neo">
                </div>
                <div>
                    <label for="packGameName">Game Title</label>
                    <input type="text" id="packGameName" placeholder="e.g. Maiya: Super Nature Girl">
                </div>
                <div>
                    <label for="packManufacturer">Manufacturer</label>
                    <input type="text" id="packManufacturer" value="Eagle Software">
                </div>
                <div>
                    <label for="packYear">Release Year</label>
                    <input type="number" id="packYear" value="2026">
                </div>
                <div>
                    <label for="packGenre">Genre</label>
                    <select id="packGenre">
                        <option value="5" selected>Platformer (5)</option>
                        <option value="1">Action (1)</option>
                        <option value="2">BeatEmUp (2)</option>
                        <option value="3">Sports (3)</option>
                        <option value="4">Driving (4)</option>
                        <option value="6">Shooter (6)</option>
                        <option value="7">Puzzle (7)</option>
                        <option value="8">Fighting (8)</option>
                        <option value="9">RPG (9)</option>
                        <option value="10">Strategy (10)</option>
                        <option value="0">Unknown (0)</option>
                    </select>
                </div>
                <div>
                    <label for="packNgh">NGH ID (Hex e.g. 0x8007 or Dec e.g. 780)</label>
                    <input type="text" id="packNgh" placeholder="Auto-detected from P-ROM if blank">
                </div>
            </div>

            <div class="action-bar">
                <button type="button" class="btn-secondary" onclick="scanDirectory()">Scan Directory</button>
                <button type="button" class="btn-primary" onclick="packNeo()">Package to .neo</button>
            </div>
        </div>

        <!-- INFO PANEL -->
        <div id="panel-info" class="panel">
            <div class="form-grid">
                <div class="form-full">
                    <label for="infoInputFile">Select / Enter .neo File Path</label>
                    <input type="text" id="infoInputFile" placeholder="Path to game.neo file">
                </div>
            </div>
            <div class="action-bar">
                <button type="button" class="btn-primary" onclick="inspectNeo()">Inspect Header & Regions</button>
            </div>
        </div>

        <!-- UNPACK PANEL -->
        <div id="panel-unpack" class="panel">
            <div class="form-grid">
                <div class="form-full">
                    <label for="unpackInputFile">Input .neo File Path</label>
                    <input type="text" id="unpackInputFile" placeholder="Path to game.neo file">
                </div>
                <div class="form-full">
                    <label for="unpackOutputDir">Extraction Directory Path</label>
                    <input type="text" id="unpackOutputDir" placeholder="Directory where loose ROMs will be written">
                </div>
            </div>
            <div class="action-bar">
                <button type="button" class="btn-primary" onclick="unpackNeo()">Extract ROM Files</button>
            </div>
        </div>

        <!-- CONSOLE TERMINAL -->
        <div class="terminal-card">
            <div class="terminal-header">
                <span class="terminal-title">Console Output</span>
                <button class="btn-secondary" style="padding: 4px 8px; font-size: 11px;" onclick="clearConsole()">Clear</button>
            </div>
            <pre id="outputTerminal">Ready. Select a preset or specify directory to begin.</pre>
        </div>
    </div>

    <script>
        function showTab(name) {
            document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
            document.querySelectorAll('.panel').forEach(p => p.classList.remove('active'));

            if (name === 'pack') {
                document.querySelectorAll('.tab-btn')[0].classList.add('active');
                document.getElementById('panel-pack').classList.add('active');
            } else if (name === 'info') {
                document.querySelectorAll('.tab-btn')[1].classList.add('active');
                document.getElementById('panel-info').classList.add('active');
            } else if (name === 'unpack') {
                document.querySelectorAll('.tab-btn')[2].classList.add('active');
                document.getElementById('panel-unpack').classList.add('active');
            }
        }

        function log(msg) {
            const term = document.getElementById('outputTerminal');
            term.textContent += "\\n" + msg;
            term.scrollTop = term.scrollHeight;
        }

        function clearConsole() {
            document.getElementById('outputTerminal').textContent = "Console cleared.";
        }

        function applyPreset(name) {
            if (name === 'maiya') {
                document.getElementById('packInputDir').value = 'roms/maiya';
                document.getElementById('packOutputFile').value = 'roms/maiya.neo';
                document.getElementById('packGameName').value = 'Maiya: Super Nature Girl';
                document.getElementById('packManufacturer').value = 'Eagle Software';
                document.getElementById('packYear').value = '2026';
                document.getElementById('packGenre').value = '5';
                document.getElementById('packNgh').value = '0x8007';
                log("[Preset] Loaded Maiya (Eagle NGH 780 / 0x8007)");
            } else if (name === 'demo') {
                document.getElementById('packInputDir').value = 'roms/demo';
                document.getElementById('packOutputFile').value = 'roms/demo.neo';
                document.getElementById('packGameName').value = 'Neo Geo SDK Demo';
                document.getElementById('packManufacturer').value = 'Eagle Software';
                document.getElementById('packYear').value = '2026';
                document.getElementById('packGenre').value = '1';
                document.getElementById('packNgh').value = '0x8006';
                log("[Preset] Loaded Neo Geo Demo (Eagle NGH 777 / 0x8006)");
            }
        }

        async function scanDirectory() {
            const dir = document.getElementById('packInputDir').value.trim();
            if (!dir) {
                log("[Error] Please enter an input directory path first.");
                return;
            }
            log("[Scan] Scanning directory: " + dir + " ...");
            try {
                const res = await fetch('/api/scan?dir=' + encodeURIComponent(dir));
                const data = await res.json();
                if (data.status === 'ok') {
                    log(data.message);
                } else {
                    log("[Error] " + data.message);
                }
            } catch (e) {
                log("[Error] Network / Server failure: " + e);
            }
        }

        async function packNeo() {
            const inputDir = document.getElementById('packInputDir').value.trim();
            const outputFile = document.getElementById('packOutputFile').value.trim();
            const gameName = document.getElementById('packGameName').value.trim();
            const manu = document.getElementById('packManufacturer').value.trim();
            const year = document.getElementById('packYear').value.trim();
            const genre = document.getElementById('packGenre').value;
            const ngh = document.getElementById('packNgh').value.trim();

            if (!inputDir || !outputFile) {
                log("[Error] Input Directory and Output File are both required.");
                return;
            }

            log("[Pack] Packaging " + inputDir + " -> " + outputFile + " ...");
            try {
                const res = await fetch('/api/pack', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        inputDir, outputFile, gameName, manufacturer: manu, year, genre, ngh
                    })
                });
                const data = await res.json();
                if (data.status === 'ok') {
                    log(data.output);
                } else {
                    log("[Error] " + data.message + "\\n" + (data.output || ""));
                }
            } catch (e) {
                log("[Error] Execution failed: " + e);
            }
        }

        async function inspectNeo() {
            const file = document.getElementById('infoInputFile').value.trim();
            if (!file) {
                log("[Error] Please enter a .neo file path to inspect.");
                return;
            }
            log("[Inspect] Inspecting: " + file + " ...");
            try {
                const res = await fetch('/api/info?file=' + encodeURIComponent(file));
                const data = await res.json();
                if (data.status === 'ok') {
                    log(data.output);
                } else {
                    log("[Error] " + data.message + "\\n" + (data.output || ""));
                }
            } catch (e) {
                log("[Error] Inspect failed: " + e);
            }
        }

        async function unpackNeo() {
            const file = document.getElementById('unpackInputFile').value.trim();
            const outDir = document.getElementById('unpackOutputDir').value.trim();
            if (!file || !outDir) {
                log("[Error] Input .neo file and Output Directory are both required.");
                return;
            }
            log("[Unpack] Extracting " + file + " -> " + outDir + " ...");
            try {
                const res = await fetch('/api/unpack', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({ file, outDir })
                });
                const data = await res.json();
                if (data.status === 'ok') {
                    log(data.output);
                } else {
                    log("[Error] " + data.message + "\\n" + (data.output || ""));
                }
            } catch (e) {
                log("[Error] Extract failed: " + e);
            }
        }
    </script>
</body>
</html>
"""


class NeoSDRequestHandler(BaseHTTPRequestHandler):
    def _send_json(self, status_code, data):
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate")
        self.end_headers()
        self.wfile.write(json.dumps(data).encode("utf-8"))

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path in ("/", "/index.html"):
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(HTML_CONTENT.encode("utf-8"))
        elif parsed.path == "/api/scan":
            query = parse_qs(parsed.query)
            dir_path = query.get("dir", [""])[0]
            if not dir_path:
                self._send_json(400, {"status": "error", "message": "Missing dir parameter"})
                return

            abs_path = os.path.join(SDK_ROOT, dir_path) if not os.path.isabs(dir_path) else dir_path
            if not os.path.exists(abs_path):
                self._send_json(404, {"status": "error", "message": f"Directory not found: {dir_path}"})
                return

            files = os.listdir(abs_path)
            found = []
            for f in sorted(files):
                size = os.path.getsize(os.path.join(abs_path, f))
                found.append(f"{f} ({size // 1024} KB)")

            msg = f"Found {len(found)} files in {dir_path}:\n  " + "\n  ".join(found)
            self._send_json(200, {"status": "ok", "message": msg, "files": found})

        elif parsed.path == "/api/info":
            query = parse_qs(parsed.query)
            file_path = query.get("file", [""])[0]
            if not file_path:
                self._send_json(400, {"status": "error", "message": "Missing file parameter"})
                return

            abs_path = os.path.join(SDK_ROOT, file_path) if not os.path.isabs(file_path) else file_path
            if not os.path.exists(abs_path):
                self._send_json(404, {"status": "error", "message": f"File not found: {file_path}"})
                return

            cmd = [CLI_BINARY, "--info", abs_path]
            try:
                p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                self._send_json(200, {"status": "ok", "output": p.stdout})
            except Exception as e:
                self._send_json(500, {"status": "error", "message": str(e)})
        else:
            self.send_error(404, "Not Found")

    def do_POST(self):
        parsed = urlparse(self.path)
        content_len = int(self.headers.get("Content-Length", 0))
        post_body = self.rfile.read(content_len)
        try:
            payload = json.loads(post_body.decode("utf-8"))
        except Exception:
            payload = {}

        if parsed.path == "/api/pack":
            in_dir = payload.get("inputDir", "")
            out_file = payload.get("outputFile", "")
            if not in_dir or not out_file:
                self._send_json(400, {"status": "error", "message": "inputDir and outputFile required"})
                return

            abs_in = os.path.join(SDK_ROOT, in_dir) if not os.path.isabs(in_dir) else in_dir
            abs_out = os.path.join(SDK_ROOT, out_file) if not os.path.isabs(out_file) else out_file

            out_dir = os.path.dirname(abs_out)
            if out_dir and not os.path.exists(out_dir):
                os.makedirs(out_dir, exist_ok=True)

            cmd = [CLI_BINARY, "-i", abs_in, "-o", abs_out]
            if payload.get("gameName"):
                cmd.extend(["-n", payload["gameName"]])
            if payload.get("manufacturer"):
                cmd.extend(["-m", payload["manufacturer"]])
            if payload.get("year"):
                cmd.extend(["-y", str(payload["year"])])
            if payload.get("genre"):
                cmd.extend(["-g", str(payload["genre"])])
            if payload.get("ngh"):
                cmd.extend(["--ngh", str(payload["ngh"])])

            try:
                p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                if p.returncode == 0:
                    self._send_json(200, {"status": "ok", "output": p.stdout})
                else:
                    self._send_json(400, {"status": "error", "message": "Packaging failed", "output": p.stdout})
            except Exception as e:
                self._send_json(500, {"status": "error", "message": str(e)})

        elif parsed.path == "/api/unpack":
            file_path = payload.get("file", "")
            out_dir = payload.get("outDir", "")
            if not file_path or not out_dir:
                self._send_json(400, {"status": "error", "message": "file and outDir required"})
                return

            abs_file = os.path.join(SDK_ROOT, file_path) if not os.path.isabs(file_path) else file_path
            abs_out = os.path.join(SDK_ROOT, out_dir) if not os.path.isabs(out_dir) else out_dir

            os.makedirs(abs_out, exist_ok=True)
            cmd = [CLI_BINARY, "-x", abs_file, "-o", abs_out]
            try:
                p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                if p.returncode == 0:
                    self._send_json(200, {"status": "ok", "output": p.stdout})
                else:
                    self._send_json(400, {"status": "error", "message": "Extraction failed", "output": p.stdout})
            except Exception as e:
                self._send_json(500, {"status": "error", "message": str(e)})
        else:
            self.send_error(404, "Not Found")

    def log_message(self, format, *args):
        # Suppress verbose default access logging
        pass


def run_web_server(port=7800):
    server_address = ("", port)
    httpd = HTTPServer(server_address, NeoSDRequestHandler)
    print("=" * 60)
    print("  Eagle Software - NeoSD Converter GUI Web Server")
    print("=" * 60)
    print(f"  Access the GUI at: http://localhost:{port}")
    print(f"  Underlying C binary: {CLI_BINARY}")
    print("  Press Ctrl+C to exit.")
    print("=" * 60)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down server...")
        httpd.server_close()


def try_run_tkinter():
    try:
        import tkinter as tk
        from tkinter import ttk, filedialog, messagebox
    except ImportError:
        return False

    root = tk.Tk()
    root.title("Eagle Software - NeoSD Converter")
    root.geometry("640x480")

    notebook = ttk.Notebook(root)
    notebook.pack(fill="both", expand=True, padx=10, pady=10)

    # Pack Frame
    frame_pack = ttk.Frame(notebook)
    notebook.add(frame_pack, text="Pack .neo")

    ttk.Label(frame_pack, text="Input ROM Directory:").grid(row=0, column=0, sticky="w", padx=5, pady=5)
    in_dir_var = tk.StringVar(value="roms/maiya")
    ttk.Entry(frame_pack, textvariable=in_dir_var, width=40).grid(row=0, column=1, padx=5, pady=5)

    def browse_dir():
        d = filedialog.askdirectory()
        if d:
            in_dir_var.set(d)

    ttk.Button(frame_pack, text="Browse", command=browse_dir).grid(row=0, column=2, padx=5, pady=5)

    ttk.Label(frame_pack, text="Output .neo File:").grid(row=1, column=0, sticky="w", padx=5, pady=5)
    out_file_var = tk.StringVar(value="roms/maiya.neo")
    ttk.Entry(frame_pack, textvariable=out_file_var, width=40).grid(row=1, column=1, padx=5, pady=5)

    def browse_out():
        f = filedialog.asksaveasfilename(defaultextension=".neo", filetypes=[("NeoSD ROM", "*.neo")])
        if f:
            out_file_var.set(f)

    ttk.Button(frame_pack, text="Browse", command=browse_out).grid(row=1, column=2, padx=5, pady=5)

    log_box = tk.Text(frame_pack, height=12, width=70)
    log_box.grid(row=3, column=0, columnspan=3, padx=5, pady=10)

    def do_pack():
        log_box.delete("1.0", tk.END)
        cmd = [CLI_BINARY, "-i", in_dir_var.get(), "-o", out_file_var.get()]
        try:
            res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            log_box.insert(tk.END, res.stdout)
        except Exception as err:
            messagebox.showerror("Error", str(err))

    ttk.Button(frame_pack, text="Build .neo ROM", command=do_pack).grid(row=2, column=1, pady=10)

    root.mainloop()
    return True


if __name__ == "__main__":
    if "--tk" in sys.argv:
        if not try_run_tkinter():
            print("[Warning] Tkinter is not available in this environment. Falling back to local Web GUI...")
            run_web_server()
    else:
        # Default to high-performance zero-dependency web GUI
        run_web_server()

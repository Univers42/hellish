#!/usr/bin/env python3
"""Fake LLM endpoint for hellish's AI tests — no real model or GPU needed.

Speaks both the OpenAI chat-completions shape and the Anthropic Messages shape,
and records every request (path, JSON body, headers) to $FAKE_LLM_LOG so tests
can assert what hellish actually sent. Env knobs:
  FAKE_LLM_PORT   port to listen on (default 8099)
  FAKE_LLM_LOG    append each received request as one JSON line
  FAKE_LLM_REPLY  assistant text to return (default "echo faked")
  FAKE_LLM_DELAY  seconds to sleep before replying (simulate a slow backend)
"""
import json
import os
import sys
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

PORT = int(os.environ.get("FAKE_LLM_PORT", "8099"))
LOG = os.environ.get("FAKE_LLM_LOG", "")
REPLY = os.environ.get("FAKE_LLM_REPLY", "echo faked")
DELAY = float(os.environ.get("FAKE_LLM_DELAY", "0"))


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def _record(self, path, body):
        if not LOG:
            return
        try:
            parsed = json.loads(body)
        except Exception:
            parsed = {"_raw": body}
        hdrs = {k.lower(): v for k, v in self.headers.items()}
        with open(LOG, "a") as f:
            f.write(json.dumps({"path": path, "body": parsed,
                                "headers": hdrs}) + "\n")

    def do_GET(self):
        self._send(200, json.dumps(
            {"data": [{"id": "fake-model-1"}, {"id": "fake-model-2"}]}))

    def do_POST(self):
        n = int(self.headers.get("Content-Length", "0"))
        body = self.rfile.read(n).decode("utf-8", "replace")
        self._record(self.path, body)
        if DELAY:
            time.sleep(DELAY)
        if "/v1/messages" in self.path:          # Anthropic shape
            payload = {"role": "assistant",
                       "content": [{"type": "text", "text": REPLY}]}
        else:                                     # OpenAI chat shape
            payload = {"choices": [{"message": {"role": "assistant",
                       "content": REPLY}}]}
        self._send(200, json.dumps(payload))

    def _send(self, code, text):
        data = text.encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        try:
            self.wfile.write(data)
        except BrokenPipeError:
            pass


if __name__ == "__main__":
    srv = HTTPServer(("127.0.0.1", PORT), Handler)
    sys.stderr.write(f"fake_llm on 127.0.0.1:{PORT}\n")
    sys.stderr.flush()
    srv.serve_forever()

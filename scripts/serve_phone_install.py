"""Temporarily serve only the built app and source WAV on a user-selected LAN IP."""
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlsplit
import argparse
import threading
import shutil

ROOT=Path(__file__).resolve().parents[1]
FILES={'/UniversalANC-Lab.apk':('application/vnd.android.package-archive',ROOT/'dist/UniversalANC-Lab.apk'),
       '/reference_300Hz.wav':('audio/wav',ROOT/'dist/reference_300Hz.wav')}
PAGE=b'''<!doctype html><meta name="viewport" content="width=device-width, initial-scale=1"><title>Universal ANC Lab</title><body style="font:18px system-ui;max-width:650px;margin:40px auto;padding:20px"><h1>Universal ANC Lab</h1><p>Experimental single-tone microphone-point cancellation. Physical headset performance is not verified.</p><p><a href="/UniversalANC-Lab.apk">Download Android app (version 2.2)</a></p><p>Install manually, open Universal ANC Lab, set 300 Hz and choose PLAY SOURCE TONE on phone speaker. Keep the app open. On the laptop, connect the Crystl earbuds, turn manufacturer ANC/Ambient off, and fix the LEFT earbud beside the laptop microphone outside your ears. Start the 32-second test in the desktop app.</p><p>For the opposite setup, connect earbuds to the phone and play this file through the laptop speakers: <a href="/reference_300Hz.wav">300 Hz source WAV</a>. Read docs/LIVE_TONAL_DEMO.md in the project for the full procedure.</p><p>This local page accepts no uploads and serves no microphone recordings.</p></body>'''
class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        path=urlsplit(self.path).path
        if path=='/':
            self.send_response(200);self.send_header('Content-Type','text/html; charset=utf-8');self.send_header('Content-Length',str(len(PAGE)));self.send_header('Cache-Control','no-store');self.end_headers();self.wfile.write(PAGE);return
        if path not in FILES:self.send_error(404);return
        kind,file=FILES[path]
        if not file.is_file():self.send_error(404);return
        self.send_response(200);self.send_header('Content-Type',kind);self.send_header('Content-Length',str(file.stat().st_size));self.send_header('Content-Disposition',f'attachment; filename="{file.name}"');self.send_header('Cache-Control','no-store');self.end_headers()
        try:
            with file.open('rb') as stream:shutil.copyfileobj(stream,self.wfile)
        except (BrokenPipeError,ConnectionResetError):pass
    def log_message(self,*args):pass
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--bind',default='127.0.0.1');p.add_argument('--port',type=int,default=8765);p.add_argument('--minutes',type=int,default=60);args=p.parse_args()
    server=ThreadingHTTPServer((args.bind,args.port),Handler);timer=threading.Timer(args.minutes*60,server.shutdown);timer.daemon=True;timer.start()
    print(f'Local phone installer: http://{args.bind}:{args.port}/ (expires after {args.minutes} minutes)',flush=True)
    try:server.serve_forever()
    finally:timer.cancel();server.server_close()

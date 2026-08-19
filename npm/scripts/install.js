#!/usr/bin/env node
'use strict';
/* postinstall: download the matching prebuilt `hellish` binary from the
 * GitHub Release for this package version. Never hard-fails the install — if
 * the download is unavailable, we print guidance and exit 0. */
const fs = require('fs');
const path = require('path');
const https = require('https');
const { version } = require('../package.json');

const REPO = 'Univers42/hellish';
const plat = process.platform;
const arch = process.arch;

function bail(msg) {
  console.error('hellish-shell:', msg);
  process.exit(0);
}

if (plat !== 'linux' || arch !== 'x64') {
  bail(`no prebuilt binary for ${plat}-${arch} yet — build from source: ` +
    `https://github.com/${REPO}`);
}

const asset = 'hellish-linux-x86_64';
const url =
  `https://github.com/${REPO}/releases/download/v${version}/${asset}`;
const dest = path.join(__dirname, '..', 'bin', 'hellish');

function download(u, file, cb, redirs) {
  https.get(u, (res) => {
    if ([301, 302, 307, 308].includes(res.statusCode) &&
        res.headers.location && (redirs || 0) < 5) {
      return download(res.headers.location, file, cb, (redirs || 0) + 1);
    }
    if (res.statusCode !== 200) return cb(new Error('HTTP ' + res.statusCode));
    const ws = fs.createWriteStream(file, { mode: 0o755 });
    res.pipe(ws);
    ws.on('finish', () => ws.close(cb));
    ws.on('error', cb);
  }).on('error', cb);
}

fs.mkdirSync(path.dirname(dest), { recursive: true });
download(url, dest, (err) => {
  if (err) return bail('download failed (' + err.message + ') — get it from ' +
    `https://github.com/${REPO}/releases`);
  fs.chmodSync(dest, 0o755);
  console.log('hellish-shell: installed binary for v' + version);
}, 0);

#!/usr/bin/env node
'use strict';
/* Thin launcher: exec the prebuilt `hellish` binary fetched at install time,
 * forwarding argv, stdio and the exit status. */
const { spawnSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const bin = path.join(__dirname, 'hellish');

if (!fs.existsSync(bin)) {
  console.error('hellish: binary missing — reinstall with `npm rebuild ' +
    'hellish-shell` or grab it from https://github.com/Univers42/hellish/releases');
  process.exit(1);
}

const r = spawnSync(bin, process.argv.slice(2), { stdio: 'inherit' });
if (r.error) {
  console.error('hellish:', r.error.message);
  process.exit(1);
}
process.exit(r.status === null ? 1 : r.status);

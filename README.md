# Browsy

A browser for System 6.

![Browsy screenshot](https://cloud.githubusercontent.com/assets/95347/3683770/631346c6-12ed-11e4-8031-6242d7e36cfc.png)

Currently this is mostly a UI concept, that can read local text files, manage
history, and allow the user to push buttons.

Build using [Retro68](https://github.com/autc04/Retro68/).

## TODO

- Implement scrolling
- Fetch pages over HTTP
- Parse HTML and maybe CSS
- Render pages
- Move build system to the [MPW Emulator](https://github.com/ksherlock/mpw)
  (maybe)

Parsing and rendering code could be taken from another browser project such as netsurf or Dillo.

Network connectivity could be implemented using GUSI, which provides a UNIX-like
API, or by handling the Macintosh TCP stuff directly, which might allow for more
control over responsiveness.

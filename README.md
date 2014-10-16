# Browsy

A browser for System 6.

![Browsy screenshot](https://cloud.githubusercontent.com/assets/95347/3683770/631346c6-12ed-11e4-8031-6242d7e36cfc.png)

## Stuff used

- Development environment: [Retro68](https://github.com/autc04/Retro68/)
- Streaming I/O library: [c-streams](https://github.com/clehner/c-streams)
- HTTP parser: [http-parser](https://github.com/joyent/http-parser)

## TODO

- Implement scrolling
- Debug crashes
- Implement DNS resolution
- Add download window
- Parse HTML and maybe CSS
- Render pages
- Optimize memory usage (Use handles)

Current plan: implement rendering of text into a TextEdit field. Style the text
according to tag type (and later, CSS). Draw form controls and images (maybe)
on top.

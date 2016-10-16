# kpidsd

Various DSD player plugins for [KbMedia Player](http://hwm5.gyao.ne.jp/kobarin/) and its dependencies

## Projects

In this directory, kpidsd.sln contains 4 Visual C++ projects:

- kpidop
  - Implementation of [DSD Audio over PCM Frames (DoP)](http://dsd-guide.com/dop-open-standard) plugin for KbMedia Player, supports DSDIFF, DSF, WSD formats.
- kpid2p
  - DSD to PCM converter plugin for KbMedia Player, supports DSDIFF, DSF, WSD formats.
- kpidsd
  - Direct DSD player plugin for KbMedia Player (requires ASIO DSD extension)
- libdsd
  - Support library for implementing various DSD file formats


## License

Copyright (C) 2015, 2016, Autch.net.

Most of codes are licensed under the MIT license, see COPYING file in each subprojects for details.

BMJ's Audio Programming
=======================

Exploring real-time audio programming in C++.

## Build

```
$ git clone --recursive https://github.com/bartjoyce/bmjap
$ cd bmjap
$ mkdir build
$ cd build
$ cmake ..
$ make
```

## Dependencies

- [ddui](https://github.com/bartjoyce/ddui)
- [libsoundio](https://github.com/andrewrk/libsoundio)

## Order of things

1. [ ] Introduction to the environment and frameworks (cmake, xcode, nanovg, ddui)
2. [ ] Loading and decoding an audio file from disk (stb_vorbis)
3. [ ] Streaming an audio file to speakers (libsoundio)
4. [ ] Rendering a waveform to the screen
5. [ ] Audio FX -- naïve EQ effect: high-cut
6. [ ] Audio FX -- naïve EQ effect: low-cut, shelves
7. [ ] Audio FX -- naïve EQ effect: band pass

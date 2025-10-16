**Allegro4 API Wrapper for Allegro5**

This project provides the Allegro4 API implemented on top of Allegro5.

### Build

```sh
make
```

### Run Examples

Copy the required data files and run an example:

```sh
cp *.dat build/examples/
./build/examples/exgui
```

### Run Demos

```sh
(cd demos/sopwith && ../../build/demos/sopwith/sopwith)
```

### Build Static Library

To generate `liballegro4-to-5.a`:

```sh
make library
```
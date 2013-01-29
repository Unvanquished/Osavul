# Osavul

Unvanquished server browser

## Build instructions
### Generic
```
qmake && make -j2
lrelease osavul.pro
./osavul
```

### Mac OS X
```
qmake -spec unsupported/macx-clang
lrelease osavul.pro
make
```

If you need to hard-code a path for the translation files, pass
```
EXTRA_CXXFLAGS='-DTRANSLATIONS_DIR="/path/to/translations"'
```
(with the path suitably replaced) to make.

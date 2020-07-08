# VerifyPN
VerifyPN is based on [PeTe](https://github.com/jopsen/PeTe) and aims to provide
a fast untimed engine for TAPAAL.

## License
VerifyPN is available under the terms of the GNU GPL version 3 or
(at your option) any later version.
If this license doesn't suit you're welcome to contact us, and purpose an
alternative license.

## Compilation
Requirements for compilation
```
cmake >= 3.9
flex >= 2.6.4
bison >= 3.0.5
gmp-static (required only for model checking competition)
```

The four distributions of VerifyPN can be compiled as follows
### Linux64 and OSX64
```
mkdir build
cd  build
cmake .. -DVERIFYPN_Static=ON -DVERIFYPN_MC_Simplification=OFF -DVERIFYPN_TAR=OFF
```

### Windows 64 cross-compilation with minGW
```
mkdir build
cd  build
cmake .. -DVERIFYPN_Static=ON -DVERIFYPN_MC_Simplification=OFF -DVERIFYPN_TAR=OFF
```

### Linux64 - Model Checking Competition
```
mkdir build
cd  build
cmake .. -DVERIFYPN_Static=OFF -DVERIFYPN_MC_Simplification=ON -DVERIFYPN_TAR=ON
```

### Mac 64 compilation
```
mkdir build
cd build
cmake -DBISON_EXECUTABLE=/usr/local/opt/bison/bin/bison -DFLEX_EXECUTABLE=/usr/local/opt/flex/bin/flex -DCMAKE_C_COMPILER=/usr/local/bin/gcc-9 -DCMAKE_CXX_COMPILER=/usr/local/bin/g++-9 ..
```


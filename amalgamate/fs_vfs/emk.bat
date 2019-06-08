:call emcc --bind --memory-init-file 0 -std=c++11 -O2 -o ./jsox-wasm.js -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -D__LINUX__  -s EXPORTED_FUNCTIONS="['_jsox_begin_parse','_jsox_parse_clear_state','_jsox_parse_add_data']" jsox.c

:--pre-js or --post-js
:call emcc --memory-init-file 0 -O2 -o ./vfs-fs-w.js -std=c++11 --bind -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -DUSE_STDIO=1 -DNO_SSL=1 -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.cc emscripten_bindings.cc

:call emcc --memory-init-file 0 -o ./vfs-fs-w0.js -std=c++11 --bind -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -DUSE_STDIO=1 -DNO_SSL=1 -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.cc emscripten_bindings.cc


set EXPORTS=
set EXPORTS='_initFS'
:set EXPORTS=%EXPORTS%,


@set CFLAGS=
@set CFLAGS=%CFLAGS% -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference
@set CFLAGS=%CFLAGS% -DUSE_STDIO=1 -DNO_SSL=1 -D__LINUX__ 
@set CFLAGS=%CFLAGS% -s WASM=1

@set CFLAGS=%CFLAGS% -D_DEBUG

:-s RESERVED_FUNCTION_POINTERS=20

:call emcc --memory-init-file 0 -s EXTRA_EXPORTED_RUNTIME_METHODS=["stringToUTF8"] -s EXPORTED_FUNCTIONS="[%EXPORTS%]" -O2 -o ./vfs-fs-w.js %CFLAGS%  vfs_fs.c emscripten_bindings.c
call emcc -s EXPORTED_FUNCTIONS="[%EXPORTS%]" -g -o ./vfs-fs-w.js  %CFLAGS%  vfs_fs.c
call emcc -s WASM=1 -s EXTRA_EXPORTED_RUNTIME_METHODS=["stringToUTF8"] -s EXPORTED_FUNCTIONS="[%EXPORTS%]" -g -o ./vfs-fs-wasm.js %CFLAGS% vfs_fs.c objStore.c

:call emcc --memory-init-file 0 -o ./vfs-fs-w0.js   -Wno-address-of-packed-member -Wno-parentheses -Wno-comment -Wno-null-dereference -DUSE_STDIO=1 -DNO_SSL=1 -D__LINUX__ -s RESERVED_FUNCTION_POINTERS=20  vfs_fs.c

call google-closure-compiler.cmd --language_out NO_TRANSPILE --formatting=pretty_print  --js=vfs-fs-w.js --js_output_file=vfs-fs-w-pretty.js

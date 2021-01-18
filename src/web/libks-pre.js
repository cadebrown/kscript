var Module = {
    '_write_stdout': [],
    '_write_stderr': [],
    'print': function(text) {
        for (var i = 0; i < Module._write_stdout.length; ++i) {
            Module._write_stdout[i](text);
        } 
    },
    'printErr': function(text) {
        for (var i = 0; i < Module._write_stderr.length; ++i) {
            Module._write_stderr[i](text);
        }
    }
};
